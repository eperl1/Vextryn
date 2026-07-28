// VXUI — Vextryn Xtensible UI Framework
// Purpose-built native widget toolkit for Vextryn Air OS.
// No STL. No Qt. No fake. Header-only.
//
// V5: VXUI now sits ABOVE the VXRender graphics layer. All drawing
// routes through vxr_* primitives instead of raw framebuffer calls.
#ifndef VXUI_HPP
#define VXUI_HPP

#include "vxui_theme.hpp"
#include "../vxrender/vxrender.hpp"

// Forward declarations for compositor text functions (still needed for char rendering).
void draw_abstract_char(int x, int y, char c, uint32_t color);
void draw_digit(int x, int y, int digit, uint32_t color);
void draw_segment(int x, int y, int len, bool horiz, uint32_t color);

// ===== Rectangle =====
struct VxRect {
    int x, y, w, h;
    bool contains(int mx, int my) const {
        return mx >= x && mx < x + w && my >= y && my < y + h;
    }
};

// V5: VXUI drawing helpers now delegate to VXRender primitives.
// These wrappers maintain backward compatibility while routing all
// rendering through the centralized graphics layer.
static inline void vxui_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    vxr_rounded_rect(x, y, w, h, radius, color);
}

static inline void vxui_draw_shadow(int x, int y, int w, int h, int depth) {
    vxr_shadow(x, y, w, h, depth);
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
            vxr_fill_rect(x, y, w, h, VxTheme::SURFACE);
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
            // Digits: glass surface, crisp text, premium rounded
            bg = is_pressed ? VxTheme::OVERLAY : VxTheme::SURFACE_HIGH;
            text_col = VxTheme::TEXT_PRIMARY;
            radius = VxTheme::RADIUS_MD;
            break;
        case VX_BTN_OPERATOR:
            // Operators: accent-tinted glass, distinct and premium
            bg = is_pressed ? VxTheme::accent_soft() : VxTheme::SURFACE_HIGH;
            text_col = VxTheme::accent_glow();
            radius = VxTheme::RADIUS_LG;
            break;
        case VX_BTN_UTILITY:
            // Utility: recessed, clearly secondary
            bg = is_pressed ? VxTheme::BORDER_SUBTLE : VxTheme::SURFACE;
            text_col = VxTheme::TEXT_SECONDARY;
            radius = VxTheme::RADIUS_SM;
            break;
        case VX_BTN_ACTION:
            // Equals: accent fill, white text — unmistakably primary
            bg = is_pressed ? VxTheme::accent_dim() : accent;
            text_col = VxTheme::TEXT_PRIMARY;
            radius = VxTheme::RADIUS_LG;
            break;
        case VX_BTN_PRIMARY:
            bg = is_pressed ? VxTheme::accent_dim() : accent;
            text_col = VxTheme::BASE_DEEP;
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

        // Focus ring — 1px accent glow
        if (is_focused) {
            vxr_fill_rect(x - 1, y - 1, w + 2, h + 2, VxTheme::accent());
        }
        // V2 Final: visible outline on EVERY button
        vxr_fill_rect(x, y, w, 1, VxTheme::BORDER_BRIGHT);
        vxr_fill_rect(x, y + h - 1, w, 1, VxTheme::BORDER_SUBTLE);
        vxr_fill_rect(x, y, 1, h, VxTheme::BORDER_SUBTLE);
        vxr_fill_rect(x + w - 1, y, 1, h, VxTheme::BORDER_SUBTLE);

        if (radius > 0) {
            vxui_draw_rounded_rect(x, y, w, h, radius, bg);
        } else {
            vxr_fill_rect(x, y, w, h, bg);
        }

        // Top highlight for digit/secondary — bright light edge
        if (variant == VX_BTN_DIGIT || variant == VX_BTN_SECONDARY) {
            vxr_fill_rect(x + 2, y + 1, w - 4, 1, VxTheme::BORDER_BRIGHT);
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
            int depth = elevation == 2 ? 8 : 5;
            vxui_draw_shadow(x, y, w, h, depth);
        }
        vxr_fill_rect(x, y, w, h, bg);
        // V2 Final: visible outlines on ALL sides
        vxr_fill_rect(x, y, w, 1, VxTheme::BORDER_BRIGHT);
        vxr_fill_rect(x, y + h - 1, w, 1, VxTheme::BORDER_STRONG);
        vxr_fill_rect(x, y, 1, h, VxTheme::BORDER_STRONG);
        vxr_fill_rect(x + w - 1, y, 1, h, VxTheme::BORDER_STRONG);
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
        uint32_t bcolor = is_focused ? accent : VxTheme::BORDER_BRIGHT;
        vxr_fill_rect(x, y, w, h, VxTheme::SURFACE);
        // V2 Final: visible outlines on all sides
        vxr_fill_rect(x, y, w, 1, bcolor);
        vxr_fill_rect(x, y + h - 1, w, 1, bcolor);
        vxr_fill_rect(x, y, 1, h, bcolor);
        vxr_fill_rect(x + w - 1, y, 1, h, bcolor);
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
