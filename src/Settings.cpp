#include "Settings.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <mutex>
#include <nlohmann/json.hpp>

#include "DefaultDiseases.h"

namespace {
    using json = nlohmann::json;

    Settings::Config g_config;
    std::mutex g_definitionsMutex;
    std::shared_ptr<const Settings::DefinitionList> g_definitions = std::make_shared<const Settings::DefinitionList>();

    std::filesystem::path ConfigPath() { return Utils::ConfigFolder() / "Settings.json"; }
    std::filesystem::path DefinitionsPath() { return Utils::ConfigFolder() / "Diseases.json"; }

    template <class T>
    void ReadValue(const json& root, const char* key, T& out) {
        const auto it = root.find(key);
        if (it == root.end() || it->is_null()) {
            return;
        }
        try {
            out = it->get<T>();
        } catch (const std::exception& e) {
            logger::warn("Settings: ignoring bad value for '{}': {}", key, e.what());
        }
    }

    void ReadColor(const json& root, const char* key, Utils::Color& out) {
        const auto it = root.find(key);
        if (it == root.end() || !it->is_string()) {
            return;
        }
        Utils::Color parsed = out;
        if (Utils::ParseColor(it->get<std::string>(), parsed)) {
            out = parsed;
        } else {
            logger::warn("Settings: '{}' is not a valid #RRGGBB[AA] colour", key);
        }
    }

    bool ReadFile(const std::filesystem::path& path, json& out) {
        std::ifstream file(path);
        if (!file) {
            return false;
        }
        try {
            out = json::parse(file, nullptr, true, true);  // allow comments
            return true;
        } catch (const std::exception& e) {
            logger::error("Could not parse {}: {}", path.string(), e.what());
            return false;
        }
    }

    bool ParseFormKey(const std::string& text, std::string& plugin, RE::FormID& localId) {
        const auto bar = text.find('|');
        if (bar == std::string::npos || bar == 0 || bar + 1 >= text.size()) {
            return false;
        }
        plugin = text.substr(0, bar);
        try {
            localId = static_cast<RE::FormID>(std::stoul(text.substr(bar + 1), nullptr, 16));
        } catch (...) {
            return false;
        }
        return true;
    }

