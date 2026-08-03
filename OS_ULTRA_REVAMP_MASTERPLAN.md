# OS Ultra Revamp Masterplan: Vextryn Air OS Platform Architecture

## Executive Summary

Vextryn Air OS is a 64-bit C++ bare-metal operating system designed for real, daily-usable desktop computing. This document defines the comprehensive engineering masterplan to rebuild the entire user experience stack. 

Rather than applying superficial cosmetic patches, this revamp replaces the existing procedural visual stack with a modular, high-performance platform architecture consisting of:
1. **Engine-Grade Surface & Compositing Architecture** (`VxSurface`, `VxDamageTracker`, `VxCompositor`)
2. **Retained Component Framework** (`VxWidget`, `VxLayoutManager`, `VxFocusManager`, `VxTextEditorCore`)
3. **Integrated Desktop Shell & Window Manager** (`VxDesktopShell`, `VxWindowManager`, `VxTaskbar`, `VxControlCenter`)
4. **Cohesive Operating System Design System** (`VxDesignSystem` tokens, typography hierarchy, elevation, states)
5. **Comprehensive First-Party App Ecosystem** (16+ full-featured desktop applications)

---

## 1. Project Description & Goals

### 1.1 Project Mission
Transform Vextryn Air OS from a bare-metal proof-of-concept into a serious, modern operating system platform. The OS must run efficiently in QEMU and real x86_64 hardware, delivering an experience that feels native, responsive, keyboard-accessible, visually rich, and platform-coherent.

### 1.2 Architectural Objectives
- **Zero Direct-to-Screen Painting**: Applications and shell elements render exclusively to isolated offscreen surface buffers (`VxSurface`).
- **Dirty-Rectangle Invalidation**: The presentation pipeline tracks damaged screen regions (`VxDamageTracker`), avoiding full 1024x768 frame flushes when only sub-regions update.
- **Retained Widget Tree**: All interface elements inherit from a structured widget hierarchy with parent-child ownership, layout passes, event routing, and focus management.
- **System-Wide Behavior & Controls**: Common behaviors (text selection, cursor navigation, focus rings, keyboard shortcuts, modal dialogs, clipboards) are implemented in framework controls rather than duplicated in apps.
- **Rich Desktop Environment**: A unified desktop shell offering workspace surfaces, window management with snap preview/shadows, system tray/status bar, searchable launcher, quick settings control center, and toast notifications.
- **Full First-Party Ecosystem**: A complete set of desktop applications that cover file management, terminal CLI, web browsing, code/text editing, document viewing, media playback, system monitoring, utility management, and software installation.

---

## 2. Root Cause Analysis: Why the Legacy UI Feels Fake & Drawn

| Defect Area | Legacy Implementation | Root Cause | User Impact |
| :--- | :--- | :--- | :--- |
| **Rendering Model** | Direct procedural drawing into global `fb_back`. | No offscreen window surfaces; windows paint over each other directly. | Visual tearing, flickering when dragging windows, lack of window transparency/shadows. |
| **Code Structure** | 1,600+ line monolithic `vxair_vxcomp.cpp`. | Hardware input, window management, desktop drawing, and 14 app views mixed together. | Impossible to maintain; apps lack independent lifecycles or reusability. |
| **Typography** | Manual line segment loops & raw 8x8 bitmap masks. | No font engine, font metrics, line wrapping, or subpixel kerning. | Text looks like hand-painted pixel lines rather than OS typography. |
| **Input & Focus** | Primitive `int focused_window` variable. | No event routing tree, keyboard tab order, focus ring indicators, or hit-test delegation. | Controls cannot be navigated via keyboard; input handling is app-specific hackery. |
| **Text Editing** | Basic string append/backspace logic (`VxTextInput`). | No cursor movement, text selection highlighting, word navigation, or clipboard support. | Text fields feel like simple raw key loggers rather than native UI text controls. |
| **Window System** | Simple bounding-box drag loop in global array. | No window surface backing, no window chrome encapsulation, no snap tiling, no shadows. | Windows feel like rectangles painted onto a canvas. |

---

