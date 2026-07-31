# Control Center Wi-Fi Toggle UI Regression Fix

## 1. Exact Root Cause
The toggle switches in the Control Center (Wi-Fi, Bluetooth, AirDrop, DND) were inadvertently coupled to unrelated UI visual presentation variables (like `show_top_bar` and `focus_dim`). When the user clicked the Wi-Fi tile, it was directly toggling the global compositor state for the Top Menu Bar visibility rather than an internal Wi-Fi state flag.

## 2. Wrongly Coupled State Variables
The tiles were bound to the following variables in `VxGuiState` (`g_state`):
- **Wi-Fi** -> `show_top_bar` (Caused the top menu bar to disappear/reappear)
- **Bluetooth** -> `show_desktop_glow`
- **AirDrop** -> `show_window_shadows`
- **DND** -> `focus_dim`

## 3. Exact Files Changed
1. `gui/compositor/vxair_vxcomp.cpp`
   - Added `wifi_enabled`, `bluetooth_enabled`, `airdrop_enabled`, and `dnd_enabled` booleans to `struct VxGuiState` (line 86).
2. `gui/compositor/apps/app_control_center.hpp`
   - Modified the `tiles` array mapping (line 94) to point to the new dedicated booleans (`&g_state.wifi_enabled`, etc.) instead of the presentation variables.

## 4. Exact Test Steps
1. Launched the compositor.
2. Opened the Control Center.
3. Clicked the Wi-Fi tile to toggle it ON and OFF.
4. Observed that the top menu bar remains visible and unaffected by the Wi-Fi toggle state.

## 5. Explicit Statement
This is a **UI-only fix**. It correctly isolates the visual toggle states in the Control Center but does **not** represent real Wi-Fi functionality or adapter state mapping. The UI is now stable and decoupled from the network stack bring-up.
