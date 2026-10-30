cmake_minimum_required(VERSION 3.21)

# DiseaseWidget.dll on CharmedBaryon's CommonLibSSE-NG (the "older" build: Skyrim VR and 1.6.1130 and older).
# Same sources as the "new" build (../src), by kuroishitam; see ../README.md.
project(DiseaseWidget VERSION 0.1.0 LANGUAGES CXX)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} NAME "DiseaseWidget" AUTHOR "kuroishitam" SOURCES
    ../src/plugin.cpp ../src/Events.cpp ../src/Hooks.cpp ../src/Hud.cpp ../src/MCP.cpp
    ../src/Manager.cpp ../src/Serialization.cpp ../src/Settings.cpp ../src/Utils.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE IS_HOST_PLUGIN DW_LEGACY_LIB)
target_include_directories(${PROJECT_NAME} PRIVATE ../include)
target_precompile_headers(${PROJECT_NAME} PRIVATE ../include/PCH.h)
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/Z7>)
target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)