## 3. Weaknesses in the Current Framebuffer & Rendering Path

```
+-----------------------------------------------------------------------------------+
| LEGACY PATH (DRAWING DIRECT TO GLOBAL BACKBUFFER)                                |
| App/Window Draw -> Global Backbuffer (Overwriting Pixels) -> Full memcpy to FB   |
+-----------------------------------------------------------------------------------+

+-----------------------------------------------------------------------------------+
| NEW ARCHITECTURE (SURFACE COMPOSITING & DAMAGE TRACKING)                           |
| App 1 -> Surface 1 --+                                                            |
| App 2 -> Surface 2 --+--> Compositor -> Damage Tracker -> Dirty Region Blit -> FB |
| Shell -> Shell Surface -+                                                         |
+-----------------------------------------------------------------------------------+
```

### Architectural Flaws in Legacy Path:
1. **Full-Buffer Copy Flushes**: `vxair_fb_flip()` copies the entire 1024x768x32 framebuffer (3.14 MB) on every frame loop regardless of changes.
2. **Lack of Window Surface Isolation**: Windows do not own backing memory. Redrawing one window requires redrawing all windows beneath it and the desktop wallpaper.
3. **Single Global Clip Rectangle**: `g_vxr_ctx.clip` supports only one active clipping rectangle. Complex nested containers, scrollable lists, and popovers break clipping boundaries.
4. **Unoptimized CPU Alpha Blending**: Transparency is calculated using unvectorized integer division (`/ 255`) across every pixel in target rectangles.
5. **No Invalidation System**: The display engine lacks damage accumulation (`VxRect` list), forcing full-screen repaints for single-character typing or cursor movement.

---

## 4. New Rendering & Compositor Architecture

### 4.1 `VxSurface` Offscreen Buffer Model
Each window, panel, and shell component renders into an offscreen pixel surface (`VxSurface`):
```cpp
struct VxSurface {
    uint32_t* pixels;
    int32_t width;
    int32_t height;
    int32_t stride; // pixels per row
    bool has_alpha;
    VxRect damage_rect; // Accumulation of modified bounds
    
    void clear(uint32_t color);
    void mark_dirty(int x, int y, int w, int h);
};
```

### 4.2 `VxDamageTracker` Region Invalidation
The damage tracker maintains an active list of invalidated screen regions:
```cpp
struct VxDamageTracker {
    static const int MAX_DAMAGED_RECTS = 32;
    VxRect damaged_rects[MAX_DAMAGED_RECTS];
    int count;

    void add_damage(int x, int y, int w, int h);
    void merge_overlapping();
    void reset();
};
```

### 4.3 `VxCompositor` Multi-Layer Pipeline
The compositor executes a multi-layered rendering pass:
1. **Wallpaper Surface**: Blits desktop background or gradient texture into the backbuffer damaged regions.
2. **Window Surface Layer**: Composites active windows in Z-order array. Blits window surfaces with drop-shadow alpha calculation into damaged bounds.
3. **Shell Surface Layer**: Blits floating Top Bar, Taskbar, Control Center popover, Launcher overlay, and Toast Notifications.
4. **Cursor Surface Layer**: Blits high-resolution cursor sprite with alpha transparency at `(mouse_x, mouse_y)`.
5. **Presentation Step**: Transfers only damaged bounding boxes from `fb_back` to `fb_front`.

---

## 5. New UI Framework Architecture

The framework adopts a **retained component tree** with explicit layout, measurement, event routing, and focus management.

```
                  +-------------------+
                  |     VxWidget      | (Base Component)
                  +-------------------+
                            |
        +-------------------+-------------------+
        |                                       |
+---------------+                       +---------------+
|  VxContainer  |                       |   VxControl   |
+---------------+                       +---------------+
        |                                       |
  +-----+-----+                           +-----+-----+-----+-----+
  |           |                           |     |     |     |     |
VxBoxLayout  VxGridLayout              VxButton VxLabel VxText VxSlider...
```

