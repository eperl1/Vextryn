# OS UI Architecture Revamp: Master Plan

## 1. Executive Summary & Diagnosis: Why the Current UI Feels Hand-Drawn

The current UI in Vextryn Air OS feels like a "hand-drawn test canvas" rather than a native operating system desktop environment. Our architectural audit reveals five core causes:

1. **Immediate Direct-to-Framebuffer Painting**:
   - Windows and apps do not render to isolated offscreen pixel surfaces (`VxSurface`). Instead, every frame redraws background, panels, title bars, text, and controls directly into a single shared global backbuffer (`backbuffer_base`).
   - Moving a window or hovering over a widget triggers direct pixel overwrites on the display backbuffer, followed by a full 1024x768x32 `memcpy` (3.14MB per frame) to the hardware frontbuffer.

2. **Monolithic State & Procedural Event Dispatch**:
   - `gui/compositor/vxair_vxcomp.cpp` contains over 1,600 lines combining low-level PS/2 mouse/keyboard port IO (`inb`/`outb`), hardcoded scancode conversion, window dragging state, app logic for 14 applications, and custom immediate drawing functions.
   - Input is not dispatched hierarchically through a widget tree; instead, global hit-test `if-else` blocks check bounding boxes against mouse coordinates.

3. **Primitive Hardcoded Bitmap Typography & Iconography**:
   - Typography relies on raw 8x8 bitmap arrays (`font8x8.h`) and custom drawn line segments (`draw_abstract_char`, `draw_digit`), lacking font metrics, kerning, clipping, or multi-line text wrapping.
   - App icons are raw 32x32 static arrays painted directly into the global framebuffer.

4. **Absence of Compositor Surface Isolation & Dirty Invalidation**:
   - There is no compositing engine. Windows do not hold backing stores; Z-order layering is simulated by re-executing procedural draw calls from back-to-front.
   - No dirty rectangle tracking (`VxDamageTracker`) exists. Any update results in redrawing the entire screen.

5. **Fragile Focus and Input State Handling**:
   - Focus is a single `int focused_window` integer in a global state struct. Individual widgets manage their own hover/press states procedurally without keyboard tab navigation, cursor caret control, or unified focus ring management.

---

## 2. Weaknesses in the Current Framebuffer & Render Path

| Architectural Area | Current Implementation | Architectural Defect | Revamp Solution |
| :--- | :--- | :--- | :--- |
| **Buffer Strategy** | Double buffer (`framebuffer_base`, `backbuffer_base`) | Whole-screen `memcpy` (3.14 MB) every frame regardless of change size. | **Dirty Region Compositing**: Only copy invalidated bounding rectangles. |
| **Surface Management** | No per-window surfaces. Windows draw into shared backbuffer. | Windows overwrite each other's pixels; no transparency or offscreen caching. | **Offscreen Window Surfaces**: Each window renders to an isolated `VxSurface`. |
| **Clipping Stack** | Single global `VxClipRect` in `g_vxr_ctx`. | Child controls cannot stack nested clipping rects safely. | **Hierarchical Clip Stack**: Push/pop clip stack per container and widget. |
| **Compositing Model** | Immediate procedural draw calls back-to-front. | No alpha blending, window shadow composition, or smooth window movement. | **Compositor Engine**: Blends window surfaces, wallpaper, shell panels, and hardware cursor. |
| **Text Rendering** | Direct pixel painting via 8x8 font bitmasks. | Hardcoded font size, missing kerning, no text selection, no alignment rules. | **Rich Text Renderer**: Sub-pixel bitmap renderer with scale, metrics, wrapping, and carets. |

---

## 3. New Graphics & Compositor Architecture

