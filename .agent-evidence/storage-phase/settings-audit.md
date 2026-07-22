# Settings Audit & Persistence Plan

## 1. Existing Settings State
The settings state is maintained centrally in `vxair_vxcomp.cpp` within the `VxGuiState g_state` struct. It consists of:
- `mouse_sensitivity_level` (int): Ranges logically from 1 to 9. Initialized to 7.
- `wallpaper_mode` (int): Defined (0, 1, 2) but completely unused in rendering logic.
- `accent_color` (uint32_t): ARGB hex format, initialized to `0xFF00F0FF`.
- `compact_taskbar` (bool): Toggles height between 40px (`true`) and 56px (`false`). Initialized to `true`.

## 2. Compositor Theme Code
- `g_state.accent_color` is actively used in `draw_polished_desktop()` and `draw_window()`.
- It dynamically colors focused window borders, taskbar active indicators, the launcher button, open launcher menu borders, and selected UI elements inside apps.
- The desktop background currently ignores `wallpaper_mode` and instead renders a hardcoded vertical gradient (lerp from `0xFF020617` to `0xFF0F172A`).

## 3. Input Sensitivity Code
- Found in `handle_input()` processing PS/2 mouse bytes (lines ~260-310).
- Uses a lookup table: `int scale_lut[10] = {32, 8, 16, 24, 32, 48, 64, 96, 128, 192};`.
- `mouse_sensitivity_level` is safely clamped between 1 and 9 before indexing.
- Position is maintained using a fixed-point coordinate system (`exact_x_fp`, `exact_y_fp`, bit-shifted by 8) which scales raw mouse deltas for smooth sub-pixel precision.

## 4. Personalization Behavior (UI)
The interface resides in `apps/app_settings.hpp` inside `draw_app_settings()`:
- **Mouse**: Renders 9 clickable blocks (1-9) that immediately mutate `g_state.mouse_sensitivity_level`.
- **Taskbar**: Renders 'C' (Compact) and 'B' (Big) blocks mutating `g_state.compact_taskbar`.
- **Theme**: Renders 5 predefined color blocks mutating `g_state.accent_color`.
- **About**: Displays static "VXAIR0.1" text.

## 5. Persistence Hooks
- **None currently exist.** Settings only live in RAM.
- `vxair_compositor_main()` initializes values with static hardcoded defaults at boot time. Mutations made in `app_settings.hpp` apply immediately but are lost on reboot.

## 6. Safe Persistence Plan
To safely implement persistence without disrupting the current compositor loop:

1. **Model Definition**: Keep `g_state` as the active runtime model, but define a lightweight `SettingsBlock` struct mapped to just the configurable fields for disk I/O.
2. **File System Integration**: Define a consistent settings path (e.g., `/sys/cfg/settings.dat`) utilizing the OS's existing VFS (Virtual File System) mechanisms.
3. **Load Hook (Boot)**: Inside `vxair_compositor_main()`, just before `mouse_init()`, execute a `load_settings()` routine. 
   - **Validation is crucial**: Ensure loaded variables are clamped (e.g., `mouse_sensitivity_level` between 1-9) to prevent out-of-bounds crashes during input handling. Fallback to hardcoded defaults if the file is missing/invalid.
4. **Save Hook (Event-Driven)**: Do not poll or save every frame. Instead, inside `app_settings.hpp`, add a `save_settings()` call directly inside the click handlers so that the file is updated *only* when a user clicks to mutate a setting.