### 5.1 Core Framework Classes
- **`VxWidget`**: Base UI node containing bounds (`x, y, w, h`), parent pointer, visibility, enabled state, focusable flag, and virtual `measure()`, `arrange()`, `render()`, `on_event()` methods.
- **`VxContainer`**: Manages children lists, composite layout calculation, and recursive rendering/event routing.
- **`VxLayoutManager`**: Flexbox-style (`VxBoxLayout`) and Grid-style (`VxGridLayout`) automated positioning engine.
- **`VxFocusManager`**: Tracks system focus, tab key order navigation, focus ring drawing, and default button triggering.
- **`VxTextEditorCore`**: Full text editing model featuring caret positioning, selection ranges, word movement (Ctrl+Left/Right), select all (Ctrl+A), delete/backspace handling, and clipboard integration (`VxClipboard`).

### 5.2 Framework Control Gallery
- **Basic Controls**: `VxButton`, `VxLabel`, `VxIcon`, `VxDivider`, `VxBadge`.
- **Input Controls**: `VxTextField`, `VxTextArea`, `VxCheckbox`, `VxSwitch`, `VxSlider`, `VxDropdown`.
- **Structure & Layout**: `VxPanel`, `VxCard`, `VxScrollView`, `VxTabBar`, `VxToolBar`, `VxStatusBar`.
- **Data & Collections**: `VxListView`, `VxGridView`, `VxTreeControl`, `VxTableView`, `VxProgressBar`.
- **Dialogs & Menus**: `VxModalDialog`, `VxContextMenu`, `VxTooltip`, `VxToastNotification`.

---

## 6. New Desktop Environment Architecture

The Desktop Environment (`VxDesktopShell`) is an integrated system shell managing all shell affordances and window interactions.

```
+-----------------------------------------------------------------------+
| TOP BAR / SYSTEM PANEL (Clock, Status Indicators, Quick Settings Btn)  |
+-----------------------------------------------------------------------+
|                                                                       |
|   +-----------------------+     +-----------------------+             |
|   | Window 1 (Active)     |     | Window 2 (Background) |             |
|   | - Titlebar + Controls |     | - Titlebar + Controls |             |
|   | - Surface Buffer      |     | - Surface Buffer      |             |
|   +-----------------------+     +-----------------------+             |
|                                                                       |
+-----------------------------------------------------------------------+
| BOTTOM DOCK / TASKBAR (Start Menu, App Pills, System Tray)           |
+-----------------------------------------------------------------------+
```

### 6.1 Desktop Components
1. **`VxDesktopSurface`**: Desktop background workspace supporting shortcuts, wallpapers, and rectangle drag selection.
2. **`VxTaskbar`**: Integrated launcher bar, app window pills with active indicators, system tray clock, and quick action icons.
3. **`VxAppLauncher`**: Searchable popover application launcher with instant filtering, category tabs, and recent documents.
4. **`VxControlCenter`**: Slide-out panel for quick toggles (Wi-Fi, Bluetooth, Dark Mode, DND, Volume slider, Brightness slider).
5. **`VxWindowManager`**: Manages window focus, Z-order stack, titlebar dragging, window resizing handles, maximize/minimize state, snap-to-edge preview guidelines, and window close confirmation.
6. **`VxNotificationCenter`**: Manages toast notifications with slide-in animations and action triggers.

---

## 7. App Model & Window Model

### 7.1 Window Architecture (`VxWindow`)
Each window consists of a frame wrapper and a client surface:
- **`VxWindowFrame`**: Renders window titlebar, app icon, title text, window action buttons (Minimize, Maximize, Close), active drop-shadows, and 1px subtle borders.
- **`VxClientSurface`**: Dedicated `VxSurface` owned by the application. Applications draw inside their client bounds `(0, 0, w, h)` without interacting with window frame borders or global screen coordinates.

### 7.2 Application Interface (`VxApp`)
All system applications implement the `VxApp` base class:
```cpp
class VxApp {
public:
    virtual void on_init(VxWindow* window) = 0;
    virtual void on_update(float delta_time) = 0;
    virtual void on_render(VxSurface& client_surface) = 0;
    virtual void on_event(const VxEvent& event) = 0;
    virtual void on_shutdown() = 0;
};
```

