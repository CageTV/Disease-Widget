#include "Hud.h"
#include "MCP.h"
#include "Manager.h"
#include "Settings.h"
#include "logger.h"

void OnMessage(SKSE::MessagingInterface::Message* message) {
    switch (message->type) {
        case SKSE::MessagingInterface::kDataLoaded:
            Settings::LoadConfig();
            Settings::LoadDefinitions();
            Manager::Init();
            MCP::Register();
            Hud::Register();
            break;
        case SKSE::MessagingInterface::kPreLoadGame:
            Manager::OnGameLoadStarted();
            break;
        case SKSE::MessagingInterface::kNewGame:
        case SKSE::MessagingInterface::kPostLoadGame:
            Manager::OnGameReady();
            break;
        default:
            break;
    }
}

#ifdef DW_LEGACY_LIB
// CharmedBaryon's CommonLibSSE-NG (the "older" build: Skyrim VR and 1.6.1130 and older) has no SKSE_PLUGIN_LOAD / Init options.
SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SetupLog();
    logger::info("Plugin loaded");
    SKSE::Init(skse);
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
#else
SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse) {
    // SKSE::Init must run first: it resolves the plugin name/handle and the SKSE interfaces.
    // It also sets up its own default log, which would reopen (and truncate) the same
    // <plugin>.log, so it is turned off here and SetupLog() below creates the log instead.
    SKSE::Init(skse, {.log = false});
    SetupLog();
    logger::info("Plugin loaded");
    SKSE::GetMessagingInterface()->RegisterListener(OnMessage);
    return true;
}
#endif
