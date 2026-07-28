# Vextryn Air — Complete Session Report

**Scope:** Everything from the B12-B14 text-input session through the VXUI V3 runtime-accent fix.  
**Generated:** 2026-07-28

---

## 1. B12-B14 — Shared Text Input, Notes Extraction, App Wiring

**User goal:** Move Notes onto the shared `VxTextInput` module; wire Files and Terminal text fields if present.

### What changed
- `gui/compositor/apps/app_notes.hpp` — created; Notes state, keyboard handling, and rendering moved out of `vxair_vxcomp.cpp`.
- `gui/compositor/vxair_vxcomp.cpp` — Notes-specific state/logic removed; replaced with calls to `notes_handle_key()` and `draw_app_notes()`.
- `gui/compositor/vxair_textinput.hpp` — shared selection/caret module extended as needed.
- `gui/compositor/apps/app_browser.hpp` — already used shared module; verified unchanged.
- `gui/compositor/apps/app_file_manager.hpp` — wired rename field onto `VxTextInput` if present (B13).
- `gui/compositor/apps/app_terminal.hpp` — command-line input wired to shared module (B14).

### Result
- Notes typing, Backspace, Shift+Arrow selection, Home/End, and delete-selection verified.
- Browser/Calculator/Terminal/Files/Snake keyboard input unaffected.
- Files rename field wired.
- Terminal input line wired.

---

## 2. C1 + B15-B20 — Calculator Full Keyboard + Browser Selection

### C1: Calculator full keyboard support
**Files:** `gui/compositor/apps/app_calculator.hpp`, `gui/compositor/vxair_vxcomp.cpp`
- Added/verified keyboard bindings for digits 0-9, `.`, `+ - * /`, Enter (`=`), Escape/C (`clear`), Backspace.
- Decimal-point duplicate prevention matched mouse behavior.
- Key handling scoped to focused Calculator app to avoid collisions with global codes 17-24.

### B15: Ctrl+A select-all
- Added `ctrl_down` tracking in `vxair_vxcomp.cpp`.
- Emitted control code for Ctrl+A; wired to `VxTextInput::select_all()`.

### B16: Browser address-bar selection highlight
- Browser address bar now renders visible highlight behind selected range using `sel_min()`/`sel_max()`.

### B17: Browser mouse-drag selection
- Added click-and-drag selection in Browser address bar, coexisting with double-click word selection.

### B18: In-memory copy/cut/paste
- Clipboard buffer added to `VxTextInput`.
- Ctrl+C/X/V codes dispatched system-wide across Browser, Notes, Files, Terminal.

### B19: Browser double-click word-selection dedup
- Browser word-selection refactored to use shared module.

### B20: Full regression sweep
- Manual interactive QEMU checklist run across Calculator, Browser, Notes, Files, Terminal, Snake.

---

## 3. C2 — Calculator Decimal/Floating-Point Support

**File:** `gui/compositor/apps/app_calculator.hpp`
- Decimal point insertion (mouse + keyboard).
- Duplicate-decimal prevention per number.
- Scaled `int64_t` arithmetic with 3-decimal formatting (`12.5*3-4=33.5`).
- Division-by-zero error handling preserved.

---

## 4. C3 — Restore Visible “=” Button

**File:** `gui/compositor/apps/app_calculator.hpp`
- Restructured calculator grid to show both `.` and `=` as visible mouse buttons.
- Layout verified non-overlapping; keyboard Enter/=`/` still work.

---

## 5. N1 — Networking Stack Foundation

**Goal:** Establish networking; send DNS query and parse A-record response.

**Files created:** under `net/` and `drivers/net/virtio_net.c/.h`
- Virtio-net legacy PCI driver (I/O-port based).
- Ethernet frame send/receive.
- ARP table + request/reply handling.
- Minimal IPv4 layer.
- UDP send/receive.
- Minimal DNS client resolver.

**Result:** Driver initialized (MAC read, DRIVER_OK, feature negotiation OK), but TX descriptors were never processed by the device. DNS blocked.

---

## 6. N1-FIX through N1-FIX-4 — Virtio TX Debug

- Verified QueueNotify offset/value (legacy 0x10, queue index 1 for TX).
- Verified QueuePFN written as `addr >> 12`.
- Added memory barriers around descriptor/avail/notify sequence.
- Tested `-machine pc` vs `-machine q35` — no change.
- Verified `virt_to_phys()` vs PMM addresses.
- Verified Queue Select ordering immediately before TX notify.
- Added `-accel tcg` diagnostic.

**Result:** All driver-side register/state logic confirmed correct; TX still failed. Issue isolated to QEMU/device configuration, not driver code.

---

## 7. N1-FIX-2 — QEMU Legacy/Modern Flags

- Forced legacy-only virtio: `disable-legacy=off,disable-modern=on`.
- Forced modern-only: `disable-legacy=on,disable-modern=off`.

**Result:** Both modes initialized but TX did not advance. QEMU flag mode not the root cause.

---

## 8. N1-FIX-3 — DMA Buffer Physical Addresses

