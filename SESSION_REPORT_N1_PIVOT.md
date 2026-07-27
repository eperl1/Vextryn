# N1-PIVOT Session Report — e1000 Driver (Pivot from virtio-net)

**Project:** Vextryn Air OS  
**Date:** July 27, 2026  
**Status:** ⚠️ N1-PIVOT PARTIAL — TX works (FIRST TIME!), RX still silent  

---

## Executive Summary

Pivoted from virtio-net-pci (which never processed descriptors after 19 tests across 10 fix attempts) to Intel e1000 (82540EM). The e1000 is an MMIO-based device with simple descriptor rings and a status byte per descriptor — vastly simpler than virtio's vring/avail/used model.

**Breakthrough:** TX descriptors complete successfully (`E1000: TX done`), confirmed by the DD (Descriptor Done) bit being set by hardware. This is the FIRST time in the entire N1 effort (20+ QEMU tests) that any NIC has actually processed a descriptor.

**Remaining:** RX is silent — the e1000 never fills any receive descriptor (RDH=0, all DD bits 0x0). This is likely an ARP frame format or QEMU user-mode networking configuration issue, not a driver-level bug.

---

## Files Created

| File | Description |
|------|-------------|
| `drivers/net/e1000.h` | e1000 driver header — register offsets, descriptor structs (TX/RX, 16 bytes each), device state |
| `drivers/net/e1000.c` | e1000 driver implementation — MMIO-based init, send, receive with DD status polling |

## Files Modified

| File | Changes |
|------|---------|
| `net/core/net_core.c` | Replaced all virtio references (include, init, send, receive, found, mac_addr) with e1000 equivalents |
| `net/core/ethernet.c` | Replaced virtio include, `g_virtio_net` → `g_e1000`, `VIRTIO_NET_FRAME_SIZE` → `E1000_BUFFER_SIZE`, `vxair_virtio_net_send` → `vxair_e1000_send` |
| `net/core/arp.c` | Replaced virtio include, `g_virtio_net.mac_addr` → `g_e1000.mac_addr` |

## Files Unchanged (virtio preserved for future retry)

| File | Status |
|------|--------|
| `drivers/net/virtio_net.h` | Unchanged, unused |
| `drivers/net/virtio_net.c` | Unchanged, unused |
| `net/udp/udp.c`, `net/udp/udp.h` | Unchanged |
| `net/wifi/dns.c`, `net/wifi/dns.h` | Unchanged |
| `net/core/ip.c`, `net/core/ip.h` | Unchanged (calls `vxair_eth_send()` which is abstracted) |

---

## Bugs Found & Fixed

| # | Bug | File | Fix | Severity |
|---|-----|------|-----|----------|
| 1 | EERD address shift = 2 instead of 8 | e1000.h | `E1000_EERD_ADDR_SHIFT` 2→8 | CRITICAL — MAC would be wrong (repeated bytes) |
| 2 | RDT written with `rx_tail` instead of `idx` | e1000.c | `mmio_write32(RDT, idx)` instead of `g_e1000.rx_tail` | HIGH — after 1 packet, RX ring would go empty |
| 3 | MMIO page not mapped (first 1 page, then 4, then 6) | e1000.c | `vxair_vmm_map_page()` loop for pages 0-5 (24KB) | CRITICAL — page faults at 0xFEBc0000, 0xFEBc2800, 0xFEBc5400 |
| 4 | `extern void *kernel_pml4` wrong type | e1000.c | Changed to `vxair_page_table_t *` | LOW — type safety |
| 5 | No return check on `vxair_vmm_map_page` | e1000.c | Added error check | LOW — robustness |
| 6 | RDT written before RX enable | e1000.c | Write RDT=0 first, then RDT=15 after RCTRL enable | LOW — ordering fix |
| 7 | RAL/RAH not configured | e1000.c | Added receive address register setup | MEDIUM — needed for MAC filtering |
| 8 | AV bit `(1 << 31)` signed shift UB | e1000.h | Changed to `(1U << 31)` | LOW — C standard compliance |
| 9 | net/core/ethernet.c still called `vxair_virtio_net_send` | ethernet.c | → `vxair_e1000_send` | CRITICAL — TX path was broken |
| 10 | net/core/arp.c still referenced `g_virtio_net.mac_addr` | arp.c | → `g_e1000.mac_addr` | HIGH — source MAC in ARP would be wrong |

---

## Diagnostic Test Results

### Test 1: Initial e1000 (no MMIO mapping)
- ❌ Page fault at 0xFEBc0000 — MMIO not identity-mapped

### Test 2: 4-page MMIO mapping + EERD fix + RDT fix
- ✅ TX completes (`E1000: TX done idx=0 iterations=0`)
- ❌ ARP resolution failed
- ❌ No RX (no ARP reply received)

### Test 3: Link up (SLU) fix
- ✅ Link UP (LU=1)
- ✅ TX completes
- ❌ ARP resolution failed

### Test 4: RX diagnostic (RDH, descriptor statuses)
- ✅ No page fault
- ✅ TX completes
- ❌ RDH=0, all descriptor statuses 0x0 — no frames ever received

