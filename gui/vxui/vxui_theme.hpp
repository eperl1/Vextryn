// VXUI Theme — Design tokens for Vextryn Air's native UI framework.
// Style correction: shifted from cold/technical to warm/premium.
// No STL, header-only, direct framebuffer rendering.
#ifndef VXUI_THEME_HPP
#define VXUI_THEME_HPP

#include <stdint.h>

// ===== Color Palette — Warm Premium =====
// Shifted from cold blue-black to warm graphite. Surfaces have subtle
// warmth. Accent is a refined teal, not neon sky blue. Borders are
// gentle, not sharp. The whole system feels livable, not analytical.
namespace VxTheme {

// Background — warm graphite, not cold blue-black
constexpr uint32_t BASE_DEEP     = 0xFF131316;  // Warm near-black
constexpr uint32_t BASE_DARK     = 0xFF1A1A1F;  // Desktop — warm dark
constexpr uint32_t SURFACE       = 0xFF222228;  // Cards/panels — soft warm grey
constexpr uint32_t SURFACE_HIGH  = 0xFF2C2C33;  // Raised surfaces — lighter warm
constexpr uint32_t OVERLAY       = 0xFF36363E;  // Hover — gentle warm lift

// Accent — refined teal, not neon. Calm, confident, premium.
constexpr uint32_t ACCENT        = 0xFF4A9B8F;  // Muted teal — premium, not loud
constexpr uint32_t ACCENT_DIM    = 0xFF3A7A70;  // Pressed — deeper teal
constexpr uint32_t ACCENT_GLOW   = 0xFF6BBFAE;  // Hover glow — soft teal lift

// Semantic — warmer tones
constexpr uint32_t SUCCESS       = 0xFF5BAA75;  // Soft green
constexpr uint32_t DANGER        = 0xFFC8555F;  // Muted coral red
constexpr uint32_t WARNING       = 0xFFD4A04A;  // Warm amber

// Text — warm off-white, not cold blue-white
constexpr uint32_t TEXT_PRIMARY  = 0xFFE8E6E3;  // Warm off-white
constexpr uint32_t TEXT_SECONDARY = 0xFF9A9690;  // Warm grey
constexpr uint32_t TEXT_MUTED    = 0xFF5C5955;   // Warm dark grey

// Borders — gentle, not sharp
constexpr uint32_t BORDER_SUBTLE = 0xFF33333A;   // Soft border
constexpr uint32_t BORDER_STRONG = 0xFF44444E;   // Slightly stronger

// ===== Spacing Scale — more breathing room =====
constexpr int SP_XS   = 6;   // Was 4 — more generous
constexpr int SP_SM   = 10;  // Was 8
constexpr int SP_MD   = 16;  // Was 12 — significant breathing room
constexpr int SP_LG   = 20;  // Was 16
constexpr int SP_XL   = 28;  // Was 20
constexpr int SP_2XL  = 36;  // Was 24
constexpr int SP_3XL  = 44;  // Was 32
constexpr int SP_4XL  = 56;  // Was 40

// ===== Typography =====
constexpr int FONT_SMALL   = 8;
constexpr int FONT_BODY    = 8;   // Must match draw_abstract_char glyph width
constexpr int FONT_LARGE   = 10;
constexpr int FONT_DISPLAY = 16;

// ===== Elevation — softer, more diffuse shadows =====
constexpr int SHADOW_FLAT     = 0;
constexpr int SHADOW_RAISED   = 28;   // Was 40 — softer
constexpr int SHADOW_FLOATING = 50;   // Was 80 — gentler

// ===== Border Radius — rounder, more approachable =====
constexpr int RADIUS_NONE    = 0;
constexpr int RADIUS_SM      = 3;    // Was 2
constexpr int RADIUS_MD      = 6;    // Was 4
constexpr int RADIUS_LG      = 10;   // Was 8
constexpr int RADIUS_FULL    = 999;

// ===== Component Sizes — slightly taller for comfort =====
constexpr int BTN_HEIGHT_SM  = 34;   // Was 32
constexpr int BTN_HEIGHT_MD  = 42;   // Was 40
constexpr int BTN_HEIGHT_LG  = 50;   // Was 48
constexpr int BTN_HEIGHT_XL  = 58;   // Was 56

constexpr int INPUT_HEIGHT   = 38;   // Was 36
constexpr int TITLE_BAR_H    = 36;   // Was 32 — more room to breathe
constexpr int TASKBAR_H      = 52;   // Was 48

} // namespace VxTheme

#endif // VXUI_THEME_HPP
