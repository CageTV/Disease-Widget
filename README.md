# Disease Widget (SKSE)

Shows an on-screen icon (and optional name) for every disease active on the player. Configured in-game through
**SKSE Menu Framework** (section "Disease Widget"). Built from SKSE_template_QY.

## Build
Same as the template: `COMMONLIB_SSE_FOLDER`, `VCPKG_ROOT` set, configure with a preset, build. Needs CMake 3.21+ and a clone of [CommonLibSSE-NG](https://github.com/alandtse/CommonLibSSE-NG) (`ng` branch, v12.x). The DLL is Address Library based and covers SE 1.5.97, AE 1.6.x and AE 1.7.x (including 1.7.104); on 1.7.104 players need the matching Address Library build and an SKSE / SKSE Menu Framework build for that version.
New vcpkg dependency: `nlohmann-json`. `SKSEMenuFramework.h` is bundled in `include/` (taken from SKSE-Menu-Framework-3).
Post-build copies the `Data/` folder (icons + default JSON) to your output mod folder.

## Files
- `Data/Interface/DiseaseWidget/Icons/*.svg|png` - 15 disease glyphs + `generic`. White artwork, tinted at runtime.
- `Data/SKSE/Plugins/DiseaseWidget/Diseases.json` - which spell name/form shows which icon (ships with Skyrim.esm, Survival Mode CC and Zombies CC form IDs; add modded diseases here).
- `Data/SKSE/Plugins/DiseaseWidget/Settings.json` - written by the plugin (auto-saved when the menu closes).

## Detection
Every 200 ms (on the game thread) the player's active effects are scanned. A spell shows if it matches a `Diseases.json`
entry (form or exact name) or is of type *Disease* (then it gets the generic icon unless disabled in the menu).

## Definition files from other mods
Besides `Diseases.json`, every `*.json` in `Data/SKSE/Plugins/DiseaseWidget/` (subfolders included, `Settings.json` excluded) is read, alphabetically, after it.
A mod ships e.g. `Data/SKSE/Plugins/DiseaseWidget/MyMod_Diseases.json`. An entry whose `id` already exists is merged (its `match` names and `forms` are added), a new `id` creates a new disease.
See `Example_ModDiseases.json.example`. Use "Reload Diseases.json" in the menu after changing files; the Diseases page shows which file each entry came from.
