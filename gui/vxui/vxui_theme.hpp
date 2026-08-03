// VXUI Theme — Vextryn Air Platform Design System
// Production-grade Slate Dark Theme tokens for a serious OS experience.
#ifndef VXUI_THEME_HPP
#define VXUI_THEME_HPP

#include <stdint.h>

namespace VxTheme {

// Slate Dark Background & Surface Palette
constexpr uint32_t BASE_DEEP     = 0xFF0D1117;  // Deepest background (Desktop / Shell)
constexpr uint32_t BASE_DARK     = 0xFF161B22;  // Desktop panel / base background
constexpr uint32_t BASE_MID      = 0xFF21262D;  // Mid background
constexpr uint32_t SURFACE       = 0xFF161B22;  // Card / Window body surface
constexpr uint32_t SURFACE_HIGH  = 0xFF21262D;  // Raised container / header surface
constexpr uint32_t OVERLAY       = 0x4430363D;  // Subtle hover lift
constexpr uint32_t GLASS_TINT    = 0xEE161B22;  // Translucent frosted glass tint

// Accent Palette (Vivid Ice / Electric Blue)
constexpr uint32_t ACCENT        = 0xFF2F81F7;  // Primary action accent
constexpr uint32_t ACCENT_DIM    = 0xFF1F5FBF;  // Pressed / Active accent
constexpr uint32_t ACCENT_GLOW   = 0xFF58A6FF;  // Focus ring & highlight glow
constexpr uint32_t ACCENT_SOFT   = 0xFF183B70;  // Muted accent container background

// Semantic Colors
constexpr uint32_t SUCCESS       = 0xFF3FB950;  // Green success
constexpr uint32_t DANGER        = 0xFFF85149;  // Red warning/error
constexpr uint32_t WARNING       = 0xFFD29922;  // Amber warning
constexpr uint32_t INFO          = 0xFF58A6FF;  // Blue info

// Text Hierarchy Colors
constexpr uint32_t TEXT_PRIMARY  = 0xFFF0F6FC;  // High contrast crisp white-blue
constexpr uint32_t TEXT_SECONDARY = 0xFF8B949E;  // Subdued secondary text
constexpr uint32_t TEXT_MUTED    = 0xFF6E7681;  // Muted caption / disabled text

// Border System
constexpr uint32_t BORDER_SUBTLE = 0xFF21262D;  // Quiet container divider
constexpr uint32_t BORDER_STRONG = 0xFF30363D;  // Strong control / window edge
constexpr uint32_t BORDER_ACCENT = 0xFF2F81F7;  // Active focus border
constexpr uint32_t BORDER_BRIGHT = 0xFF363B42;  // Highlight top border edge

// Spacing Scale (4px grid system)
constexpr int SP_XS   = 4;
constexpr int SP_SM   = 8;
constexpr int SP_MD   = 12;
constexpr int SP_LG   = 16;
constexpr int SP_XL   = 24;
constexpr int SP_2XL  = 32;
constexpr int SP_3XL  = 48;
constexpr int SP_4XL  = 64;

// Typography Scale
constexpr int FONT_CAPTION = 10;
constexpr int FONT_BODY    = 12;
constexpr int FONT_LARGE   = 14;
constexpr int FONT_HEADING = 16;
constexpr int FONT_DISPLAY = 20;

// Elevation & Depth
constexpr int SHADOW_FLAT     = 0;
constexpr int SHADOW_RAISED   = 4;
constexpr int SHADOW_FLOATING = 12;
constexpr int SHADOW_ELEVATED = 20;

// Radii
constexpr int RADIUS_NONE    = 0;
constexpr int RADIUS_SM      = 4;
constexpr int RADIUS_MD      = 8;
constexpr int RADIUS_LG      = 12;
constexpr int RADIUS_FULL    = 999;

// Shell Component Dimensions
constexpr int BTN_HEIGHT_SM  = 28;
constexpr int BTN_HEIGHT_MD  = 34;
constexpr int BTN_HEIGHT_LG  = 42;

constexpr int INPUT_HEIGHT   = 34;
constexpr int TITLE_BAR_H    = 36;
constexpr int TASKBAR_H      = 48;
constexpr int TOPBAR_H       = 28;

// Runtime Accent & Theme Helpers
static uint32_t g_accent = ACCENT;

inline void set_accent(uint32_t c) { g_accent = c; }
inline uint32_t accent() { return g_accent; }

inline uint8_t r_of(uint32_t c) { return (c >> 16) & 0xFF; }
inline uint8_t g_of(uint32_t c) { return (c >> 8)  & 0xFF; }
inline uint8_t b_of(uint32_t c) { return  c        & 0xFF; }

inline uint32_t mix_channel(uint8_t a, uint8_t b, uint32_t t, uint32_t max_t) {
    if (max_t == 0) return a;
    return (a * (max_t - t) + b * t) / max_t;
}
inline uint32_t mix_color(uint32_t c1, uint32_t c2, uint32_t t, uint32_t max_t) {
    uint8_t r = (uint8_t)mix_channel(r_of(c1), r_of(c2), t, max_t);
    uint8_t g = (uint8_t)mix_channel(g_of(c1), g_of(c2), t, max_t);
    uint8_t b = (uint8_t)mix_channel(b_of(c1), b_of(c2), t, max_t);
    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

inline uint32_t accent_dim()  { return mix_color(g_accent, BASE_DEEP, 40, 100); }
inline uint32_t accent_glow() { return mix_color(g_accent, TEXT_PRIMARY, 30, 100); }
inline uint32_t accent_soft() { return mix_color(g_accent, BASE_DEEP, 70, 100); }

} // namespace VxTheme

#endif // VXUI_THEME_HPP
