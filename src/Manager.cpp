#include "Manager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <format>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include "Settings.h"

namespace {
    constexpr auto kPollInterval = std::chrono::milliseconds(200);

    std::mutex g_mutex;
    std::vector<Manager::ActiveDisease> g_active;

    // game-thread only
    std::unordered_map<RE::FormID, double> g_firstSeen;
    bool g_firstScan = true;

    std::atomic<bool> g_inGame{false};
    std::atomic<bool> g_taskQueued{false};
    std::atomic<bool> g_started{false};
    std::atomic<bool> g_menuOpen{false};
    std::atomic<bool> g_dialogueOpen{false};
    std::atomic<bool> g_inFurniture{false};

    const Settings::DiseaseDef* FindDefinition(const Settings::DefinitionList& defs, const RE::SpellItem* spell,
                                               bool isDisease) {
        const RE::FormID id = spell->GetFormID();
        const char* rawName = spell->GetName();
        const std::string name = rawName ? Utils::ToLower(rawName) : std::string{};
        // exact form matches always win, even if the spell is not of type Disease (e.g. Survival Mode ailments)
        for (const auto& def : defs) {
            if (std::ranges::find(def.resolvedForms, id) != def.resolvedForms.end()) {
                return &def;
            }
        }
        for (const auto& def : defs) {
            if (def.requireDiseaseType && !isDisease) {
                continue;
            }
            if (!name.empty() && std::ranges::find(def.names, name) != def.names.end()) {
                return &def;
            }
        }
        return nullptr;
    }

