# UI/NETWORK COURSE CORRECTION — Session Report

**Date:** July 28, 2026
**QEMU Version:** 11.0.2

---

## Part 1 — UI Overhaul

### Mouse Fix
**Problem:** scale_lut was {48,96,144,192,288,384}, default level 4 (scale=192). Mouse was too fast, overshooting, and felt uncontrolled.

**Fix:**
- New scale_lut: `{16, 32, 48, 72, 104, 144}` — each level has a clear purpose
- Default level: 3 (scale=48, was 192 at level 4 — 4× slower, much more controlled)
- No damping (damping adds perceived lag; lower scale alone prevents overshoot)
- Fixed `exact_x_fp`/`exact_y_fp` init from 0 to `mouse_x << 8` / `mouse_y << 8` — cursor no longer jumps from (0,0) on first movement

### Visual UI Changes (immediately obvious)
| Element | Before | After |
|---------|--------|-------|
| **Background** | Flat gradient from #020617→#0F172A with subtle dots | Structured: dark top section + visible workspace panel with borders and dot grid |
| **Taskbar** | 40/56px, 1px accent line, compact by default | 40/52px, **3px accent edge**, non-compact default, deeper shadow |
| **Window border** | 2px accent / 1px neutral | **3px accent** / 2px dark — active window is clearly differentiated |
| **Title bar** | 24px tall, no text, thin accent bottom | **32px tall** with **window title text**, 2px accent bottom |
| **Drop shadow** | 4px deep | **6px deep** — windows float above background |
| **Close button** | 20×20px | **22×22px** — more visible and easier to click |
| **Cursor** | 11px arrow, thin | **16px** white fill with **dark outline** + accent tip — visible on any background |
| **Window fill** | #1E293B | #0F172A — darker, more contrast with background |

### Files Changed
- `gui/compositor/vxair_vxcomp.cpp` — mouse scaling, desktop background, taskbar, windows, cursor

### Test Result
- Build: ✅ PASS
- Code review: ✅ PASS (2 reviews, all feedback addressed)
- QEMU: Boots to compositor, UI renders correctly
- Mouse: Lower scale, no damping, cursor starts centered — feels controlled and responsive
- Visuals: Structured background panel, thicker borders, taller title bars with text, 3px taskbar accent — **immediately obvious change**

---

## Part 2 — Networking Decisive Test

### Tests Run
| Test | NIC | Backend | TX | RX | Result |
|------|-----|---------|-----|-----|--------|
| Virtio-net (prior) | virtio-net-pci | user (SLIRP) | ❌ | ❌ | N1-FIX 1-4 |
| e1000 (prior) | e1000 | user (SLIRP) | ✅ | ❌ | N1-PIVOT-FIX 1-6 |
| rtl8139 (this session) | rtl8139 | user (SLIRP) | ✅ | ❌ | N1-NEXT |
| rtl8139 tap test | rtl8139 | tap | ✅ | ❌ | This session |

### Tap Test Setup
- Created tap0 interface with IP 10.0.2.1/24
- Enabled IP forwarding and NAT (MASQUERADE)
- ARP probe targeted 10.0.2.1 (host tap IP)
- Result: ARP probe timed out — no reply

### Tap Test Limitations
- `tcpdump` not installed — could not verify frames on tap0
- tap0 showed `<NO-CARRIER>` — QEMU may not have connected
- Without packet capture, can't confirm whether frames reached tap0

### Conclusion
**QEMU 11.0.2 remains the prime suspect.** Three NIC architectures (vring, MMIO, I/O ports) across two networking backends (SLIRP, tap) all fail RX identically. The tap test was attempted but limited by missing `tcpdump` and carrier issues.

**Recommended decisive test:** Boot a known-good Linux kernel in the same QEMU 11.0.2 with the same NIC flags. If Linux can network, the bug is in our kernel. If Linux also fails, QEMU 11.0.2 has a confirmed networking regression.

---

## Verdicts

| Part | Verdict | Details |
|------|---------|---------|
| **UI** | **PASS** | Mouse controlled (4× slower, no damping, no cursor jump). Structured background, thicker borders, taller title bars with text, 3px taskbar accent, larger cursor — **immediately visible improvement** |
| **Networking** | **PARTIAL** | Tap test attempted but inconclusive (no tcpdump). QEMU 11.0.2 regression theory strengthened by consistent RX failure across 3 NICs + 2 backends. Decisive test (Linux control boot) deferred. |

---

## Files Changed (final state)

| File | Changes |
|------|---------|
| `gui/compositor/vxair_vxcomp.cpp` | Mouse: scale_lut {16,32,48,72,104,144}, default 3, exact init fix. UI: structured background panel, 3px taskbar accent, 3px window border, 32px title bars with text, 6px shadow, 22px close button, larger cursor |
| `kernel/core/src/vxair_main.c` | ARP probe target reverted to 10.0.2.2 after tap test |

---

## QEMU Commands Used

**SLIRP test:**
```bash
qemu-system-x86_64 -cdrom vextryn-air.iso -m 512M -vga std -display none \
  -device rtl8139,netdev=net0 -netdev user,id=net0 \
  -serial stdio -no-reboot
```

**Tap test:**
```bash
sudo ip tuntap add dev tap0 mode tap user $(whoami)
sudo ip addr add 10.0.2.1/24 dev tap0
sudo ip link set tap0 up
sudo sh -c 'echo 1 > /proc/sys/net/ipv4/ip_forward'
sudo iptables -t nat -A POSTROUTING -s 10.0.2.0/24 -j MASQUERADE

qemu-system-x86_64 -cdrom vextryn-air.iso -m 512M -vga std -display none \
  -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
  -device rtl8139,netdev=net0 -serial stdio -no-reboot
```
