#include "MCP.h"

#include "Hud.h"
#include "Manager.h"
#pragma warning(push)
#pragma warning(disable : 4099 5054)
#include "SKSEMenuFramework.h"
#pragma warning(pop)
#include "Settings.h"

namespace {
    bool g_dirty = false;

    void SaveIfDirty() {
        if (g_dirty) {
            Settings::SaveConfig();
            g_dirty = false;
        }
    }

    void __stdcall OnMenuEvent(SKSEMenuFramework::Model::EventType type) {
        if (type == SKSEMenuFramework::Model::kCloseMenu) {
            SaveIfDirty();
        }
    }

    void Tip(const char* text) {
        if (ImGuiMCP::IsItemHovered()) {
            ImGuiMCP::SetTooltip("%s", text);
        }
    }

    void Place(Settings::Config& c, float x, float y, int grow) {
        c.posX = x;
        c.posY = y;
        c.grow = grow;
    }

    void __stdcall RenderSettings() {
        using namespace ImGuiMCP;
        Hud::MarkPreview();
        auto& c = Settings::Get();
        bool ch = false;

        SeparatorText("Widget");
        ch |= Checkbox("Enable disease widget", &c.enabled);
        ch |= Checkbox("Hide while menus are open", &c.hideInMenus);
        ch |= Checkbox("Hide during dialogue", &c.hideInDialogue);
        ch |= Checkbox("Hide while using furniture (sitting, sleeping, crafting)", &c.hideInFurniture);
        ch |= Checkbox("Show unrecognised diseases (generic icon)", &c.showUnknownDiseases);
        Tip("Any spell of type 'Disease' without an entry in Diseases.json uses the generic flask icon.");

        SeparatorText("Position & layout");
        ch |= SliderFloat("Horizontal position", &c.posX, 0.0f, 1.0f, "%.3f");
        ch |= SliderFloat("Vertical position", &c.posY, 0.0f, 1.0f, "%.3f");
        if (Button("Top left")) { Place(c, 0.02f, 0.08f, Settings::kGrowRight); ch = true; }
        SameLine();
        if (Button("Top right")) { Place(c, 0.98f, 0.08f, Settings::kGrowLeft); ch = true; }
        SameLine();
        if (Button("Bottom left")) { Place(c, 0.02f, 0.78f, Settings::kGrowRight); ch = true; }
        SameLine();
        if (Button("Bottom right")) { Place(c, 0.98f, 0.78f, Settings::kGrowLeft); ch = true; }
        ch |= Combo("Grow direction", &c.grow, "Right\0Left\0Down\0Up\0");
        ch |= Combo("Names", &c.label, "None\0Below icon\0Beside icon\0");
        ch |= SliderFloat("Icon size", &c.iconSize, 24.0f, 128.0f, "%.0f px");
        ch |= SliderFloat("Scale", &c.scale, 0.5f, 3.0f, "%.2f");
        ch |= SliderFloat("Spacing", &c.spacing, 0.0f, 40.0f, "%.0f px");
        ch |= SliderFloat("Name text scale", &c.textScale, 0.5f, 2.0f, "%.2f");

        SeparatorText("Appearance");
        ch |= SliderFloat("Opacity", &c.opacity, 0.1f, 1.0f, "%.2f");
        ch |= Checkbox("Dark plate behind icons", &c.showPlate);
        ch |= Checkbox("Ring", &c.showRing);
        ch |= Checkbox("Colour icons by disease", &c.useDiseaseColors);
        ch |= Checkbox("Pulse when a disease is newly contracted", &c.pulseNewDiseases);
        ch |= SliderFloat("Fade-in time", &c.fadeSeconds, 0.0f, 3.0f, "%.1f s");
        ch |= ColorEdit4("Plate colour", c.plateColor.data(), ImGuiColorEditFlags_AlphaBar);
        ch |= ColorEdit4("Ring colour", c.ringColor.data(), ImGuiColorEditFlags_AlphaBar);
        ch |= ColorEdit4("Icon tint", c.iconColor.data(), ImGuiColorEditFlags_AlphaBar);
        ch |= ColorEdit4("Text colour", c.textColor.data(), ImGuiColorEditFlags_AlphaBar);

        SeparatorText("Preview");
        ch |= SliderInt("Preview icons", &c.previewCount, 0, 15);
        Tip("While this menu is open the widget shows this many sample icons. 0 = show your real active diseases.");

        Spacing();
        if (Button("Save now")) { Settings::SaveConfig(); g_dirty = false; }
        SameLine();
        if (Button("Reload from disk")) { Settings::LoadConfig(); g_dirty = false; }
        SameLine();
        if (Button("Reset to defaults")) { Settings::ResetConfig(); ch = true; }
        TextDisabled("Settings are saved automatically when this menu closes.");
        g_dirty |= ch;
    }

    void __stdcall RenderDiseases() {
        using namespace ImGuiMCP;
        Hud::MarkPreview();
        auto& c = Settings::Get();

        SeparatorText("Currently active");
        const auto active = Manager::GetActive();
        if (active.empty()) {
            TextDisabled("No diseases active.");
        }
        for (const auto& d : active) {
            BulletText("%s", d.name.c_str());
        }

        SeparatorText("Known diseases");
        const auto defs = Settings::GetDefinitions();
        if (BeginTable("##diseases", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH)) {
            TableSetupColumn("Show", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 48.0f);
            TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
            TableSetupColumn("Matches (spell names / forms)");
            TableHeadersRow();
            for (const auto& def : *defs) {
                TableNextRow();
                PushID(def.id.c_str());
                TableSetColumnIndex(0);
                bool show = !c.disabled.contains(def.id);
                if (Checkbox("##show", &show)) {
                    if (show) c.disabled.erase(def.id); else c.disabled.insert(def.id);
                    g_dirty = true;
                }
                TableSetColumnIndex(1);
                const ImVec2 pos = GetCursorScreenPos();
                ImDrawListManager::AddRectFilled(GetWindowDrawList(), pos, ImVec2{pos.x + 40.0f, pos.y + 40.0f},
                                                 IM_COL32(18, 16, 14, 235), 6.0f, 0);
                if (auto* tex = Hud::GetIconTexture(def.icon, 40.0f)) {
                    Image(tex, ImVec2{40.0f, 40.0f});
                } else {
                    Dummy(ImVec2{40.0f, 40.0f});
                }
                TableSetColumnIndex(2);
                Text("%s", def.displayName.c_str());
                TableSetColumnIndex(3);
                std::string joined;
                for (const auto& n : def.names) joined += (joined.empty() ? "" : ", ") + n;
                for (const auto& f : def.forms) joined += (joined.empty() ? "" : ", ") + f;
                TextDisabled("%s", joined.c_str());
                if (!def.source.empty()) {
                    TextDisabled("from: %s", def.source.c_str());
                }
                PopID();
            }
            EndTable();
        }
        Spacing();
        if (Button("Reload Diseases.json")) {
            SKSE::GetTaskInterface()->AddTask([] {
                Settings::LoadDefinitions();
                Hud::InvalidateIcons();
            });
        }
        TextDisabled("Diseases.json and every other .json in Data/SKSE/Plugins/DiseaseWidget are read, so mods can ship their own.");
    }
}

void MCP::Register() {
    if (!SKSEMenuFramework::IsInstalled()) {
        return;
    }
    SKSEMenuFramework::SetSection("Disease Widget");
    SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);
    SKSEMenuFramework::AddSectionItem("Diseases", RenderDiseases);
    SKSEMenuFramework::AddEvent(OnMenuEvent, 0.0f);  // never freed on purpose
    logger::info("Menu pages registered");
}
