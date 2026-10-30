#pragma once

#include <string>
#include <vector>

#include "Utils.h"

namespace Manager {

    struct ActiveDisease {
        std::string id;    // definition id, or "form:XXXXXXXX" for diseases without a definition
        std::string name;  // display name
        std::string spellName;  // name of the spell the disease came from (several spells can make up one disease)
        std::string icon;  // icon file stem
        Utils::Color tint{0.93f, 0.89f, 0.80f, 1.0f};
        bool hasTint = false;
        bool known = true;  // false = generic icon for an unrecognised Disease-type spell
        double firstSeen = 0.0;
    };

    void Init();             // kDataLoaded: starts the poll thread
    void OnGameLoadStarted();  // kPreLoadGame
    void OnGameReady();        // kPostLoadGame / kNewGame

    // Thread-safe copy of the latest snapshot (taken on the game thread).
    std::vector<ActiveDisease> GetActive();
    bool IsMenuOpen();      // pausing menu, loading screen or main menu
    bool IsDialogueOpen();
    bool IsInFurniture();   // player is using furniture (sitting, sleeping, crafting station, ...)
}
