#include "Utils.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>

double Utils::NowSeconds() {
    using Clock = std::chrono::steady_clock;
    static const auto start = Clock::now();
    return std::chrono::duration<double>(Clock::now() - start).count();
}

std::string Utils::ToLower(std::string_view text) {
    std::string out(text);
    std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

bool Utils::ParseColor(std::string_view text, Color& out) {
    if (!text.empty() && text.front() == '#') {
        text.remove_prefix(1);
    }
    if (text.size() != 6 && text.size() != 8) {
        return false;
    }
    Color parsed{0.0f, 0.0f, 0.0f, 1.0f};
    for (std::size_t i = 0; i < text.size() / 2; ++i) {
        const char* first = text.data() + i * 2;
        const char* last = first + 2;
        unsigned int value = 0;
        const auto [ptr, ec] = std::from_chars(first, last, value, 16);
        if (ec != std::errc{} || ptr != last) {
            return false;
        }
        parsed[i] = static_cast<float>(value) / 255.0f;
    }
    out = parsed;
    return true;
}

std::string Utils::FormatColor(const Color& color) {
    auto to_byte = [](float v) { return static_cast<unsigned>(std::lround(std::clamp(v, 0.0f, 1.0f) * 255.0f)); };
    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X%02X", to_byte(color[0]), to_byte(color[1]), to_byte(color[2]),
                  to_byte(color[3]));
    return buffer;
}

std::filesystem::path Utils::ConfigFolder() { return std::filesystem::path("Data") / "SKSE" / "Plugins" / "DiseaseWidget"; }