    void ResolveForms(Settings::DiseaseDef& def) {
        def.resolvedForms.clear();
        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) {
            return;
        }
        for (const auto& key : def.forms) {
            std::string plugin;
            RE::FormID localId = 0;
            if (!ParseFormKey(key, plugin, localId)) {
                logger::warn("Disease '{}': form entry '{}' is not in 'Plugin.esp|0x1234' format", def.id, key);
                continue;
            }
            const RE::FormID resolved = dataHandler->LookupFormID(localId, plugin);
            if (resolved == 0) {
                logger::info("Disease '{}': form '{}' not found (plugin not loaded?)", def.id, key);
                continue;
            }
            def.resolvedForms.push_back(resolved);
        }
    }

    void AppendUnique(std::vector<std::string>& list, std::string value) {
        if (std::ranges::find(list, value) == list.end()) {
            list.push_back(std::move(value));
        }
    }

    // Adds one entry of a definition file. An id that already exists is MERGED, not replaced: the match names and forms are
    // added to it (so a mod can add its own spells to "ataxia"), and name / icon / color / requireDiseaseType only change
    // when the entry actually contains them.
    void ApplyEntry(Settings::DefinitionList& list, const json& item, const std::string& source) {
        const std::string id = item.value("id", std::string{});
        if (id.empty()) {
            logger::warn("{}: skipped an entry without an 'id'", source);
            return;
        }
        auto it = std::ranges::find(list, id, &Settings::DiseaseDef::id);
        if (it == list.end()) {
            Settings::DiseaseDef fresh;
            fresh.id = id;
            fresh.displayName = id;
            fresh.icon = id;
            fresh.source = source;
            list.push_back(std::move(fresh));
            it = std::prev(list.end());
        } else if (it->source.find(source) == std::string::npos) {
            it->source += ", " + source;
        }
        auto& def = *it;
        if (item.contains("name") && item["name"].is_string()) {
            def.displayName = item["name"].get<std::string>();
        }
        if (item.contains("icon") && item["icon"].is_string()) {
            def.icon = item["icon"].get<std::string>();
        }
        if (item.contains("requireDiseaseType") && item["requireDiseaseType"].is_boolean()) {
            def.requireDiseaseType = item["requireDiseaseType"].get<bool>();
        }
        if (item.contains("match") && item["match"].is_array()) {
            for (const auto& n : item["match"]) {
                if (n.is_string()) {
                    AppendUnique(def.names, Utils::ToLower(n.get<std::string>()));
                }
            }
        }
        if (item.contains("forms") && item["forms"].is_array()) {
            for (const auto& f : item["forms"]) {
                if (f.is_string()) {
                    AppendUnique(def.forms, f.get<std::string>());
                }
            }
        }
        if (item.contains("color") && item["color"].is_string()) {
            Utils::Color parsed = def.tint;
            if (Utils::ParseColor(item["color"].get<std::string>(), parsed)) {
                def.tint = parsed;
                def.hasTint = true;
            } else {
                logger::warn("{}: disease '{}' has an invalid colour", source, id);
            }
        }
    }

    // Returns the number of entries read; files without a "diseases" array are ignored (they are not ours).
    std::size_t ApplyFile(Settings::DefinitionList& list, const json& root, const std::string& source) {
        const auto it = root.find("diseases");
        if (it == root.end() || !it->is_array()) {
            logger::info("{}: no 'diseases' array, ignored", source);
            return 0;
        }
        std::size_t count = 0;
        for (const auto& item : *it) {
            try {
                ApplyEntry(list, item, source);
                ++count;
            } catch (const std::exception& e) {
                logger::warn("{}: skipped a malformed entry: {}", source, e.what());
            }
        }
        return count;
    }

    // Diseases.json first, then every other *.json below the DiseaseWidget folder in alphabetical order.
    std::vector<std::filesystem::path> CollectDefinitionFiles() {
        std::vector<std::filesystem::path> extra;
        std::error_code ec;
        const auto folder = Utils::ConfigFolder();
        if (std::filesystem::exists(folder, ec)) {
            for (std::filesystem::recursive_directory_iterator dir(folder, std::filesystem::directory_options::skip_permission_denied, ec), end;
                 !ec && dir != end; dir.increment(ec)) {
                if (!dir->is_regular_file(ec)) {
                    continue;
                }
                const auto& path = dir->path();
                if (Utils::ToLower(path.extension().string()) != ".json") {
                    continue;
                }
                const auto name = Utils::ToLower(path.filename().string());
                if (name == "settings.json" || name == "diseases.json") {
                    continue;
                }
                extra.push_back(path);
            }
        }
        std::ranges::sort(extra, [](const auto& a, const auto& b) { return Utils::ToLower(a.generic_string()) < Utils::ToLower(b.generic_string()); });
        std::vector<std::filesystem::path> files;
        files.push_back(DefinitionsPath());
        files.insert(files.end(), extra.begin(), extra.end());
        return files;
    }
}

Settings::Config& Settings::Get() { return g_config; }

void Settings::ResetConfig() { g_config = Config{}; }

void Settings::LoadConfig() {
    g_config = Config{};
    json root;
    if (!ReadFile(ConfigPath(), root)) {
        logger::info("No Settings.json yet, writing defaults");
        SaveConfig();
        return;
    }
    auto& c = g_config;
    ReadValue(root, "enabled", c.enabled);
    ReadValue(root, "hideInMenus", c.hideInMenus);
    ReadValue(root, "hideInDialogue", c.hideInDialogue);
    ReadValue(root, "hideInFurniture", c.hideInFurniture);
    ReadValue(root, "showUnknownDiseases", c.showUnknownDiseases);
    ReadValue(root, "posX", c.posX);
    ReadValue(root, "posY", c.posY);
    ReadValue(root, "grow", c.grow);
    ReadValue(root, "label", c.label);
    ReadValue(root, "iconSize", c.iconSize);
    ReadValue(root, "scale", c.scale);
    ReadValue(root, "spacing", c.spacing);
    ReadValue(root, "textScale", c.textScale);
    ReadValue(root, "opacity", c.opacity);
    ReadValue(root, "showPlate", c.showPlate);
    ReadValue(root, "showRing", c.showRing);
    ReadValue(root, "useDiseaseColors", c.useDiseaseColors);
    ReadValue(root, "pulseNewDiseases", c.pulseNewDiseases);
    ReadValue(root, "fadeSeconds", c.fadeSeconds);
    ReadValue(root, "previewCount", c.previewCount);
    ReadColor(root, "plateColor", c.plateColor);
    ReadColor(root, "ringColor", c.ringColor);
    ReadColor(root, "iconColor", c.iconColor);
    ReadColor(root, "textColor", c.textColor);
    if (root.contains("disabledDiseases") && root["disabledDiseases"].is_array()) {
        for (const auto& id : root["disabledDiseases"]) {
            if (id.is_string()) {
                c.disabled.insert(id.get<std::string>());
            }
        }
    }
    c.grow = std::clamp(c.grow, 0, 3);
    c.label = std::clamp(c.label, 0, 2);
    logger::info("Settings loaded");
}