---

## 8. System-Wide Input, Focus, Text & Navigation Behavior

### 8.1 Input Abstraction Pass
Low-level hardware scancodes (PS/2 keyboard, mouse packets) are transformed into normalized system `VxEvent` objects:
- `VX_EV_MOUSE_MOVE`, `VX_EV_MOUSE_PRESS`, `VX_EV_MOUSE_RELEASE`, `VX_EV_MOUSE_SCROLL`
- `VX_EV_KEY_DOWN`, `VX_EV_KEY_UP`, `VX_EV_KEY_CHAR`
- `VX_EV_WINDOW_FOCUS`, `VX_EV_WINDOW_BLUR`, `VX_EV_RESIZE`

### 8.2 Keyboard Navigation Standards
- **Tab / Shift+Tab**: Traverses focus sequentially across focusable controls inside the active window.
- **Enter / Space**: Triggers the currently focused button, checkbox, or control action.
- **Escape**: Closes active modal dialogs, context menus, popovers, or launcher overlays.
- **Arrow Keys**: Navigates options within dropdowns, radio groups, list views, and grid views.

### 8.3 Text Selection & Editing Standard
- **Text Cursor Navigation**: Left/Right arrows move caret by character; Up/Down move across lines.
- **Word Navigation**: `Ctrl+Left` / `Ctrl+Right` jumps caret by word boundaries.
- **Text Selection Range**: `Shift+Left/Right/Home/End` expands or shrinks selection highlight. `Ctrl+A` selects all text.
- **Clipboard Integration**: `Ctrl+C` copies selected text to `VxClipboard`; `Ctrl+X` cuts selection; `Ctrl+V` pastes from `VxClipboard`.

---

## 9. System Visual Language & Design System (`VxDesignSystem`)

### 9.1 Color Token Palette (Slate Dark Theme)
- **Background Base**: `#0D1117` (Deep Dark Blue-Gray)
- **Surface Level 1**: `#161B22` (Card / Panel Background)
- **Surface Level 2**: `#21262D` (Window Body / Container Fill)
- **Surface Level 3 / Hover**: `#30363D` (Interactive Hover Fill)
- **Border Subtle**: `#21262D` (Quiet Dividers)
- **Border Bright**: `#30363D` (Control & Window Borders)
- **Accent Primary**: `#2F81F7` (Vibrant Blue Accent)
- **Accent Glow**: `#58A6FF` (Focus Ring & Active Highlight)
- **Text Primary**: `#F0F6FC` (High Contrast Crisp White-Blue)
- **Text Secondary**: `#8B949E` (Muted Gray Text)
- **Status Colors**: Danger `#F85149`, Success `#3FB950`, Warning `#D29922`, Info `#58A6FF`

### 9.2 Typography Scale
- **Display**: 20px Bold (Window Titles, Hero Headers)
- **Heading**: 16px SemiBold (Section Headers, Modal Titles)
- **Subheading**: 14px Medium (App Section Titles, Card Headers)
- **Body**: 12px Regular (Standard Labels, Text Fields, Buttons)
- **Caption**: 10px Medium (Status Messages, Tooltips, Muted Subtexts)
- **Monospace**: 12px Mono (Terminal Output, Code Editor Text)

### 9.3 Spacing, Radius & Elevation Scales
- **Spacing Scale**: 4px, 8px, 12px, 16px, 24px, 32px
- **Corner Radii**: Small 4px (Buttons, Text Inputs), Medium 8px (Panels, Cards, Modals), Large 12px (Windows, Shell Popovers)
- **Elevation / Depth**:
  - Level 0: Flat background surface
  - Level 1: Panel / Card (Subtle 4px drop shadow)
  - Level 2: Standard Window (Deep 12px drop shadow)
  - Level 3: Elevated Popover / Modal (20px drop shadow + backdrop dimming)

---

## 10. First-Party Application Suite Plan

### 10.1 First-Party Application Matrix

