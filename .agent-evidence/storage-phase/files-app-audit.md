# Files App & Compositor Architecture Audit

## 1. Current Static/Mock Behavior
* **File System (`g_state.ram_files`)**: Currently mocked entirely in RAM. The system supports up to 10 files, each with a maximum name length of 15 chars and content length of 511 chars. No actual disk persistence or VFS integration exists.
* **Date & Time**: Hardcoded visually as `"JUL 20 10:42"` in the system tray. The terminal app also hardcodes the `date` command output.
* **Apps List**: Hardcoded static enum (`VxAppId`) and instances (Calculator, Notes, SysMon, Files, Settings, Terminal, Snake).
* **Terminal**: Hardcoded string parsing for commands (`help`, `clear`, `whoami`, `version`, `date`) returning static strings. No real shell execution.
* **System Monitor (SysMon)**: Visually mocked UI elements with hardcoded widths to mimic usage charts for CPU/RAM.
* **Notes**: A single static 1024-byte character buffer (`g_state.notes`) stored in the compositor state.

## 2. Real Hooks Available
* **Input Architecture**:
  * Keyboard: PS/2 hook via `inb(0x60)` integrated with a `scancode_to_ascii()` mapper supporting shift states.
  * Mouse: Real PS/2 mouse packet parsing (3-byte protocol) that handles deltas, clamping, sensitivity scaling, and click/drag detection.
* **Graphics / Compositor**:
  * `vxair_fb_fill_rect(x, y, w, h, color)`: Direct framebuffer rectangle drawing.
  * `vxair_fb_flip()`: Buffer swapping.
  * `font8x8.h` / `draw_abstract_char()`: Real bitmap font rendering character by character.
  * Display dimensions available via `vxair_fb_get_width()` / `vxair_fb_get_height()`.
* **Timing & Execution**:
  * `vxair_hpet_sleep_ms(ms)`: HPET-based sleep for frame pacing (e.g., 16ms for ~60 FPS).
  * `g_frame`: Global frame counter used for blinking cursors and animations.
* **Logging**:
  * `vxair_log_info(...)`: Formatted logging to serial/console output.

## 3. Inventory of Elements That Must Not Regress
### Taskbar & Desktop
* **Desktop Background**: Vertical gradient drawing (`lerp_color`) with a 40px grid overlay.
* **Taskbar Layout**: 
  * Adaptive height (40px compact or 56px default).
  * Accent color border on top (`g_state.accent_color`).
* **Launcher Button**: 32x32 button positioned at `x=12`. Must toggle launcher on click and highlight on hover.
* **Taskbar App Icons**: Placed starting at `x=60`, spaced by 40px. Must display active state (accent color underline) and highlight on hover.
* **System Tray**: Right-aligned block (Network, Battery, Clock) starting at `W - 180`.

### Window Manager (Max 7 Windows via `g_z_order`)
* **Window Chrome**: 1px border colored with accent (if focused) or grey (if unfocused).
* **Title Bar**: 24px height. Must support click-and-drag for moving windows (`dragging`, `drag_offset_x`, `drag_offset_y`).
* **Close Button**: 16x16 button on the right of the title bar (turns bright red on hover).
* **Z-Ordering & Focus**: Clicking inside a window or its taskbar icon must bring it to the front and apply focus.

### Launcher Menu
* **Position & Layout**: 200px wide menu anchoring above the launcher button, framed by the accent color.
* **Behavior**: Must close if clicking outside or pressing Escape.
* **Items**: Clickable 30px high list rows that launch respective apps.
