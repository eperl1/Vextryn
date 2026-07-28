// VXUI — Vextryn Xtensible UI Framework
// Purpose-built native widget toolkit for Vextryn Air OS.
// No STL. No Qt. No fake. Header-only. Direct framebuffer rendering.
#ifndef VXUI_HPP
#define VXUI_HPP

#include "vxui_theme.hpp"

// Forward declarations for compositor functions. These are static in the
// compositor but visible because vxui.hpp and app headers are all #included
// into the compositor's single translation unit. Using plain declarations
// (no extern/static) avoids ODR linkage mismatches — the compiler resolves
// to the static definitions in the same TU.
void vxair_fb_fill_rect(int x, int y, int w, int h, uint32_t color);
void draw_abstract_char(int x, int y, char c, uint32_t color);
void draw_digit(int x, int y, int digit, uint32_t color);
void draw_segment(int x, int y, int len, bool horiz, uint32_t color);

// ===== Global accent — set by compositor once, used by all components =====
namespace VxTheme {
    // Runtime accent — compositor sets this from g_state.accent_color at boot.
    // Components read it for dynamic theming. Falls back to ACCENT if not set.
    static uint32_t g_accent = ACCENT;

    inline void set_accent(uint32_t c) { g_accent = c; }
    inline uint32_t accent() { return g_accent; }
}

// ===== Rectangle =====
struct VxRect {
    int x, y, w, h;
    bool contains(int mx, int my) const {
        return mx >= x && mx < x + w && my >= y && my < y + h;
    }
};

// ===== Utility: draw rounded rectangle corners =====
static inline void vxui_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    vxair_fb_fill_rect(x + radius, y, w - radius * 2, h, color);
    vxair_fb_fill_rect(x, y + radius, w, h - radius * 2, color);
    if (radius >= VxTheme::RADIUS_MD) {
        vxair_fb_fill_rect(x, y, radius, radius, color);
        vxair_fb_fill_rect(x + w - radius, y, radius, radius, color);
        vxair_fb_fill_rect(x, y + h - radius, radius, radius, color);
        vxair_fb_fill_rect(x + w - radius, y + h - radius, radius, radius, color);
    }
}

// ===== Shadow helper =====
static inline void vxui_draw_shadow(int x, int y, int w, int h, int depth) {
    // Softer, more diffuse shadow — lower alpha, wider spread
    for (int s = 0; s < depth; s++) {
        uint32_t a = (depth - s) * (depth - s) * 2;  // Was *3 — gentler
        if (a > 100) a = 100;  // Was 160 — much softer
        uint32_t c = 0xFF000000 | (a << 16) | (a << 8) | a;
        vxair_fb_fill_rect(x + s, y + h + s, w, 1, c);
        vxair_fb_fill_rect(x + w + s, y + s, 1, h, c);
    }
}

// ===== Button Styles =====
enum VxButtonVariant {
    VX_BTN_DIGIT,      // Number key: square, elevated surface, bright text
    VX_BTN_OPERATOR,   // + - * /: rounded, accent-tinted, distinct
    VX_BTN_UTILITY,    // C . ⌫: small, muted, different shape
    VX_BTN_ACTION,     // =: prominent, accent fill, bold
    VX_BTN_PRIMARY,    // Generic primary action
    VX_BTN_SECONDARY,  // Generic secondary
    VX_BTN_DEFAULT     // Plain clickable item (launcher, taskbar)
};

// ===== VxButton =====
struct VxButton {
    int x, y, w, h;
    const char* label;
    VxButtonVariant variant;
    bool is_hovered;
    bool is_pressed;
    bool is_focused;
    bool is_disabled;

    void draw() {
        if (is_disabled) {
            // Disabled: greyed out, no interaction
            vxair_fb_fill_rect(x, y, w, h, VxTheme::SURFACE);
            int label_len = 0; for (; label[label_len]; label_len++);
            int tx = x + (w - label_len * VxTheme::FONT_BODY) / 2;
            int ty = y + (h - 12) / 2 + 2;
            for (int i = 0; label[i]; i++)
                draw_abstract_char(tx + i * VxTheme::FONT_BODY, ty, label[i], VxTheme::TEXT_MUTED);
            return;
        }

        uint32_t bg, text_col;
        int radius = VxTheme::RADIUS_NONE;
        uint32_t accent = VxTheme::accent();

        switch (variant) {
        case VX_BTN_DIGIT:
            // Digits: warm raised surface, soft text, rounded
            bg = is_pressed ? VxTheme::OVERLAY : VxTheme::SURFACE_HIGH;
            text_col = VxTheme::TEXT_PRIMARY;
            radius = VxTheme::RADIUS_MD;
            break;
        case VX_BTN_OPERATOR:
            // Operators: subtle teal tint, not loud — calm distinctiveness
            bg = is_pressed ? 0xFF2A3F3A : 0xFF2D3530;
            text_col = VxTheme::ACCENT_GLOW;  // Soft teal text, not neon
            radius = VxTheme::RADIUS_LG;
            break;
        case VX_BTN_UTILITY:
            // Utility: muted, recessed — clearly secondary
            bg = is_pressed ? VxTheme::BORDER_SUBTLE : VxTheme::SURFACE;
            text_col = VxTheme::TEXT_SECONDARY;
            radius = VxTheme::RADIUS_SM;
            break;
        case VX_BTN_ACTION:
            // Equals: warm accent fill, dark text — primary but not screaming
            bg = is_pressed ? VxTheme::ACCENT_DIM : accent;
            text_col = 0xFF1A1A1F;  // Warm dark, not pure black
            radius = VxTheme::RADIUS_LG;
            break;
        case VX_BTN_PRIMARY:
            bg = is_pressed ? VxTheme::ACCENT_DIM : accent;
            text_col = 0xFF1A1A1F;
            radius = VxTheme::RADIUS_MD;
            break;
        case VX_BTN_DEFAULT:
            // Default: calm, only gentle lift on hover
            bg = is_pressed ? VxTheme::OVERLAY : (is_hovered ? VxTheme::SURFACE_HIGH : VxTheme::SURFACE);
            text_col = is_hovered ? VxTheme::TEXT_PRIMARY : VxTheme::TEXT_SECONDARY;
            radius = VxTheme::RADIUS_SM;
            break;
        case VX_BTN_SECONDARY:
        default:
            bg = is_pressed ? VxTheme::OVERLAY : VxTheme::SURFACE_HIGH;
            text_col = VxTheme::TEXT_SECONDARY;
            radius = VxTheme::RADIUS_MD;
            break;
        }

        if (is_hovered && !is_pressed && variant != VX_BTN_ACTION && variant != VX_BTN_PRIMARY) {
            bg = VxTheme::OVERLAY;
        }

        // Focus ring — softer, 1px with accent glow, not harsh 2px solid
        if (is_focused) {
            vxair_fb_fill_rect(x - 1, y - 1, w + 2, h + 2, VxTheme::ACCENT_DIM);
        }

        if (radius > 0) {
            vxui_draw_rounded_rect(x, y, w, h, radius, bg);
        } else {
            vxair_fb_fill_rect(x, y, w, h, bg);
        }

        // Top highlight for digit/secondary — subtle light edge for depth
        if (variant == VX_BTN_DIGIT || variant == VX_BTN_SECONDARY) {
            vxair_fb_fill_rect(x, y, w, 1, 0xFF3A3A42);  // Gentle top highlight
        }

        int label_len = 0;
        for (; label[label_len]; label_len++);
        int tx = x + (w - label_len * VxTheme::FONT_BODY) / 2;
        int ty = y + (h - 12) / 2 + 2;
        for (int i = 0; label[i]; i++) {
            draw_abstract_char(tx + i * VxTheme::FONT_BODY, ty, label[i], text_col);
        }
    }

