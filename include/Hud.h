#pragma once

#include <string>

namespace Hud {
    void Register();
    // Config pages call this every frame so the widget is drawn as a live preview while they are open.
    void MarkPreview();
    // While the config page is open the mouse can move the widget (click it on the screen and drag). Locks itself again when the page closes.
    bool& Unlocked();
    // True once after the widget was moved with the mouse, so the page can mark the settings as changed.
    bool TakeMoved();
    void InvalidateIcons();
    // Returns an ImTextureID (void*) or nullptr. px = intended on-screen size, used to pick the SVG raster size.
    void* GetIconTexture(const std::string& icon, float px);
}
