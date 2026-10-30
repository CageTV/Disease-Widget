#pragma once

#include <string>

namespace Hud {
    void Register();
    // Config pages call this every frame so the widget is drawn as a live preview while they are open.
    void MarkPreview();
    void InvalidateIcons();
    // Returns an ImTextureID (void*) or nullptr. px = intended on-screen size, used to pick the SVG raster size.
    void* GetIconTexture(const std::string& icon, float px);
}