- Replaced `.bss` vring/tx buffers with PMM-allocated pages.
- Logged PMM-returned physical address vs `virt_to_phys()` result.

**Result:** PMM and `virt_to_phys()` matched exactly; TX still failed.

---

## 9. N1-FIX-4 — Queue Select State Before Notify

- Logged last value written to Queue Select before TX notify.
- Confirmed it was 1.
- Tried explicit re-select and `-accel tcg`.

**Result:** Still failing; recommended moving to OSDev reference driver comparison or QEMU version check.

---

## 10. N1-PIVOT — Switch to e1000 Driver

**Files:** `drivers/net/e1000.c`, `drivers/net/e1000.h`
- Implemented Intel e1000/82540EM MMIO driver.
- Wired into existing Ethernet/ARP/IP/UDP/DNS stack.

**Result:** TX worked (first RX/TX success). RX completely silent.

---

## 11. N1-PIVOT-FIX — RX Register/Debug

- Enabled BAM/UPE/MPE/promiscuous bits in RCTL.
- Read back RCTL to confirm bits stuck.
- Tried `e1000` vs `nic user,model=e1000`.

**Result:** RX still silent despite correct register config.

---

## 12. N1-PIVOT-FIX-2 — Packet Capture Ground Truth

- Ran QEMU with `-netdev user,id=net0,dump=/tmp/vxair_net.pcap`.
- Verified ARP request left guest and was correctly formatted per RFC 826.
- Confirmed SLIRP sent ARP reply.
- Performed MAC loopback mode test.

**Result:** TX on wire valid, SLIRP replies, but loopback RX also failed → bug in e1000 RX descriptor path.

---

## 13. N1-PIVOT-FIX-3 — RX Descriptor Struct Verification

- Confirmed `e1000_rx_desc_t` layout matches Intel datasheet (16 bytes).
- Added raw 16-byte hex dump of RX descriptor[0] after loopback send.
- Verified `RDLEN == ring_count * sizeof(e1000_rx_desc_t)`.
- Added `volatile` on descriptor status read.

**Result:** Raw descriptor memory remained untouched; struct correct. Bug persisted in init/control sequence.

---

## 14. N1-PIVOT-FIX-4 — Reference Driver Diff

- Compared against known-working JOS e1000 init sequence.
- Matched TIPG, TCTL CT/COLD, MTA clear, RAL/RAH ordering, RCTL bits.

**Result:** RX still silent; sequence correct.

---

## 15. N1-PIVOT-FIX-5 — QEMU e1000 Model / DMA Trace

- Ran QEMU with e1000 trace flags.
- Inspected `info qtree`, `info mtree`, `info pci`.
- Key clue: QEMU trace showed `rx_enabled=0` and `pci_master=0` at packet arrival, even though driver readbacks showed RCTL.EN set and Bus Master re-asserted.

**Result:** Suggested PCI config-write path issue on q35/PCIe.

---

## 16. N1-PIVOT-FIX-6 — PCIe MMCONFIG Command Register

- Investigated ACPI MCFG / MMCONFIG discovery.
- Attempted MMCONFIG read/write of e1000 PCI Command register.
- Legacy PCI config read returned `0x107`; MMCONFIG returned `0xFFFFFFFF`.

**Result:** MMCONFIG path broken in this environment; RX remained blocked.

---

## 17. N1-NEXT — NIC Pivot / RTL8139

**Files:** `drivers/net/rtl8139.c`, `drivers/net/rtl8139.h`
- Implemented RTL8139 I/O-port NIC driver as highest-probability alternative.
- Preserved existing ARP/IP/UDP/DNS stack.

**Result:** Added to codebase. QEMU 11.0.2 networking regression remained prime suspect.

---

## 18. GUI_BASELINE / GUI-FASTTRACK-1 — Restore Working Desktop

**Files:** `kernel/core/src/vxair_main.c`, `gui/compositor/vxair_vxcomp.cpp`, `drivers/gpu/vxair_gop.c/.h`
- Found why GUI was black: compositor was stubbed/wrong path.
- Implemented minimal clear → draw shell → flip pipeline.
- Restored visible desktop, taskbar, launcher, mouse cursor.
- Added safety fallback path.

---

## 19. GUI-FASTTRACK-2 — Premium Dark UI + Fast Mouse

**Files:** `gui/compositor/vxair_vxcomp.cpp`, `kernel/core/src/vxair_main.c`
- Removed rainbow/candy gradients.
- Applied premium dark graphite/navy palette with one disciplined accent.
- Redesigned window chrome, taskbar, launcher with cleaner spacing.
- Made mouse significantly faster via fixed-point sub-pixel precision and acceleration curve.
- Fixed click/draw coordinate sync bugs (taskbar icons, close button, title bar drag).

---

## 20. VXUI Framework Expansion

**Files:** `gui/vxui/vxui.hpp`, `gui/vxui/vxui_theme.hpp`, `gui/compositor/vxair_vxcomp.cpp`, app headers
- Built native VXUI framework: `VxButton`, `VxLabel`, `VxPanel`, `VxTextField`, layouts.
- Added theme tokens, focus/hover/pressed states, event routing.
- Migrated Calculator, Launcher, Settings, Notes, Browser chrome, Terminal surfaces onto VXUI.
- Wired `g_state.accent_color` into framework.

