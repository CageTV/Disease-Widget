# Disease Widget - SKSE (kuroishitam) v1.1 source + the movable-widget patch (E:/WorkSpace/Disease-Widget-Movable), on alandtse's CommonLibSSE-NG.
ng_plugin(TARGET DiseaseWidget NAME DiseaseWidget VERSION 1.1.0 AUTHOR "kuroishitam" ROOT "E:/WorkSpace/Disease-Widget-Movable"
    SOURCES src/plugin.cpp src/Events.cpp src/Hooks.cpp src/Hud.cpp src/MCP.cpp src/Manager.cpp src/Serialization.cpp src/Settings.cpp src/Utils.cpp
    INCLUDES "E:/WorkSpace/Disease-Widget-Movable/include"
    PCH "E:/WorkSpace/Disease-Widget-Movable/include/PCH.h"
    DEFINES IS_HOST_PLUGIN
    LIBS nlohmann_json::nlohmann_json)