```
+-----------------------------------------------------------------------+
|                            APPLICATIONS                               |
|   (Calculator, SysMon, Notes, Settings, Terminal, Files, Browser)     |
+-----------------------------------------------------------------------+
                                   |
                                   v  (Render into Window Offscreen Surfaces)
+-----------------------------------------------------------------------+
|                         RETAINED UI FRAMEWORK                         |
|   - Widget Tree (VxWidget, VxContainer, VxButton, VxTextBox, etc.)     |
|   - Layout Engine (VxBoxLayout, VxGridLayout, Flex metrics)          |
|   - Focus & Event Routing (VxFocusManager, VxEventRouter)             |
|   - Design Tokens (VxThemeTokens, Typography Scale, Color Tokens)     |
+-----------------------------------------------------------------------+
                                   |
                                   v  (Offscreen Surface Commands)
+-----------------------------------------------------------------------+
|                         WINDOW SURFACE MANAGER                        |
|   - VxWindowSurface (Width, Height, Pitch, Backing Buffer, DirtyRects) |
+-----------------------------------------------------------------------+
                                   |
                                   v  (Compositing Pass)
+-----------------------------------------------------------------------+
|                          VXCOMPOSITOR ENGINE                          |
|   - Layer Stack (Wallpaper -> Window Surfaces -> Panels -> Cursor)     |
|   - Alpha Compositor & Drop Shadows                                   |
|   - Damage Tracking (Accumulates Dirty Rectangles across display)    |
+-----------------------------------------------------------------------+
                                   |
                                   v  (Sub-region Blit)
+-----------------------------------------------------------------------+
|                      HARDWARE FRAMEBUFFER DRIVER                      |
|   - Frontbuffer / Backbuffer Swap                                     |
|   - Dirty Rect Invalidation (Present Only Changed Pixels)             |
+-----------------------------------------------------------------------+
```

### Key Components:
1. **`VxSurface`**:
   - Represents an offscreen pixel buffer with dimensions `(w, h)`, pixel pointer `uint32_t*`, stride, and internal dirty rectangle list.
   - Applications render entirely into their owned `VxSurface` without touching the main display.

2. **`VxDamageTracker`**:
   - Maintains a bounding box list of dirty regions (`VxRect`).
   - When a widget or window updates, only its modified bounding box is marked dirty.

3. **`VxCompositor`**:
   - Responsible for layer composition: Desktop Wallpaper -> Window Surfaces (in Z-Order) -> Window Chrome & Shadows -> Taskbar/Panel -> Launcher Popover -> Toast Notifications -> Mouse Cursor.
   - Performs sub-region compositing into the display backbuffer for dirty regions only.

---

## 4. New UI Framework Architecture

The new framework is a **retained-tree component framework**:

### Core Class Hierarchy:
- **`VxNode`**: Base class for tree hierarchy (`parent`, `children`, `bounds`, `visible`, `enabled`).
- **`VxWidget`**: Base UI component:
  - `measure(constraints)`: Computes preferred width/height.
  - `layout(bounds)`: Assigns explicit position and size.
  - `render(surface, clip_rect)`: Draws widget onto window surface.
  - `on_event(event)`: Handles mouse/keyboard input.
- **`VxContainer`**: Composite widget hosting child widgets (`VxPanel`, `VxScrollView`, `VxWindowContent`).
- **`VxLayout`**: Layout manager (`VxBoxLayout` for horizontal/vertical flex stacks, `VxGridLayout` for uniform grids).

### Widget Primitives:
1. `VxButton`: Interactive button with states (Normal, Hover, Pressed, Focused, Disabled), primary/secondary/ghost variants, icon + text layout.
2. `VxLabel`: Multi-line text display with font scale, alignment (Left, Center, Right), text truncation (`...`).
3. `VxTextBox`: Single-line and multi-line text edit box with cursor caret, text selection highlight, backspace/delete, arrow navigation, placeholder text.
4. `VxPanel`: Card container with elevation shadow, rounded corners, background fill, and optional border.
5. `VxScrollView`: Scrollable area with vertical/horizontal scrollbars, touch/mouse wheel scrolling.
6. `VxSlider`: Range slider control for volume, brightness, sensitivity.
7. `VxIcon`: Vector-style bitmap icon viewer with tinting.

---

## 5. Desktop Environment Components & Responsibilities

The desktop shell is decoupled into five core surfaces:

1. **`VxDesktopSurface`**:
   - Manages desktop wallpaper (solid, gradient, pattern), desktop shortcut grid, selection rectangle.

2. **`VxTaskbar` (Panel)**:
   - Fixed top/bottom panel holding Launcher trigger ("Start"), Active Window task buttons (with indicator dots for open/active states), Status Tray (Clock, Network status, Volume indicator), and Quick Settings toggle button.

3. **`VxLauncher` (Start Menu)**:
   - Popover surface anchored to Launcher trigger.
   - Includes real-time search box, categorical app grid, user profile header, and power options.