---

## 21. Settings App Revamp

**Files:** `gui/compositor/apps/app_settings.hpp`, `gui/compositor/vxair_vxcomp.cpp`
- Added 5 categories: Input, Appearance, Desktop, Accessibility, System.
- Options added: mouse sensitivity, large cursor, accent color, taskbar style, top bar toggle, desktop glow, window shadows, wallpaper mode, 24-hour clock, show seconds, auto-center windows, focus dimming, high contrast, reset defaults.
- Implemented v2 settings persistence with backward-compatible defaults.
- Removed bright accent bar on title bars (flat neutral title bar).
- Wallpaper modes: Gradient / Dots / None.
- High-contrast border boost.
- Large cursor, clock format, auto-center.

---

## 22. Latest VXUI V3 Accent / Framework Fix

**Files:** `gui/vxui/vxui_theme.hpp`, `gui/vxui/vxui.hpp`, `gui/compositor/vxair_vxcomp.cpp`, `gui/compositor/apps/app_calculator.hpp`
- Replaced warm title-bar gradient (perceived as pink) with flat cool-neutral `SURFACE`/`BASE_DEEP`.
- Made the user-selected accent color apply OS-wide instead of only buttons:
  - Window focused border + outer glow
  - Taskbar accent line
  - Launcher button/menu highlights
  - Cursor tip
  - Top-bar signal bars
  - Calculator display
  - Button focus rings / primary / action buttons
- Added runtime accent helpers: `accent_dim()`, `accent_glow()`, `accent_soft()` derived from live accent via `mix_color()`.
- Moved runtime accent state into `vxui_theme.hpp`; removed duplicate from `vxui.hpp`.
- Neutralized background palette to cool greys.
- Fixed symbol collision by renaming internal `lerp_color` to `mix_color`/`mix_channel`.
- Rebuilt ISO successfully; checksum: `3b4ff9cd2788fcd647dbfd7d446495a99fcc9fec734eddcf4dc321327ede1adf`.

---

## Summary Table

| Milestone | Status | Key Files |
|-----------|--------|-----------|
| B12-B14 Notes/TextInput | ✅ Pass | `app_notes.hpp`, `vxair_vxcomp.cpp` |
| C1 Calculator keyboard | ✅ Pass | `app_calculator.hpp` |
| B15-B20 Selection/Clipboard | ✅ Pass | `vxair_textinput.hpp`, `vxair_vxcomp.cpp`, `app_browser.hpp` |
| C2 Calculator decimals | ✅ Pass | `app_calculator.hpp` |
| C3 Calculator “=” button | ✅ Pass | `app_calculator.hpp` |
| N1 Networking foundation | ⚠️ TX blocked | `drivers/net/virtio_net.c`, `net/**` |
| N1-FIX → N1-FIX-4 | ⚠️ Driver correct, TX still blocked | `drivers/net/virtio_net.c` |
| N1-FIX-2 QEMU flags | ❌ No fix | — |
| N1-FIX-3 PMM addresses | ✅ PMM matched; TX still blocked | `drivers/net/virtio_net.c` |
| N1-FIX-4 Queue Select | ✅ Confirmed 1; TX still blocked | `drivers/net/virtio_net.c` |
| N1-PIVOT e1000 | ✅ TX works, RX silent | `drivers/net/e1000.c` |
| N1-PIVOT-FIX → -6 | ❌ RX still blocked | `drivers/net/e1000.c` |
| N1-NEXT RTL8139 |  Added | `drivers/net/rtl8139.c` |
| GUI baseline restore | ✅ Pass | `vxair_vxcomp.cpp`, `vxair_main.c` |
| GUI-FASTTRACK-2 UI polish | ✅ Pass | `vxair_vxcomp.cpp` |
| VXUI framework | ✅ Pass | `gui/vxui/*`, app headers |
| Settings revamp | ✅ Pass | `app_settings.hpp`, `vxair_vxcomp.cpp` |
| VXUI V3 accent fix | ✅ Pass | `vxui_theme.hpp`, `vxui.hpp`, `vxair_vxcomp.cpp`, `app_calculator.hpp` |

---

## Still-Open Items

1. **Networking RX:** e1000/RTL8139/virtio-net RX remains the largest blocker. Recommended decisive next step: boot a known-good Linux image in the same QEMU 11.0.2 + NIC environment.
2. **`show_close_confirm` setting** is persisted but not wired to any behavior (no modal dialog system yet).
3. **High contrast** currently only boosts focused window border; could be expanded.
4. **Real Qt / framework swap** was discussed; the pragmatic path taken was upgrading VXUI.

---

## How to Boot the Latest Build

```bash
cd ~/Vextryn_Air
qemu-system-x86_64 -cdrom vextryn-air.iso -m 512M -vga std -display sdl -serial stdio -no-reboot
```
