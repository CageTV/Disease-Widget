#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <string_view>

namespace Utils {
    // RGBA, each channel 0..1
    using Color = std::array<float, 4>;

    // Monotonic seconds since the first call (render-thread and game-thread safe).
    double NowSeconds();

    std::string ToLower(std::string_view text);

    // Accepts "#RRGGBB", "#RRGGBBAA" (leading '#' optional).
    bool ParseColor(std::string_view text, Color& out);
    std::string FormatColor(const Color& color);

    // Data/SKSE/Plugins/DiseaseWidget (relative to the game folder, resolved by MO2's VFS too).
    std::filesystem::path ConfigFolder();
}
