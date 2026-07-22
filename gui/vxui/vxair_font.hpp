#ifndef VXAIR_FONT_HPP
#define VXAIR_FONT_HPP

#include <string>
#include <cstdint>

namespace Vx {
namespace Gui {

class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    bool load_font(const std::string& path);
    
    /**
     * @brief Render text onto a target buffer
     * @param text The text to render
     * @param x The X coordinate
     * @param y The Y coordinate
     * @param color ARGB color for the text
     * @param buffer Target buffer (32-bit ARGB)
     * @param buffer_width Target buffer width
     * @param buffer_height Target buffer height
     */
    void render_text(const std::string& text, int x, int y, uint32_t color, uint32_t* buffer, int buffer_width, int buffer_height);

private:
    // A simple built-in 8x8 font for fallback
    static const uint8_t s_builtin_font[256][8];
    bool use_builtin_;
};

} // namespace Gui
} // namespace Vx

#endif // VXAIR_FONT_HPP
