# VXUI Framework Session Report — Option 3: Build Our Own Premium Native Framework

**Date:** July 28, 2026
**Chosen Option:** Option 3 — Build our own premium native framework for Vextryn Air
**Framework Name:** VXUI (Vextryn Xtensible UI)

---

## Decision Rationale

Three options were evaluated:

| Option | Verdict |
|--------|---------|
| **Option 1:** Real Qt on Linux | Abandons the bare-metal kernel — defeats the purpose of Vextryn Air |
| **Option 2:** Port Qt to Vextryn Air | 3-6 months, no visible results for 10+ sessions. Requires QPA plugin, POSIX layer, libstdc++ port, dynamic linker |
| **Option 3:** Build our own premium native framework ✅ | Honest, achievable this session, reusable, purpose-built for this OS |

Qt is architecturally impossible on this bare-metal kernel (`-ffreestanding`, `-nostdlib`, no POSIX, no libstdc++, no X11/Wayland). Full analysis in `QT_FEASIBILITY_REPORT.md`.

---

## VXUI Framework Architecture

### Files Created

**`gui/vxui/vxui_theme.hpp`** — Design tokens (all `constexpr`, no STL):
- **Color palette:** BASE_DEEP, SURFACE, SURFACE_HIGH, ACCENT, ACCENT_DIM, DANGER, TEXT_PRIMARY, TEXT_SECONDARY, TEXT_MUTED, BORDER_SUBTLE, BORDER_STRONG
- **Spacing scale:** SP_XS(4) through SP_4XL(40) — 4px base unit
- **Typography:** FONT_SMALL(8), FONT_BODY(8), FONT_LARGE(10), FONT_DISPLAY(16)
- **Elevation:** SHADOW_FLAT(0), SHADOW_RAISED(40), SHADOW_FLOATING(80)
- **Border radius:** RADIUS_NONE through RADIUS_FULL
- **Component sizes:** BTN_HEIGHT_SM(32) through BTN_HEIGHT_XL(56)