| App ID | Name | Category | Phase | Description |
| :--- | :--- | :--- | :--- | :--- |
| `vxfiles` | **File Manager** | System | Phase 1 | Multi-pane file navigation, folder creation, file deletion, rename mode, file preview drawer, storage status bar. |
| `vxterm` | **Terminal** | System | Phase 1 | Full-featured CLI shell with command line editing, history, command output, executable launching, color output. |
| `vxsettings` | **Settings** | System | Phase 1 | System configurator for resolution, themes, wallpaper mode, input sensitivity, network toggles, system specs. |
| `vxweb` | **Web Browser** | Network | Phase 1 | Serious web browser architecture with URL navigation bar, tab management, bookmarks bar, HTML layout renderer. |
| `vxedit` | **Code / Text Editor** | Productivity | Phase 1 | Syntax-aware text editor with line numbers, multi-tab editing, search/replace, file load/save capabilities. |
| `vxcalc` | **Calculator** | Utility | Phase 1 | Standard and scientific math calculator, formula display, operation history log, quick keypad. |
| `vxsysmon` | **Task Manager** | System | Phase 1 | Real-time CPU graph, RAM utilization pie breakdown, active process table, process kill control. |
| `vxview` | **Image Viewer** | Media | Phase 2 | Graphic image viewer with zoom/pan controls, image rotation, slideshow mode, thumbnail carousel. |
| `vxdoc` | **Document Viewer** | Productivity | Phase 2 | PDF / rich document viewer with page controls, zoom level, text searching, thumbnail drawer. |
| `vxmedia` | **Media Player** | Media | Phase 2 | Audio waveform player, playback scrubber, volume control, playlist manager, visualizer mode. |
| `vxclock` | **Clock & Calendar** | Utility | Phase 2 | Digital/analog clock, multi-timezone timekeeper, month calendar view, stopwatch, countdown timer. |
| `vxnotes` | **Notes App** | Productivity | Phase 2 | Categorized note taking app, quick search, rich note cards, auto-save storage. |
| `vxshot` | **Screenshot Utility** | Utility | Phase 2 | Desktop capture tool, region selector, clipboard copy, PNG file saver. |
| `vxzip` | **Archive Manager** | System | Phase 3 | Archive file viewer, compression/decompression tool, file extractor interface. |
| `vxlaunch` | **App Launcher** | System | Phase 1 | Full-screen and popover application search, categorized software grid, recent file quick launcher. |
| `vxstore` | **Software Center** | System | Phase 3 | System package installer, software repository list, app update manager. |

---

## 11. App Rollout Phasing

```
+-----------------------------------------------------------------------------+
| PHASE 1: ESSENTIAL CORE DEFAULTS (BOOTABLE & FUNCTIONAL OS PLATFORM)        |
| - File Manager (vxfiles)      - Web Browser Core (vxweb)                   |
| - Terminal Shell (vxterm)     - Task Manager (vxsysmon)                     |
| - System Settings (vxsettings)- App Launcher (vxlaunch)                    |
| - Text/Code Editor (vxedit)   - Math Calculator (vxcalc)                    |
+-----------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------+
| PHASE 2: MEDIA, PRODUCTIVITY & DESKTOP UTILITIES                            |
| - Image Viewer (vxview)       - Clock & Calendar (vxclock)                  |
| - Media Player (vxmedia)      - Notes App (vxnotes)                         |
| - Document Viewer (vxdoc)     - Screenshot Utility (vxshot)                 |
+-----------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------+
| PHASE 3: ADVANCED PLATFORM UTILITIES                                        |
| - Archive Manager (vxzip)     - Web Engine Enhancements (vxweb v2)          |
| - Software Center (vxstore)   - Integrated IDE Extensions (vxedit v2)     |
+-----------------------------------------------------------------------------+
```

---

## 12. Migration Strategy: Legacy UI to Ultra Platform