void Settings::SaveConfig() {
    const auto& c = g_config;
    json root;
    root["enabled"] = c.enabled;
    root["hideInMenus"] = c.hideInMenus;
    root["hideInDialogue"] = c.hideInDialogue;
    root["hideInFurniture"] = c.hideInFurniture;
    root["showUnknownDiseases"] = c.showUnknownDiseases;
    root["posX"] = c.posX;
    root["posY"] = c.posY;
    root["grow"] = c.grow;
    root["label"] = c.label;
    root["iconSize"] = c.iconSize;
    root["scale"] = c.scale;
    root["spacing"] = c.spacing;
    root["textScale"] = c.textScale;
    root["opacity"] = c.opacity;
    root["showPlate"] = c.showPlate;
    root["showRing"] = c.showRing;
    root["useDiseaseColors"] = c.useDiseaseColors;
    root["pulseNewDiseases"] = c.pulseNewDiseases;
    root["fadeSeconds"] = c.fadeSeconds;
    root["previewCount"] = c.previewCount;
    root["plateColor"] = Utils::FormatColor(c.plateColor);
    root["ringColor"] = Utils::FormatColor(c.ringColor);
    root["iconColor"] = Utils::FormatColor(c.iconColor);
    root["textColor"] = Utils::FormatColor(c.textColor);
    std::vector<std::string> disabled(c.disabled.begin(), c.disabled.end());
    std::ranges::sort(disabled);
    root["disabledDiseases"] = disabled;

    std::error_code ec;
    std::filesystem::create_directories(Utils::ConfigFolder(), ec);
    std::ofstream file(ConfigPath(), std::ios::trunc);
    if (!file) {
        logger::error("Could not write {}", ConfigPath().string());
        return;
    }
    file << root.dump(2) << '\n';
    logger::info("Settings saved");
}

void Settings::LoadDefinitions() {
    const auto corePath = DefinitionsPath();
    if (!std::filesystem::exists(corePath)) {
        std::error_code ec;
        std::filesystem::create_directories(Utils::ConfigFolder(), ec);
        std::ofstream file(corePath, std::ios::trunc);
        if (file) {
            file << kDefaultDiseasesJson;
            logger::info("Wrote default {}", corePath.string());
        }
    }

    auto list = std::make_shared<DefinitionList>();
    std::size_t files = 0;
    for (const auto& path : CollectDefinitionFiles()) {
        json root;
        if (!ReadFile(path, root)) {
            if (path == corePath) {
                logger::warn("Falling back to the built-in disease definitions");
                root = json::parse(kDefaultDiseasesJson);
            } else {
                continue;
            }
        }
        const std::size_t read = ApplyFile(*list, root, path.filename().string());
        if (read > 0) {
            ++files;
            logger::info("{}: {} disease entries", path.filename().string(), read);
        }
    }
    for (auto& def : *list) {
        ResolveForms(def);
    }
    logger::info("Loaded {} disease definitions from {} file(s)", list->size(), files);
    std::scoped_lock lock(g_definitionsMutex);
    g_definitions = std::move(list);
}

std::shared_ptr<const Settings::DefinitionList> Settings::GetDefinitions() {
    std::scoped_lock lock(g_definitionsMutex);
    return g_definitions;
}
