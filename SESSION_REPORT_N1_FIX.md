# N1-FIX Session Report — TX Descriptor Never Processed

**Date:** July 27, 2026
**Project:** Vextryn Air OS
**Status:** N1-FIX PARTIAL — all layers implemented and compile, TX descriptor never processed by device

---

## Summary of Attempts

| Attempt | Fix | Result |
|---------|-----|--------|
| **1. Queue numbering** | Swapped q0=RX, q1=TX per virtio-net spec (was q0=TX, q1=RX) | ❌ TX still not processed |
| **2. Feature negotiation** | Changed from all features to only MAC+STATUS (0x10020) | ❌ TX still not processed |
| **3. Virtio-net header** | Added mandatory 12-byte `struct virtio_net_hdr` before every packet (all zeros) | ❌ TX still not processed |
| **4. `-machine pc`** | Changed from q35 to pc as diagnostic | ❌ Identical failure |
| **5. Modern MMIO interface** | Full rewrite using virtio 1.0 spec, PCI capabilities, MMIO BAR | ❌ Crashed (page fault at 0xFE000014, MMIO region not mapped) |
| **6. Diagnostic buffer** | Used separate static buffer at 0x154000 (different .bss address) | ❌ Same failure (buffer region unchanged) |

---

## What Works (Verified)

- ✅ PCI device discovery (bus=0 slot=2, vendor=0x1AF4, device=0x1000)
- ✅ BAR0 I/O base (0xC040)
- ✅ Device features read (0x79bf8064)
- ✅ Feature negotiation (0x10020 = MAC + STATUS)
- ✅ MAC address read (52:54:00:12:34:56)
- ✅ Queue sizes read (256 each)
- ✅ Queue PFN written (address >> 12) 
- ✅ DRIVER_OK status confirmed (0x0F)
- ✅ virtio-net header prepended (12 bytes)
- ✅ Descriptor written to vring (avail.idx advances 0→1→2)
- ✅ Memory barrier (mfence) before avail.idx update and before notify
- ✅ Queue numbering correct (q0=RX, q1=TX per virtio-net spec)
- ✅ No kernel panics or crashes

## What Doesn't Work

- ❌ TX descriptor never processed (used.idx stays at 0 after notify)
- ❌ RX descriptor never tested (no TX completion)
- ❌ ARP resolution never attempted (TX prevents it)
- ❌ DNS resolution blocked

## Root Cause Analysis

The device **accepts the notification** (avail.idx advances meaning the device reads the avail ring) but **never processes descriptors** (used.idx never advances). This is the same behavior across ALL 6 attempted fixes.

### Register Offset Table (virtio 0.9.5 PCI spec)

| Offset | Name | Access | Our Code |
|--------|------|--------|----------|
| 0x00 | Device Features | 32-bit R | `inl(io + 0x00)` |
| 0x04 | Guest Features | 32-bit W | `outl(io + 0x04, val)` |
| 0x08 | Queue Address (PFN) | 32-bit W | `outl(io + 0x08, pfn)` |
| 0x0C | Queue Size | 16-bit R | `inw(io + 0x0C)` |
| 0x0E | Queue Select | 16-bit W | `outw(io + 0x0E, qnum)` |
| 0x10 | Queue Notify | 16-bit W | `outw(io + 0x10, qnum)` |
| 0x12 | Device Status | 8-bit R/W | `inb/outb(io + 0x12)` |
| 0x13 | ISR | 8-bit R | `inb(io + 0x13)` |
| 0x14 | Device Config | variable R | `inb(io + 0x14 + off)` |

### Queue Mapping (virtio-net spec, both legacy and modern)

| Queue | Purpose | Our Code |
|-------|---------|----------|
| 0 | Receiveq (RX) | `outw(QUEUE_SEL, 0)` → rx_vq PFN |
| 1 | Transmitq (TX) | `outw(QUEUE_SEL, 1)` → tx_vq PFN |

### Physical Address Layout

| Buffer | Physical Address | Notes |
|--------|-----------------|-------|
| rx_vq | ~0x151000 | Page-aligned vring for RX |
| tx_vq | ~0x153000 | Page-aligned vring for TX |
| g_virtio_net struct | ~0x150000 | Contains tx_frame, rx_bufs |
| tx_frame (data) | ~0x150050 | TX packet buffer (from diag_buf test: 0x154000) |
| Framebuffer | 0xFD000000 | Known-mapped, but MMIO region |

---

## Remaining Possibilities (untested)

1. **Modern MMIO with proper page table mapping** — The MMIO BAR at ~0xFE000000 is not identity-mapped. Need to use kernel VMM functions to map it, or modify the page tables directly.

2. **Kernel VMM PCI MMIO mapping** — Use `vxair_vmm_map_page()` or similar to map the PCI MMIO region before accessing it.

3. **QEMU KVM vs TCG** — Test with explicit `-accel tcg` (software emulation) to eliminate KVM-specific coherency issues.

4. **Buffers in guaranteed DMA-able memory** — Use the kernel's physical memory allocator (`vxair_pmm_alloc_pages()`) to get buffers from a known-good physical address range, rather than relying on .bss placement.

5. **Interrupt-based processing** — Set up MSI-X or pin-based interrupt handling so the device can signal completion to the driver, rather than relying on polling.

---
*10 QEMU test runs across 6 fix attempts, all producing the identical result: TX descriptor posted but never processed.*