    void UpdateUiFlags() {
        bool menu = false;
        bool dialogue = false;
        if (auto* ui = RE::UI::GetSingleton()) {
            menu = ui->GameIsPaused() || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) ||
                   ui->IsMenuOpen(RE::MainMenu::MENU_NAME);
            dialogue = ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME);
        }
        bool furniture = false;
        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            const auto handle = player->GetOccupiedFurniture();
            const auto ref = handle.get();
            furniture = ref.get() != nullptr;
        }
        g_inFurniture = furniture;
        g_menuOpen = menu;
        g_dialogueOpen = dialogue;
    }

    // Runs on the game thread.
    void Refresh() {
        UpdateUiFlags();
        if (!g_inGame) {
            return;
        }
        const double now = Utils::NowSeconds();
        const auto defs = Settings::GetDefinitions();
        std::vector<Manager::ActiveDisease> found;
        std::unordered_set<RE::FormID> seen;

        const auto* player = RE::PlayerCharacter::GetSingleton();
        auto* target = player ? const_cast<RE::PlayerCharacter*>(player)->AsMagicTarget() : nullptr;
        if (target) {
            // VisitActiveEffects works on every runtime. GetActiveEffectList() returns nullptr on VR in an all-runtime
            // build, which would hide every disease there. The walk is not locked, so the callback only copies plain
            // data (names, ids, icons) out of the effect and never keeps the pointer.
            auto visit = [&](RE::ActiveEffect* effect) {
                if (!effect || !effect->spell) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }
                const auto* spell = effect->spell->As<RE::SpellItem>();
                if (!spell || !seen.insert(spell->GetFormID()).second) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }
                const bool isDisease = spell->GetSpellType() == RE::MagicSystem::SpellType::kDisease;
                const auto* def = FindDefinition(*defs, spell, isDisease);
                if (!def && !isDisease) {
                    return RE::BSContainer::ForEachResult::kContinue;
                }
                Manager::ActiveDisease entry;
                const char* rawName = spell->GetName();
                if (def) {
                    entry.id = def->id;
                    entry.name = def->displayName;
                    entry.spellName = (rawName && *rawName) ? rawName : def->displayName;
                    entry.icon = def->icon;
                    entry.tint = def->tint;
                    entry.hasTint = def->hasTint;
                } else {
                    entry.id = std::format("form:{:08X}", spell->GetFormID());
                    entry.name = (rawName && *rawName) ? rawName : "Unknown Disease";
                    entry.icon = "generic";
                    entry.known = false;
                }
                const auto [it, inserted] = g_firstSeen.try_emplace(spell->GetFormID(), now);
                if (inserted && g_firstScan) {
                    it->second = now - 3600.0;  // already present when the save loaded: no "new" pulse
                }
                entry.firstSeen = it->second;
                found.push_back(std::move(entry));
                return RE::BSContainer::ForEachResult::kContinue;
            };
            // VisitActiveEffects only exists in an all-runtime build of CommonLibSSE-NG (where GetActiveEffectList() is a VR shim); builds without
            // it (Skyrim SE / AE 1.6.1170 and newer) walk the native list instead. A generic lambda lets the compiler pick whichever exists.
            auto walk = [&](auto* magicTarget) {
                if constexpr (requires { magicTarget->VisitActiveEffects(visit); }) {
                    magicTarget->VisitActiveEffects(visit);
                } else if (auto* effects = magicTarget->GetActiveEffectList()) {
                    for (auto* effect : *effects) {
                        visit(effect);
                    }
                }
            };
            walk(target);
        }
        std::erase_if(g_firstSeen, [&](const auto& kv) { return !seen.contains(kv.first); });
        g_firstScan = false;

        // Some diseases are made of several spells (Last Seed: the infection and its stage, "Ataxia" and "Mild Ataxia"). One icon per disease:
        // the effects that share a definition are merged, and the label becomes the longest spell name (the stage).
        {
            std::unordered_map<std::string, std::size_t> byId;
            std::vector<Manager::ActiveDisease> merged;
            for (auto& e : found) {
                if (!e.known) {
                    merged.push_back(std::move(e));
                    continue;
                }
                // Same icon and same display name = same disease, even when two definitions matched (the infection can match one of the
                // shipped definitions by its name while its stage matches another one added by a mod).
                const std::string key = e.icon + "|" + Utils::ToLower(e.name);
                const auto it = byId.find(key);
                if (it == byId.end()) {
                    byId.emplace(key, merged.size());
                    merged.push_back(std::move(e));
                } else {
                    auto& m = merged[it->second];
                    if (e.spellName.size() > m.spellName.size()) {
                        m.spellName = e.spellName;
                        m.name = e.spellName;
                    }
                    m.firstSeen = std::min(m.firstSeen, e.firstSeen);
                }
            }
            found = std::move(merged);
        }

        std::ranges::sort(found, [](const auto& a, const auto& b) {
            return a.firstSeen != b.firstSeen ? a.firstSeen < b.firstSeen : a.name < b.name;
        });
        std::scoped_lock lock(g_mutex);
        g_active = std::move(found);
    }

    void PollLoop() {
        while (true) {
            std::this_thread::sleep_for(kPollInterval);
            if (g_taskQueued.exchange(true)) {
                continue;  // previous refresh not executed yet
            }
            SKSE::GetTaskInterface()->AddTask([] {
                Refresh();
                g_taskQueued = false;
            });
        }
    }
}

void Manager::Init() {
    if (g_started.exchange(true)) {
        return;
    }
    std::thread(PollLoop).detach();
    logger::info("Disease poll thread started");
}

void Manager::OnGameLoadStarted() {
    g_inGame = false;
    std::scoped_lock lock(g_mutex);
    g_active.clear();
}

void Manager::OnGameReady() {
    SKSE::GetTaskInterface()->AddTask([] {
        g_firstSeen.clear();
        g_firstScan = true;
        g_inGame = true;
    });
}

std::vector<Manager::ActiveDisease> Manager::GetActive() {
    std::scoped_lock lock(g_mutex);
    return g_active;
}

bool Manager::IsMenuOpen() { return g_menuOpen; }
bool Manager::IsDialogueOpen() { return g_dialogueOpen; }
bool Manager::IsInFurniture() { return g_inFurniture; }
