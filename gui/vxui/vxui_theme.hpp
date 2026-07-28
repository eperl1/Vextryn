// VXUI Theme — VAir OS V4 Premium Design System
// Cool, glassy, premium.  Lighter surfaces, refined accents, calmer contrast.
#ifndef VXUI_THEME_HPP
#define VXUI_THEME_HPP

#include <stdint.h>

namespace VxTheme {

// Background — softer and cooler, less harsh
constexpr uint32_t BASE_DEEP     = 0xFFE0EAF5;  // Deepest background
constexpr uint32_t BASE_DARK     = 0xFFE8F0F8;  // Desktop base
constexpr uint32_t BASE_MID      = 0xFFF0F5FA;  // Mid background
constexpr uint32_t SURFACE       = 0xFFFFFFFF;  // Card surface
constexpr uint32_t SURFACE_HIGH  = 0xFFF8FAFC;  // Raised surface
constexpr uint32_t OVERLAY       = 0x333B8CFF;  // Subtle blue hover lift (transparent)
constexpr uint32_t GLASS_TINT    = 0xDDF0F5FA;  // Real translucent frosted glass

// Accent — electric ice blue, premium and vivid
constexpr uint32_t ACCENT        = 0xFF3B8CFF;
constexpr uint32_t ACCENT_DIM    = 0xFF2568CC;
constexpr uint32_t ACCENT_GLOW   = 0xFF6BA8FF;
constexpr uint32_t ACCENT_SOFT   = 0xFF1E3A66;

// Semantic — bright and clear
constexpr uint32_t SUCCESS       = 0xFF34D399;
constexpr uint32_t DANGER        = 0xFFFB7185;
constexpr uint32_t WARNING       = 0xFFFBBF24;

// Text — crisp, readable
constexpr uint32_t TEXT_PRIMARY  = 0xFF101B2B;
constexpr uint32_t TEXT_SECONDARY = 0xFF4A5568;
constexpr uint32_t TEXT_MUTED    = 0xFF718096;

// Borders — refined, not harsh
constexpr uint32_t BORDER_SUBTLE = 0xFFD2DCE6;
constexpr uint32_t BORDER_STRONG = 0xFFB0C0D4;
constexpr uint32_t BORDER_ACCENT = 0xFF3B8CFF;
constexpr uint32_t BORDER_BRIGHT = 0xFFE2E8F0;

// Spacing
constexpr int SP_XS   = 6;
constexpr int SP_SM   = 10;
constexpr int SP_MD   = 16;
constexpr int SP_LG   = 22;
constexpr int SP_XL   = 30;
constexpr int SP_2XL  = 40;
constexpr int SP_3XL  = 52;
constexpr int SP_4XL  = 64;

// Typography
constexpr int FONT_SMALL   = 8;
constexpr int FONT_BODY    = 8;
constexpr int FONT_LARGE   = 10;
constexpr int FONT_DISPLAY = 16;

// Elevation
constexpr int SHADOW_FLAT     = 0;
constexpr int SHADOW_RAISED   = 40;
constexpr int SHADOW_FLOATING = 70;

// Radius
constexpr int RADIUS_NONE    = 0;
constexpr int RADIUS_SM      = 4;
constexpr int RADIUS_MD      = 8;
constexpr int RADIUS_LG      = 12;
constexpr int RADIUS_FULL    = 999;

// Component Sizes
constexpr int BTN_HEIGHT_SM  = 36;
constexpr int BTN_HEIGHT_MD  = 44;
constexpr int BTN_HEIGHT_LG  = 52;
constexpr int BTN_HEIGHT_XL  = 60;

constexpr int INPUT_HEIGHT   = 40;
constexpr int TITLE_BAR_H    = 38;
constexpr int TASKBAR_H      = 56;
constexpr int TOPBAR_H       = 28;

// ===== Runtime accent state and helpers =====
static uint32_t g_accent = ACCENT;

inline void set_accent(uint32_t c) { g_accent = c; }
inline uint32_t accent() { return g_accent; }

// Extract RGB channels from a packed ARGB color
inline uint8_t r_of(uint32_t c) { return (c >> 16) & 0xFF; }
inline uint8_t g_of(uint32_t c) { return (c >> 8)  & 0xFF; }
inline uint8_t b_of(uint32_t c) { return  c        & 0xFF; }

// Linear interpolation between two colors (kept simple for no-FPU use)
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

// Derive tints/shades from the current runtime accent
inline uint32_t accent_dim()  { return mix_color(g_accent, BASE_DEEP, 40, 100); }
inline uint32_t accent_glow() { return mix_color(g_accent, TEXT_PRIMARY, 30, 100); }
inline uint32_t accent_soft() { return mix_color(g_accent, BASE_DEEP, 70, 100); }

} // namespace VxTheme

#endif // VXUI_THEME_HPP