```
+------------------+     +------------------+     +------------------+
| STEP 1: GRAPHICS | --> | STEP 2: UI CORE  | --> | STEP 3: DESKTOP  |
| Build VxSurface  |     | Build VxWidget   |     | Build Desktop    |
| & VxCompositor   |     | & Layout System  |     | Shell & Window   |
+------------------+     +------------------+     +------------------+
                                                           |
                                                           v
+------------------+     +------------------+     +------------------+
| STEP 6: CLEANUP  | <-- | STEP 5: ADVANCED | <-- | STEP 4: PHASE 1  |
| Purge Legacy     |     | Build Phase 2/3  |     | Rebuild Essential|
| Monolithic Comp  |     | Applications     |     | Apps (8 core)    |
+------------------+     +------------------+     +------------------+
```

---

## 13. Component Disposition Matrix

| Component | Status | Action / Replacement Strategy |
| :--- | :--- | :--- |
| `drivers/gpu/vxair_gop.c` | **Wrapped** | Keep hardware framebuffer mapping; adapt `vxair_fb_flip` to dirty rect blitting. |
| `gui/vxrender/vxrender.hpp` | **Replaced** | Refactor into `VxSurface` rasterizer with clip stack and SIMD-optimized blits. |
| `gui/vxui/vxui.hpp` | **Replaced** | Rebuild into full retained component framework (`VxWidget`, `VxLayoutManager`). |
| `gui/compositor/vxair_vxcomp.cpp` | **Deleted** | Replace 1,600+ line monolithic procedural file with `VxCompositor` & `VxDesktopShell`. |
| `gui/compositor/apps/*.hpp` | **Replaced** | Refactor inline app views into clean modular `VxApp` classes inheriting from `VxWidget`. |
| `gui/desktop/vxair_desktop.cpp` | **Replaced** | Rebuild into modular `VxDesktopShell` with surface composition. |

---

## 14. Risks, Dependencies & Execution Order

### 14.1 Key Implementation Risks
1. **Memory Pressure**: Allocating multiple 32bpp offscreen window surfaces (1024x768x4 = 3.14MB each) in freestanding kernel heap.
   - *Mitigation*: Surface buffers are sized dynamically to actual window dimensions (e.g. 640x480 = 1.2MB) and freed on window close.
2. **QEMU Software Compositing Overhead**: Full CPU compositing could drop framerates in emulation.
   - *Mitigation*: Strictly enforce dirty-rectangle tracking so only modified regions are composited and blitted per tick.

### 14.2 Execution Order
1. **Rendering / Compositor Foundation**: Build `VxSurface`, `VxDamageTracker`, and `VxCompositor`.
2. **UI Framework Core**: Implement `VxWidget`, layout engines, `VxFocusManager`, `VxTextEditorCore`, and core controls.
3. **Shell & Window Manager Foundation**: Build `VxDesktopShell`, `VxWindowManager`, `VxTaskbar`, and `VxControlCenter`.
4. **Core System Surfaces**: Integrate Launcher overlay, Toast notifications, Context menus, and Modal dialogs.
5. **Phase 1 Core Applications**: Rebuild `vxfiles`, `vxterm`, `vxsettings`, `vxedit`, `vxcalc`, `vxsysmon`, `vxweb`, and `vxlaunch`.
6. **Phase 2 & 3 Applications**: Build media players, viewers, archives, utilities, and software center.
7. **Legacy Cleanup**: Purge old monolithic compositor files and verify 100% build integrity.

---

## 15. Technical Evidence: Proven, Inferred & Unknown

### 15.1 Proven (Empirically Verified in Codebase)
- Framebuffer resolution is 1024x768x32bpp mapped via UEFI GOP driver (`drivers/gpu/vxair_gop.c`).
- System compiles cleanly with freestanding GCC/G++ (`-ffreestanding -fno-exceptions -fno-rtti -std=c++20`).
- Top-level `make` successfully outputs `build/bin/vextryn_air.elf` and ISO images.

### 15.2 Inferred (System Design Deductions)
- Isolating window rendering into per-window `VxSurface` buffers will completely eliminate visual tearing and flickering during window dragging.
- Implementing dirty-rectangle invalidation will reduce QEMU frame blit bandwidth by up to 90% during idle or localized editing operations.

### 15.3 Unknown (Requires Runtime Tuning)
- Maximum simultaneous open window count before kernel slab memory exhaustion in 64MB QEMU baseline. (Will test dynamically and set standard window bounds limit if needed).
