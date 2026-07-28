// VXUI Theme — VAir OS V2 Final Design System
// Brighter, bluer, premium. Less dark, more vibrant blue.
// Outlines on everything. Premium and amazing — better than macOS.
#ifndef VXUI_THEME_HPP
#define VXUI_THEME_HPP

#include <stdint.h>

namespace VxTheme {

// Background — brighter, less oppressively dark
constexpr uint32_t BASE_DEEP     = 0xFF0C0F1A;  // Deep navy — not black
constexpr uint32_t BASE_DARK     = 0xFF121624;  // Desktop — rich navy
constexpr uint32_t SURFACE       = 0xFF1A2030;  // Cards — blue-tinted dark
constexpr uint32_t SURFACE_HIGH  = 0xFF232C42;  // Raised — brighter blue surface
constexpr uint32_t OVERLAY       = 0xFF2D3856;  // Hover — clear blue lift
constexpr uint32_t GLASS_TINT    = 0xFF1E2640;  // Frosted glass

// Accent — vibrant electric blue, the star of the show
constexpr uint32_t ACCENT        = 0xFF2D7FF9;  // Electric blue — bold, premium
constexpr uint32_t ACCENT_DIM    = 0xFF1E5FCC;  // Pressed — deeper blue
constexpr uint32_t ACCENT_GLOW   = 0xFF5AA0FF;  // Hover glow — bright sky blue
constexpr uint32_t ACCENT_SOFT   = 0xFF1A3A6C;  // Subtle accent bg

// Semantic — bright and clear
constexpr uint32_t SUCCESS       = 0xFF22C55E;
constexpr uint32_t DANGER        = 0xFFEF4444;
constexpr uint32_t WARNING       = 0xFFF59E0B;

// Text — bright, high contrast
constexpr uint32_t TEXT_PRIMARY  = 0xFFFFFFFF;  // Pure white — maximum contrast
constexpr uint32_t TEXT_SECONDARY = 0xFFA8B2C8;  // Bright blue-grey
constexpr uint32_t TEXT_MUTED    = 0xFF5C6680;   // Muted blue-grey

// Borders — VISIBLE outlines on everything (user requested)
constexpr uint32_t BORDER_SUBTLE = 0xFF2A3450;   // Visible subtle border
constexpr uint32_t BORDER_STRONG = 0xFF3D4A6E;   // Strong visible border
constexpr uint32_t BORDER_ACCENT = 0xFF2D7FF9;   // Accent border (focused)
constexpr uint32_t BORDER_BRIGHT = 0xFF4A5A82;   // Bright outline

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
constexpr int TOPBAR_H       = 28;   // macOS-style top menu bar

} // namespace VxTheme

#endif // VXUI_THEME_HPP
