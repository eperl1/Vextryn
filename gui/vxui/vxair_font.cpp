#include "vxair_font.hpp"

namespace Vx {
namespace Gui {

// Simple 8x8 font.
const uint8_t FontRenderer::s_builtin_font[256][8] = {
    // Dummy initialization
};

FontRenderer::FontRenderer() : use_builtin_(true) {
}

FontRenderer::~FontRenderer() {
}

bool FontRenderer::load_font(const std::string& path) {
    (void)path;
    use_builtin_ = true;
    return true; // We always fall back to built-in for now.
}

void FontRenderer::render_text(const std::string& text, int x, int y, uint32_t color, uint32_t* buffer, int buffer_width, int buffer_height) {
    if (!buffer) return;

    int cursor_x = x;
    int cursor_y = y;

    for (char c : text) {
        if (c == '\n') {
            cursor_y += 8;
            cursor_x = x;
            continue;
        }
        
        // Render 8x8 glyph (using an outline box as placeholder)
        for (int gy = 0; gy < 8; ++gy) {
            uint8_t row = 0xFF; // Solid block for simplicity
            
            if (gy == 0 || gy == 7) row = 0xFF; // Top/bottom edge
            else row = 0x81; // Left/right edge

            for (int gx = 0; gx < 8; ++gx) {
                if (row & (1 << (7 - gx))) {
                    int px = cursor_x + gx;
                    int py = cursor_y + gy;
                    if (px >= 0 && px < buffer_width && py >= 0 && py < buffer_height) {
                        buffer[py * buffer_width + px] = color;
                    }
                }
            }
        }
        cursor_x += 8;
    }
}

} // namespace Gui
} // namespace Vx