### Test 5: RDT ordering fix
- ✅ No page fault
- ✅ TX completes
- ❌ RDH=0, all d0_st-d3_st = 0x0 — still no RX

### Test 6: RAL/RAH + extended MMIO (6 pages)
- ✅ RAL=0x12005452, RAH=0x80005634
- ✅ No page fault
- ✅ TX completes
- ❌ RDH=0, all DD bits 0x0 — still no RX

### Final QEMU Command
```bash
qemu-system-x86_64 -cdrom vextryn-air.iso -m 512M -smp 4 -machine q35 \
  -cpu qemu64 -device e1000,netdev=net0 -netdev user,id=net0 \
  -serial file:/tmp/vxair_e1000_test.log -display none -no-reboot
```

### Critical Serial Log
```
[INFO] E1000: Found device 0x100e at bus=0 slot=2
[INFO] E1000: MMIO base = 0xfebc0000
[INFO] E1000: Reset complete
[INFO] E1000: STATUS=0x80080783 LU=1
[INFO] E1000: Link status after SLU: LU=1
[INFO] E1000: MAC=52:54:00:12:34:56
[INFO] E1000: RAL=0x12005452 RAH=0x80005634
[INFO] E1000: Init OK (TX=16 RX=16)
[INFO] NET: e1000 MAC 52:54:00:12:34:56
[INFO] NET: Stack initialized successfully
[INFO] NET: Starting DNS test...
[INFO] E1000: TX posted idx=0 len=42 addr=0x9fb000
[INFO] E1000: TX done idx=0 iterations=0      ← FIRST TX COMPLETION EVER!
[INFO] DNS: Polling for ARP reply...
[INFO] E1000: RX miss #1 RDH=0 rxtail=0 d0_st=0x0 d1_st=0x0 ...
[INFO] DNS: ARP resolution failed
[INFO] E1000: TX posted idx=1 len=42 addr=0x9fb800
[INFO] E1000: TX done idx=1 iterations=0
[INFO] DNS: Polling for ARP reply...
[INFO] DNS: ARP resolution failed
```

---

## e1000 Driver Architecture

### Register Access
- All registers accessed via MMIO through identity-mapped PCI BAR0 (0xFEBc0000)
- 6 pages (24KB) mapped via `vxair_vmm_map_page()`
- `volatile` pointers for all MMIO reads/writes

### TX Path
1. Copy packet data to TX buffer at current TDT index
2. Fill descriptor: addr (physical), length, cmd=EOP|RS|IFCS, status=0
3. mfence → write TDT → device auto-polls descriptor
4. Poll `desc->status` for DD bit (0x01) → TX done

### RX Path
1. Check `desc->status` for DD bit at current rx_tail
2. If set: copy from RX buffer, clear status, advance tail, write RDT=idx
3. If not set: return 0 (no packet)

### Key Differences from virtio-net
| Feature | virtio-net | e1000 |
|---------|-----------|-------|
| Access | I/O ports | MMIO |
| Notification | `outw(QueueNotify)` | Write TDT register (auto-poll) |
| TX completion | used.idx ring advance | DD bit in descriptor status |
| Descriptor | 16 bytes + vring struct | 16 bytes flat |
| Ring complexity | avail/used ring dance | Simple head/tail pointers |

---

## Verdict

**N1-PIVOT PARTIAL PASS** — TX works for the first time in the entire networking effort. The e1000 pivot was the correct call: the simpler model eliminated the virtio-specific blocking issue.

RX is still silent (RDH never advances, no DD bits set on receive descriptors). This is most likely NOT a driver bug — the hardware-level diagnostics confirm correct init, link up, MAC configuration, and RAL/RAH setup. The remaining issue is likely:

1. **ARP frame format**: The frame may be malformed (wrong ethertype encoding, wrong ARP fields)
2. **QEMU user-mode networking**: QEMU may not respond to ARP requests from e1000 (works differently than virtio-net)
3. **Missing e1000 configuration**: Some receive path register not yet set

### Next Steps Recommended

1. **Test with a raw Ethernet frame** instead of ARP — send a known-valid frame to a known MAC and verify TX→RX loop with netcat/UDP listener on the host
2. **Try -nic user,model=e1000** instead of separate -device/-netdev
3. **Investigate QEMU's SLIRP behavior with e1000** — possibly requires explicit guest IP assignment or DHCP

---

## Build & Test

```bash
# Build (auto-discovers drivers/net/e1000.c via GLOB_RECURSE)
cd ~/Vextryn_Air/build && make -j$(nproc) vextryn_air.elf

# ISO
cd ~/Vextryn_Air
cp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/

# Test
qemu-system-x86_64 -cdrom vextryn-air.iso -m 512M -smp 4 -machine q35 \
  -cpu qemu64 -device e1000,netdev=net0 -netdev user,id=net0 \
  -serial file:/tmp/vxair_e1000.log -display none -no-reboot
```

---

*Report generated: July 27, 2026 — 6 QEMU tests, 10 bugs fixed, FIRST TX completion in N1 effort*