4. **`VxWindowManager`**:
   - Controls window creation, position, sizing, Z-order stack, min/max/restore animations, active window focus transitions, titlebar chrome rendering (close, minimize, maximize buttons, window title, active border highlight).

5. **`VxNotificationCenter`**:
   - Floating toast notification queue with auto-dismiss timers, action buttons, and slide-in motion.

---

## 6. System-Wide Input, Focus & Text Editing Behavior Plan

1. **Input Pipeline**:
   - PS/2 Keyboard & Mouse interrupts push normalized events to `vxair_input_event_t` event queue.
   - Compositor pulls events from queue and determines target:
     - Mouse events: Hit-tested top-down (Cursor -> Active Popovers -> Taskbar -> Windows in Z-order -> Desktop).
     - Keyboard events: Dispatched directly to active window -> focused widget.

2. **Focus Management (`VxFocusManager`)**:
   - Active window maintains a focus manager tracking the currently focused widget.
   - Tab key advances focus to next focusable widget in layout order; Shift+Tab reverses focus.
   - Focused widgets render a 2px high-contrast accent focus ring.

3. **Text Editing Subsystem (`VxTextEditor`)**:
   - Handles key presses (ASCII chars, Backspace, Delete, Left/Right arrows, Home, End).
   - Manages `caret_index` and `selection_start`/`selection_end`.
   - Renders a blinking 2px cursor caret and selection highlight rectangle using design token colors.

---

## 7. Rendering and Compositing Model

### Per-Frame Execution Pipeline:
```
1. Input Poll & Event Dispatch
   └─ Mouse / Keyboard events routed to shell or focused widget.

2. Widget State & Animation Update
   └─ Widgets mark internal dirty flags on state changes.

3. Window Surface Render Phase (Dirty Windows Only)
   └─ For each window with dirty region:
        - Bind window's VxSurface
        - Set clip rect to dirty bounds
        - Execute recursive widget render(surface)
        - Clear window dirty flag

4. Compositor Render Phase (Dirty Display Regions Only)
   └─ Accumulate display damage rects (moved windows, updated surfaces)
   └─ For each damaged display region:
        - Draw desktop background
        - Composite visible window surfaces (bottom-to-top Z-order)
        - Draw window chrome & shadows
        - Composite taskbar, launcher, notifications
        - Draw hardware mouse cursor

5. Hardware Present Phase
   └─ Copy dirty display regions from display backbuffer to hardware frontbuffer.
```

---

## 8. Vextryn Native Design Language (VNDL)

### 1. Color Palette Tokens:
- **Base Background**: `0xFF0B0F19` (Deep Slate Black)
- **Surface**: `0xFF161D2A` (Elevated Container)
- **Surface High**: `0xFF212B3D` (Card / Input Fill)
- **Border Subtle**: `0xFF2D3B52` (Standard Divider)
- **Border Bright**: `0xFF475978` (Active Border)
- **Accent Primary**: `0xFF3B82F6` (Electric Blue)
- **Accent Soft**: `0xFF1D4ED8` (Pressed Accent)
- **Text Primary**: `0xFFF8FAFC` (Pure White Text)
- **Text Secondary**: `0xFF94A3B8` (Muted Label Text)
- **Text Muted**: `0xFF64748B` (Disabled Text)
- **Status Success**: `0xFF10B981` (Green)
- **Status Destructive**: `0xFFEF4444` (Red)

### 2. Typography Scale:
- `FONT_TITLE_LARGE` (18px) - Window titles, Header banners
- `FONT_TITLE` (14px) - Section headers, Dialog titles
- `FONT_BODY` (12px) - Standard button labels, text boxes, list items
- `FONT_CAPTION` (10px) - Status bar, secondary timestamps

### 3. Spacing Scale:
- `SPACE_XS` (4px), `SPACE_SM` (8px), `SPACE_MD` (12px), `SPACE_LG` (16px), `SPACE_XL` (24px)

### 4. Bounding Radii & Elevation:
- `RADIUS_SM` (4px) - Buttons, Input boxes
- `RADIUS_MD` (8px) - Panels, Cards, Popups
- `RADIUS_LG` (12px) - Windows, Dialogs
- `ELEVATION_WINDOW`: 8px soft drop-shadow
- `ELEVATION_POPOVER`: 12px deep drop-shadow

