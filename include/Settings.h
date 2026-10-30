#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "Utils.h"

namespace Settings {

    enum GrowDirection : int { kGrowRight = 0, kGrowLeft, kGrowDown, kGrowUp };
    enum LabelMode : int { kLabelNone = 0, kLabelBelow, kLabelSide };

    // User-facing configuration, stored in Settings.json and edited through the SKSE Menu Framework.
    struct Config {
        // behaviour
        bool enabled = true;
        bool hideInMenus = true;
        bool hideInDialogue = true;
        bool hideInFurniture = true;
        bool showUnknownDiseases = true;

        // placement (anchor = fraction of the screen, scaled for 1080p-relative sizes)
        float posX = 0.02f;
        float posY = 0.78f;
        int grow = kGrowRight;
        int label = kLabelBelow;
        float iconSize = 56.0f;  // px at 1080p
        float scale = 1.0f;
        float spacing = 10.0f;   // px at 1080p
        float textScale = 0.9f;

        // look
        float opacity = 1.0f;
        bool showPlate = true;
        bool showRing = true;
        bool useDiseaseColors = false;
        bool pulseNewDiseases = true;
        float fadeSeconds = 0.6f;
        Utils::Color plateColor{0.05f, 0.045f, 0.04f, 0.72f};
        Utils::Color ringColor{0.78f, 0.72f, 0.55f, 0.85f};
        Utils::Color iconColor{0.93f, 0.89f, 0.80f, 1.0f};
        Utils::Color textColor{0.93f, 0.89f, 0.80f, 1.0f};

        // preview used while the configuration pages are open (0 = show the real active diseases)
        int previewCount = 3;

        // disease definition ids the user has switched off
        std::unordered_set<std::string> disabled;
    };

    // One entry of Diseases.json
    struct DiseaseDef {
        std::string id;
        std::string displayName;
        std::string icon;                       // file stem in Data/Interface/DiseaseWidget/Icons
        std::string source;                     // JSON file(s) the entry came from, for the menu
        std::vector<std::string> names;         // lower-case spell display names
        std::vector<std::string> forms;         // "Plugin.esp|0x1234" as written in the file
        std::vector<RE::FormID> resolvedForms;  // resolved once the data handler is ready
        bool requireDiseaseType = false;
        bool hasTint = false;
        Utils::Color tint{0.93f, 0.89f, 0.80f, 1.0f};
    };
    using DefinitionList = std::vector<DiseaseDef>;

    Config& Get();
    void LoadConfig();
    void SaveConfig();
    void ResetConfig();

    // Needs the data handler (call at/after kDataLoaded). Creates Diseases.json when it does not exist, then merges every
    // other *.json in Data/SKSE/Plugins/DiseaseWidget (other mods can drop their own definition files there).
    void LoadDefinitions();
    std::shared_ptr<const DefinitionList> GetDefinitions();
}
