#include "Hud.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <unordered_map>
#include <vector>

#include "Manager.h"
#pragma warning(push)
#pragma warning(disable : 4099 5054)
#include "SKSEMenuFramework.h"
#pragma warning(pop)
#include "Settings.h"

namespace {
    std::unordered_map<std::string, ImGuiMCP::ImTextureID> g_textures;  // render thread only
    std::atomic<bool> g_invalidate{false};
    double g_lastPreview = -100.0;
    bool g_unlocked = false;  // the config page lets the mouse move the widget
    bool g_dragging = false;
    bool g_moved = false;

    struct Entry {
        std::string label;
        std::string icon;
        Utils::Color tint;
        double firstSeen;
    };

    struct Metrics {
        float width = 0, height = 0, textW = 0, textH = 0;
    };

    ImGuiMCP::ImU32 Pack(const Utils::Color& c, float alphaMul) {
        auto byte = [](float v) { return static_cast<int>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f)); };
        return IM_COL32(byte(c[0]), byte(c[1]), byte(c[2]), byte(c[3] * alphaMul));
    }

    ImGuiMCP::ImTextureID LoadIcon(const std::string& icon, int bucket) {
        const std::string basePath = "Data\\Interface\\DiseaseWidget\\Icons\\" + icon;
        auto tex = SKSEMenuFramework::LoadTexture(basePath + ".svg",
                                                  ImGuiMCP::ImVec2{static_cast<float>(bucket), static_cast<float>(bucket)});
        if (!tex) {
            tex = SKSEMenuFramework::LoadTexture(basePath + ".png");
        }
        if (!tex) {
            logger::warn("Disease icon '{}' could not be loaded", icon);
        }
        return tex;
    }

    std::vector<Entry> BuildEntries(const Settings::Config& cfg, bool previewing, double now) {
        std::vector<Entry> entries;
        if (previewing && cfg.previewCount > 0) {
            const auto defs = Settings::GetDefinitions();
            for (const auto& def : *defs) {
                if (static_cast<int>(entries.size()) >= cfg.previewCount) {
                    break;
                }
                if (cfg.disabled.contains(def.id)) {
                    continue;
                }
                entries.push_back({def.displayName, def.icon, def.hasTint ? def.tint : cfg.iconColor, now - 100.0});
            }
            return entries;
        }
        for (const auto& active : Manager::GetActive()) {
            if (cfg.disabled.contains(active.id) || (!active.known && !cfg.showUnknownDiseases)) {
                continue;
            }
            entries.push_back({active.name, active.icon, active.hasTint ? active.tint : cfg.iconColor, active.firstSeen});
        }
        return entries;
    }

    void Draw(const Settings::Config& cfg, const std::vector<Entry>& entries, double now, bool previewing) {
        using namespace ImGuiMCP;
        const ImVec2 display = GetIO()->DisplaySize;
        const float uiScale = display.y / 1080.0f;
        const float size = cfg.iconSize * cfg.scale * uiScale;
        const float gap = cfg.spacing * uiScale;
        const float textGap = 4.0f * uiScale;
        const float baseFont = GetFontSize();
        const float textFactor = cfg.textScale * std::clamp(uiScale, 0.75f, 2.0f);
        ImFont* font = GetFont();
        ImDrawList* drawList = GetForegroundDrawList();

        std::vector<Metrics> metrics(entries.size());
        for (std::size_t i = 0; i < entries.size(); ++i) {
            auto& m = metrics[i];
            if (cfg.label != Settings::kLabelNone) {
                const ImVec2 t = CalcTextSize(entries[i].label.c_str());
                m.textW = t.x * textFactor;
                m.textH = t.y * textFactor;
            }
            if (cfg.label == Settings::kLabelBelow) {
                m.width = std::max(size, m.textW);
                m.height = size + textGap + m.textH;
            } else if (cfg.label == Settings::kLabelSide) {
                m.width = size + textGap * 2.0f + m.textW;
                m.height = size;
            } else {
                m.width = size;
                m.height = size;
            }
        }

        const bool vertical = cfg.grow == Settings::kGrowDown || cfg.grow == Settings::kGrowUp;
        if (vertical) {
            // one shared column width so every icon lines up, whatever the length of its name
            float column = 0.0f;
            for (const auto& m : metrics) column = std::max(column, m.width);
            for (auto& m : metrics) m.width = column;
        }

        const float anchorX = cfg.posX * display.x;
        const float anchorY = cfg.posY * display.y;
        std::vector<ImVec2> positions(entries.size());
        float cursor = 0.0f;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& m = metrics[i];
            float x = anchorX, y = anchorY;
            switch (cfg.grow) {
                case Settings::kGrowRight: x = anchorX + cursor; cursor += m.width + gap; break;
                case Settings::kGrowLeft:  x = anchorX - cursor - m.width; cursor += m.width + gap; break;
                case Settings::kGrowDown:  y = anchorY + cursor; cursor += m.height + gap; break;
                default:                   y = anchorY - cursor - m.height; cursor += m.height + gap; break;
            }
            positions[i] = ImVec2{x, y};
        }
        // keep the whole block on screen by shifting it as one piece (never per entry, that breaks alignment)
        float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
        for (std::size_t i = 0; i < entries.size(); ++i) {
            minX = std::min(minX, positions[i].x);
            minY = std::min(minY, positions[i].y);
            maxX = std::max(maxX, positions[i].x + metrics[i].width);
            maxY = std::max(maxY, positions[i].y + metrics[i].height);
        }
        float shiftX = 0.0f, shiftY = 0.0f;
        if (minX < 0.0f) shiftX = -minX; else if (maxX > display.x) shiftX = display.x - maxX;
        if (minY < 0.0f) shiftY = -minY; else if (maxY > display.y) shiftY = display.y - maxY;
        const float ringThick = std::max(1.0f, 1.6f * uiScale);

        // Moving it with the mouse (config page open, unlocked): click the widget and drag. The anchor moves by the mouse's move.
        if (previewing && g_unlocked && !entries.empty()) {
            auto* io = GetIO();
            const ImVec2 mouse = io->MousePos;
            const float pad = 8.0f * uiScale;
            const float bx0 = minX + shiftX - pad, by0 = minY + shiftY - pad, bx1 = maxX + shiftX + pad, by1 = maxY + shiftY + pad;
            const bool inside = mouse.x >= bx0 && mouse.x <= bx1 && mouse.y >= by0 && mouse.y <= by1;
            if (!g_dragging && IsMouseClicked(0) && !io->WantCaptureMouse && inside) {
                g_dragging = true;
            }
            if (g_dragging) {
                if (IsMouseDown(0)) {
                    auto& live = Settings::Get();
                    live.posX = std::clamp(live.posX + io->MouseDelta.x / display.x, 0.0f, 1.0f);
                    live.posY = std::clamp(live.posY + io->MouseDelta.y / display.y, 0.0f, 1.0f);
                    g_moved = true;
                } else {
                    g_dragging = false;
                }
            }
            const bool active = g_dragging || inside;
            ImDrawListManager::AddRect(drawList, ImVec2{bx0, by0}, ImVec2{bx1, by1},
                                       active ? IM_COL32(255, 220, 110, 242) : IM_COL32(255, 255, 255, 115), 3.0f, 0, 1.5f);
        } else {
            g_dragging = false;
        }

        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& e = entries[i];
            const auto& m = metrics[i];
            const float x = positions[i].x + shiftX;
            const float y = positions[i].y + shiftY;

            const double age = now - e.firstSeen;
            const float fade = cfg.fadeSeconds > 0.01f ? static_cast<float>(std::clamp(age / cfg.fadeSeconds, 0.0, 1.0)) : 1.0f;
            const float alpha = cfg.opacity * fade;
            float ringAlpha = alpha;
            if (cfg.pulseNewDiseases && age < 5.0) {
                ringAlpha *= 0.7f + 0.3f * static_cast<float>(std::sin(age * 6.0));
            }

            const float iconX = (cfg.label == Settings::kLabelBelow) ? x + (m.width - size) * 0.5f : x;
            const ImVec2 center{iconX + size * 0.5f, y + size * 0.5f};

            if (cfg.showPlate) {
                ImDrawListManager::AddCircleFilled(drawList, center, size * 0.5f, Pack(cfg.plateColor, alpha), 48);
            }
            if (cfg.showRing) {
                ImDrawListManager::AddCircle(drawList, center, size * 0.5f - ringThick * 0.5f, Pack(cfg.ringColor, ringAlpha), 48, ringThick);
                ImDrawListManager::AddCircle(drawList, center, size * 0.45f, Pack(cfg.ringColor, ringAlpha * 0.45f), 48, std::max(1.0f, ringThick * 0.6f));
            }
            const float half = size * 0.36f;
            if (auto* tex = Hud::GetIconTexture(e.icon, half * 2.0f)) {
                const Utils::Color tint = cfg.useDiseaseColors ? e.tint : cfg.iconColor;
                ImDrawListManager::AddImage(drawList, tex, ImVec2{center.x - half, center.y - half},
                                            ImVec2{center.x + half, center.y + half}, ImVec2{0, 0}, ImVec2{1, 1},
                                            Pack(tint, alpha));
            }

            if (cfg.label != Settings::kLabelNone) {
                ImVec2 pos;
                if (cfg.label == Settings::kLabelBelow) {
                    pos = ImVec2{x + (m.width - m.textW) * 0.5f, y + size + textGap};
                } else {
                    pos = ImVec2{x + size + textGap * 2.0f, y + (size - m.textH) * 0.5f};
                }
                const float fontSize = baseFont * textFactor;
                ImDrawListManager::AddText(drawList, font, fontSize, ImVec2{pos.x + 1.0f, pos.y + 1.0f},
                                           IM_COL32(0, 0, 0, static_cast<int>(200 * alpha)), e.label.c_str());
                ImDrawListManager::AddText(drawList, font, fontSize, pos, Pack(cfg.textColor, alpha), e.label.c_str());
            }
        }
    }

    void __stdcall RenderOverlay() {
        const auto& cfg = Settings::Get();
        if (!cfg.enabled) {
            return;
        }
        const double now = Utils::NowSeconds();
        const bool previewing = (now - g_lastPreview) < 0.35;
        if (!previewing) {
            g_unlocked = false;  // the config page is closed: the widget locks itself again
            if (SKSEMenuFramework::IsAnyBlockingWindowOpened()) return;
            if (cfg.hideInMenus && Manager::IsMenuOpen()) return;
            if (cfg.hideInDialogue && Manager::IsDialogueOpen()) return;
            if (cfg.hideInFurniture && Manager::IsInFurniture()) return;
        }
        const auto entries = BuildEntries(cfg, previewing, now);
        if (!entries.empty()) {
            Draw(cfg, entries, now, previewing);
        }
    }
}

void Hud::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        logger::warn("SKSE Menu Framework not found - the HUD widget and its menu are disabled");
        return;
    }
    SKSEMenuFramework::AddHudElement(RenderOverlay);  // intentionally never freed
    logger::info("HUD element registered");
}

void Hud::MarkPreview() { g_lastPreview = Utils::NowSeconds(); }

bool& Hud::Unlocked() { return g_unlocked; }

bool Hud::TakeMoved() {
    const bool moved = g_moved;
    g_moved = false;
    return moved;
}

void Hud::InvalidateIcons() { g_invalidate = true; }

void* Hud::GetIconTexture(const std::string& icon, float px) {
    if (g_invalidate.exchange(false)) {
        g_textures.clear();
    }
    const int bucket = px <= 64.0f ? 64 : px <= 128.0f ? 128 : px <= 256.0f ? 256 : 512;
    const std::string key = std::format("{}@{}", icon, bucket);
    if (const auto it = g_textures.find(key); it != g_textures.end()) {
        return it->second;
    }
    auto tex = LoadIcon(icon, bucket);
    if (!tex && icon != "generic") {
        tex = LoadIcon("generic", bucket);
    }
    g_textures[key] = tex;  // failures are cached too, so we do not hit the disk every frame
    return tex;
}