    bool handle_click(int mx, int my) {
        if (is_disabled) return false;
        if (VxRect{x, y, w, h}.contains(mx, my)) {
            is_pressed = true;
            return true;
        }
        return false;
    }

    void check_hover(int mx, int my) {
        is_hovered = !is_disabled && VxRect{x, y, w, h}.contains(mx, my);
    }

    void release() { is_pressed = false; }
};

// ===== VxLabel =====
struct VxLabel {
    int x, y;
    const char* text;
    uint32_t color;
    int font_size;

    void draw() {
        for (int i = 0; text[i]; i++) {
            draw_abstract_char(x + i * font_size, y, text[i], color);
        }
    }
};

// ===== VxPanel =====
struct VxPanel {
    int x, y, w, h;
    int elevation;
    uint32_t bg_color; // 0 = use VxTheme::SURFACE

    void draw() {
        uint32_t bg = bg_color ? bg_color : VxTheme::SURFACE;
        if (elevation > 0) {
            int depth = elevation == 2 ? 5 : 3;  // Was 6/4 — softer
            vxui_draw_shadow(x, y, w, h, depth);
        }
        vxair_fb_fill_rect(x, y, w, h, bg);
        // Gentle borders — only top and bottom, not all 4 sides
        vxair_fb_fill_rect(x, y, w, 1, VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(x, y + h - 1, w, 1, VxTheme::BORDER_SUBTLE);
    }

    bool contains(int mx, int my) const {
        return VxRect{x, y, w, h}.contains(mx, my);
    }
};

// ===== VxTextField (minimal — for address bars, input fields) =====
struct VxTextField {
    int x, y, w, h;
    const char* buffer;
    int buf_len;
    int caret_pos;
    bool is_focused;

    void draw() {
        uint32_t accent = VxTheme::accent();
        vxair_fb_fill_rect(x, y, w, h, VxTheme::SURFACE);
        vxair_fb_fill_rect(x, y, w, 1, is_focused ? accent : VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(x, y + h - 1, w, 1, is_focused ? accent : VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(x, y, 1, h, is_focused ? accent : VxTheme::BORDER_SUBTLE);
        vxair_fb_fill_rect(x + w - 1, y, 1, h, is_focused ? accent : VxTheme::BORDER_SUBTLE);
        // Text
        for (int i = 0; i < buf_len && buffer[i]; i++) {
            draw_abstract_char(x + 8 + i * VxTheme::FONT_BODY, y + (h - 12) / 2 + 2,
                               buffer[i], VxTheme::TEXT_PRIMARY);
        }
    }
};

// ===== Layout: HBox (horizontal row) =====
struct VxHBox {
    int x, y, spacing, next_x;
    void begin(int sx, int sy, int gap) { x = sx; y = sy; spacing = gap; next_x = sx; }
    int next(int width) { int p = next_x; next_x += width + spacing; return p; }
};

// ===== Layout: VBox (vertical column) =====
struct VxVBox {
    int x, y, spacing, next_y;
    void begin(int sx, int sy, int gap) { x = sx; y = sy; spacing = gap; next_y = sy; }
    int next(int height) { int p = next_y; next_y += height + spacing; return p; }
};

// ===== Layout: Grid =====
struct VxGrid {
    int ox, oy, cw, ch, gap, cols;
    void begin(int _ox, int _oy, int _cw, int _ch, int _g, int _cols) {
        ox = _ox; oy = _oy; cw = _cw; ch = _ch; gap = _g; cols = _cols;
    }
    void cell(int col, int row, int& out_x, int& out_y) {
        out_x = ox + col * (cw + gap);
        out_y = oy + row * (ch + gap);
    }
};

#endif // VXUI_HPP