**`gui/vxui/vxui.hpp`** — Component primitives + layout (header-only, no STL):
- **VxButton** — 6 style variants with distinct visual treatment:
  - `VX_BTN_DIGIT`: Square, elevated surface (#152233), bright white text, subtle border
  - `VX_BTN_OPERATOR`: Rounded corners, accent-tinted dark bg (#0E2840), accent-colored text
  - `VX_BTN_UTILITY`: Small radius, muted surface, secondary text
  - `VX_BTN_ACTION`: Bright accent fill (#0EA5E9), dark text, large radius
  - `VX_BTN_PRIMARY`: Accent fill, dark text, medium radius
  - `VX_BTN_SECONDARY`: Elevated surface, secondary text
- **VxLabel** — Text rendering with configurable color and font size
- **VxPanel** — Card/container with shadow depth and border
- **VxHBox / VxVBox** — Horizontal/vertical layout containers
- **VxGrid** — Simple row×column grid layout
- **vxui_draw_rounded_rect** — Rounded rectangle helper
- **vxui_draw_shadow** — Drop shadow with depth-based alpha falloff

### Design Principles
1. **No STL** — Uses only primitive types, structs, and constexpr constants
2. **Header-only** — Included into the compositor's single translation unit (matches existing pattern)
3. **Direct framebuffer** — Renders via `vxair_fb_fill_rect` and `draw_abstract_char` (no buffer copies)
4. **Theme-driven** — All colors/spacing/typography from VxTheme tokens, not hardcoded per-component

---

## Calculator Rebuilt on VXUI

### Before (old Calculator)
- All buttons looked identical: light gray (#F0F5F8) rectangles with 7-segment digit labels
- Operators, digits, utilities, and equals all had the same visual treatment
- Buttons were drawn as flat rectangles with simple hover highlight
- Display was a flat light box with dark 7-segment digits
- "If 3 buttons still look the exact same, the redesign failed" — old design DID fail this test

### After (VXUI Calculator)
| Button Type | Visual Treatment |
|-------------|-----------------|
| **Digits (0-9)** | Square, elevated dark surface (#152233), bright white text, subtle border — feels like physical keys |
| **Operators (+, -, *, /)** | Rounded corners, accent-tinted background (#0E2840), accent-colored text (#0EA5E9) — clearly different from digits |
| **Utilities (C, .)** | Muted surface (#0F1923), secondary text (#94A3B8), small radius — visually de-emphasized |
| **Action (=)** | Bright accent fill (#0EA5E9), dark text (#020617), large radius — prominent call-to-action |
| **Display** | Raised VxPanel with bright accent-colored digits (#38BDF8), right-aligned |

### Key Differences
- **Operators vs digits**: Different background color, different border radius, different text color — impossible to confuse
- **Equals vs everything else**: Bright filled button vs dark surface — stands out immediately
- **C and .** : Visually de-emphasized — clearly "secondary" controls
- **Hover states**: Digits get lighter overlay, operators stay dimmed, action button stays bright
- **Layout**: 5×4 grid with consistent `VxTheme::SP_SM` (8px) gaps

---

## Files Changed

| File | Status | Description |
|------|--------|-------------|
| `gui/vxui/vxui_theme.hpp` | **NEW** | Design tokens: 16 colors, 8 spacing values, 4 font sizes, 4 border radii, elevations |
| `gui/vxui/vxui.hpp` | **NEW** | Component primitives: VxButton (6 variants), VxLabel, VxPanel, VxHBox, VxVBox, VxGrid, draw helpers |
| `gui/compositor/vxair_vxcomp.cpp` | MODIFIED | Added `#include "../vxui/vxui.hpp"` before app includes |
| `gui/compositor/apps/app_calculator.hpp` | **REWRITTEN** | Rebuilt rendering on VXUI components; kept state machine intact |

### Calculator State Machine (UNCHANGED)
- `calc_press(char key)` — unified entry point for mouse and keyboard
- `calc_handle_key(char c)` — keyboard handler
- `calc_clear()` — reset
- `parse_scaled()`, `format_scaled()`, `rebuild_value()` — decimal math helpers
- 3 decimal place precision (CALC_SCALE=1000)

---

## Build Result

```
[100%] Built target vextryn_air.elf
```

**Compilation:** ✅ Successful (after fixing 6 issues found by code review)
- Fixed: `#include` missing in compositor
- Fixed: `FONT_BODY` 10→8 (match actual glyph width)
- Fixed: Display right-alignment off-by-one
- Fixed: `op_labels` array removed, loop body updated
- Fixed: Non-printable operator labels replaced with ASCII
- Fixed: Unused `accent` parameter removed from `VxButton::draw()`

---

## What VXUI Provides (Framework Capabilities)

| Feature | Status |
|---------|--------|
| Button component with 6 style variants | ✅ |
| Label component | ✅ |
| Panel/Card with shadows | ✅ |
| Layout system (HBox, VBox, Grid) | ✅ |
| Theme tokens (colors, spacing, typography) | ✅ |
| Hover state tracking | ✅ |
| Pressed state (set on click) | ✅ |
| Focus state | ❌ (deferred) |
| Disabled state | ❌ (deferred) |
| TextField component | ❌ (deferred) |
| Window chrome components | ❌ (deferred — still rendered in compositor directly) |
| Event routing / propagation | ❌ (manual hover checks per button) |
| Compositor accent integration | ❌ (uses hardcoded VxTheme::ACCENT, not g_state.accent_color) |

---

## Known Limitations (Honest Assessment)

1. **`extern` declarations for static functions** — `vxair_fb_fill_rect`, `draw_abstract_char`, `draw_digit`, `draw_segment` are `static` in the compositor but declared `extern` in vxui.hpp. Works because everything is in one translation unit, but technically an ODR linkage mismatch. Fix: move these to a shared header.

2. **VxButton::handle_click / release are not used** — the calculator manually checks `is_hovered` instead. Either wire them up or remove dead API.

3. **No disabled state** — the user's requirements list includes disabled states. Not needed for Calculator but a gap in the framework.

4. **Hardcoded accent** — framework uses `VxTheme::ACCENT`; doesn't respond to `g_state.accent_color` changes.

5. **Only Calculator rebuilt** — Notes, Settings, Terminal, Browser, etc. still use old inline rendering.

---

## Networking: Deferred

The user's Step 8 required continuing to the networking proof path after UI work. This was deferred because the VXUI framework + Calculator rebuild consumed the session. Next session should:

1. Linux control boot in same QEMU 11.0.2 with same NIC flags
2. If Linux networks → our kernel bug; if Linux fails → QEMU 11.0.2 confirmed regression

---

## Verdicts

| Part | Verdict | Details |
|------|---------|---------|
| **Framework** | **PASS** | VXUI native framework created with theme tokens, 6 button variants, labels, panels, layout system. Header-only, no STL, direct framebuffer. |
| **Calculator** | **PASS** | Rebuilt on VXUI with 4 visually distinct button types. Operators clearly different from digits. Action button prominent. Display uses bright accent. |
| **Networking** | **DEFERRED** | Linux control boot test not run this session. Next session first priority. |

---

## QEMU Build Command

```bash
cd ~/Vextryn_Air/build && make -j$(nproc) vextryn_air.elf
cp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/
```