---

## 9. Step-by-Step Migration Sequence

```
+-------------------------------------------------------------------+
| MILESTONE 1: Framebuffer, Surface & Compositor Core               |
| - Implement VxSurface, VxDamageTracker, VxCompositor Engine       |
| - Implement sub-region dirty rect hardware present step           |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
| MILESTONE 2: Retained UI Framework & Core Widgets                 |
| - Implement VxWidget tree, VxBoxLayout, VxGridLayout              |
| - Build VxButton, VxLabel, VxTextBox, VxPanel primitives          |
| - Build VxFocusManager, VxEventRouter, keyboard navigation        |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
| MILESTONE 3: Desktop Environment & Window Manager                 |
| - Build VxWindowManager, VxWindowSurface, Window Chrome           |
| - Build VxTaskbar, VxLauncher (Start Menu), Notification Center   |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
| MILESTONE 4: Native App Migration                                 |
| - Refactor apps (Calculator, SysMon, Notes, Terminal, Settings,   |
|   Files, Browser) to subclass VxWidget & use new framework        |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
| MILESTONE 5: Removal of Legacy Direct-Painting Code               |
| - Eliminate monolithic legacy draw loop in vxair_vxcomp.cpp       |
| - Clean up obsolete direct-to-backbuffer painting functions       |
+-------------------------------------------------------------------+
```

---

## 10. File Disposal & Migration Blueprint

| File / Component | Action | Details |
| :--- | :--- | :--- |
| `drivers/gpu/vxair_gpu_fb.c` | **Enhanced** | Add `vxair_gpu_fb_present_rects(VxRect* rects, int count)` for sub-region presentation. |
| `gui/vxrender/vxrender.hpp` | **Enhanced** | Integrate `VxSurface` offscreen rendering and clip-stack management. |
| `gui/vxui/vxui.hpp` | **Replaced** | Replace procedural button/panel primitives with object-oriented `VxWidget` tree framework. |
| `gui/compositor/vxair_vxcomp.cpp` | **Refactored / Replaced** | Remove 1,600-line monolithic draw loop; replace with modular `VxCompositor`, `VxWindowManager`, and `VxShell`. |
| `gui/desktop/vxair_desktop.cpp` | **Replaced** | Replace stub shell implementation with full `VxDesktopEnvironment`. |
| `gui/compositor/apps/*.hpp` | **Migrated** | Refactor all 14 apps to inherit from `VxWidget` and use retained layouts. |

---

## 11. Risks, Dependencies, and Order of Implementation

### Order of Implementation:
1. **Graphics/Framebuffer/Compositor Foundation** (`VxSurface`, `VxDamageTracker`, `VxCompositor`)
2. **UI Framework Core** (`VxWidget`, `VxContainer`, Layouts, Widgets, `VxFocusManager`)
3. **Windowing / Shell Infrastructure** (`VxWindowManager`, Window Chrome, Taskbar, Launcher)
4. **Desktop Environment Surfaces** (Wallpaper, Notification Center, Quick Settings)
5. **App Migration** (Calculator, SysMon, Notes, Settings, Terminal, Files, Browser, etc.)
6. **Removal of Legacy UI Paths**

### Technical Risks & Mitigations:
- **Risk 1: Software Compositing Overhead**:
  - *Mitigation*: Dirty rectangle invalidation ensures only modified screen regions are composited and blitted.
- **Risk 2: Heap Fragmentation in Bare-Metal Environment**:
  - *Mitigation*: Pre-allocate window surfaces and use fixed max widget pools for bare-metal safety.

---

## 12. Proven, Inferred, and Unknown Aspects

### PROVEN:
- Bare-metal framebuffer provides 1024x768 32-bit ARGB display (`drivers/gpu/vxair_gpu_fb.c`).
- Entire build succeeds using CMake + `make` producing `vextryn_air.elf`.
- Existing UI in `vxair_vxcomp.cpp` relies on immediate direct painting to a single display backbuffer without window surface isolation.

### INFERRED:
- PS/2 driver input is polled synchronously via hardware ports 0x60 and 0x64.
- OS runs inside QEMU with VBE / GOP display output.

### UNKNOWN:
- Hardware 2D/3D acceleration capabilities (assumed software rendering pipeline for maximum hardware compatibility).
