#pragma once

// Generated from Data/SKSE/Plugins/DiseaseWidget/Diseases.json - written to disk when the file is missing.
inline constexpr const char* kDefaultDiseasesJson =
R"DWJSON({
  "_readme": [
    "Disease definitions for Disease Widget. Edit freely; use the 'Reload' button in the in-game menu to apply changes.",
    "Every other *.json in this folder (subfolders included, except Settings.json) is loaded after this file, so mods can ship their own definitions. Same id = merged.",
    "match: spell display names (case-insensitive, exact) that should show this icon.",
    "forms: optional 'Plugin.esp|0xLocalFormID' entries, most reliable for modded diseases.",
    "requireDiseaseType: true = only match spells whose type is 'Disease' (avoids name collisions with unrelated effects).",
    "color: optional tint used when 'Colour icons by disease' is enabled. Any spell of type Disease with no entry gets the generic icon."
  ],
  "diseases": [
    {
      "id": "ataxia",
      "name": "Ataxia",
      "icon": "ataxia",
      "match": [
        "Ataxia"
      ],
      "forms": [
        "Skyrim.esm|0xB877C",
        "ccQDRSSE001-SurvivalMode.esl|0x992",
        "ccQDRSSE001-SurvivalMode.esl|0x993"
      ],
      "requireDiseaseType": true,
      "color": "#D9C58E"
    },
    {
      "id": "bone_break_fever",
      "name": "Bone Break Fever",
      "icon": "bone_break_fever",
      "match": [
        "Bone Break Fever"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x830",
        "ccQDRSSE001-SurvivalMode.esl|0x984",
        "ccQDRSSE001-SurvivalMode.esl|0x985"
      ],
      "requireDiseaseType": true,
      "color": "#E39A5A"
    },
    {
      "id": "brain_rot",
      "name": "Brain Rot",
      "icon": "brain_rot",
      "match": [
        "Brain Rot"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x82E",
        "ccQDRSSE001-SurvivalMode.esl|0x990",
        "ccQDRSSE001-SurvivalMode.esl|0x991",
        "ccbgssse003-zombies.esl|0x872"
      ],
      "requireDiseaseType": true,
      "color": "#A8B67A"
    },
    {
      "id": "brown_rot",
      "name": "Brown Rot",
      "icon": "brown_rot",
      "match": [
        "Brown Rot"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x914",
        "ccQDRSSE001-SurvivalMode.esl|0x98B",
        "ccQDRSSE001-SurvivalMode.esl|0x986"
      ],
      "requireDiseaseType": true,
      "color": "#B58A5E"
    },
    {
      "id": "droops",
      "name": "Droops",
      "icon": "droops",
      "match": [
        "Droops"
      ],
      "forms": [
        "Dragonborn.esm|0x0285C1",
        "ccQDRSSE001-SurvivalMode.esl|0x996",
        "ccQDRSSE001-SurvivalMode.esl|0x997"
      ],
      "requireDiseaseType": true,
      "color": "#93A8BD"
    },
    {
      "id": "rattles",
      "name": "Rattles",
      "icon": "rattles",
      "match": [
        "Rattles"
      ],
      "forms": [
        "Skyrim.esm|0xB8781",
        "ccQDRSSE001-SurvivalMode.esl|0x98C",
        "ccQDRSSE001-SurvivalMode.esl|0x98D"
      ],
      "requireDiseaseType": true,
      "color": "#D3CFC0"
    },
    {
      "id": "rockjoint",
      "name": "Rockjoint",
      "icon": "rockjoint",
      "match": [
        "Rockjoint"
      ],
      "forms": [
        "Skyrim.esm|0xB8782",
        "ccQDRSSE001-SurvivalMode.esl|0x98E",
        "ccQDRSSE001-SurvivalMode.esl|0x98F"
      ],
      "requireDiseaseType": true,
      "color": "#9CD0D6"
    },
    {
      "id": "sanguinare_vampiris",
      "name": "Sanguinare Vampiris",
      "icon": "sanguinare_vampiris",
      "match": [
        "Sanguinare Vampiris"
      ],
      "forms": [
        "Skyrim.esm|0xB8780",
        "Dawnguard.esm|0x0037E9"
      ],
      "requireDiseaseType": true,
      "color": "#B8434A"
    },
    {
      "id": "witbane",
      "name": "Witbane",
      "icon": "witbane",
      "match": [
        "Witbane"
      ],
      "forms": [
        "Skyrim.esm|0xB8783",
        "ccQDRSSE001-SurvivalMode.esl|0x994",
        "ccQDRSSE001-SurvivalMode.esl|0x995"
      ],
      "requireDiseaseType": true,
      "color": "#86B6E6"
    },
    {
      "id": "food_poisoning",
      "name": "Food Poisoning",
      "icon": "food_poisoning",
      "match": [
        "Food Poisoning"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x918"
      ],
      "requireDiseaseType": false,
      "color": "#A9BB68"
    },
    {
      "id": "greenspore",
      "name": "Greenspore",
      "icon": "greenspore",
      "match": [
        "Greenspore",
        "Green Spore"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x912",
        "ccQDRSSE001-SurvivalMode.esl|0x987",
        "ccQDRSSE001-SurvivalMode.esl|0x988"
      ],
      "requireDiseaseType": false,
      "color": "#7DBA6B"
    },
    {
      "id": "gutworm",
      "name": "Gutworm",
      "icon": "gutworm",
      "match": [
        "Gutworm",
        "Gut Worm"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x915",
        "ccQDRSSE001-SurvivalMode.esl|0x989",
        "ccQDRSSE001-SurvivalMode.esl|0x98A"
      ],
      "requireDiseaseType": false,
      "color": "#CBA57C"
    },
    {
      "id": "addled",
      "name": "Addled",
      "icon": "addled",
      "match": [
        "Addled"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x911"
      ],
      "requireDiseaseType": false,
      "color": "#B9A0DA"
    },
    {
      "id": "frostbitten",
      "name": "Frostbitten",
      "icon": "frostbitten",
      "match": [
        "Frostbitten",
        "Frostbite"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x913"
      ],
      "requireDiseaseType": false,
      "color": "#93CDEB"
    },
    {
      "id": "weakened",
      "name": "Weakened",
      "icon": "weakened",
      "match": [
        "Weakened"
      ],
      "forms": [
        "ccQDRSSE001-SurvivalMode.esl|0x910"
      ],
      "requireDiseaseType": false,
      "color": "#B3B3AA"
    }
  ]
}
)DWJSON";
