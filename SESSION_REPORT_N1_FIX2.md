# N1-FIX-2 Session Report — QEMU Device Flag Tests

**Status:** N1-FIX-2 FAIL — both legacy-only and modern-only QEMU flags fail

## Test Results

### Test 1: `disable-legacy=off,disable-modern=on` (force legacy-only)
- Device found ✅
- BAR0 I/O base (0xC040) ✅
- Init OK ✅
- TX: `NOT done (used=0)` ❌ (identical to default flags)

### Test 2: `disable-legacy=on,disable-modern=off` (force modern-only)
- Device found ✅
- `BAR0 not I/O` ❌ (modern-only mode uses MMIO BAR, not I/O BAR)
- Init FAILED (-1)
- No TX attempted

## Conclusion

The issue is confirmed **not** related to QEMU device mode selection. With legacy-only mode, the init works perfectly but TX descriptors never get processed. With modern-only mode, the legacy I/O driver can't init at all (BAR0 is MMIO).

## Remaining Options (not yet tried)

1. **Map PCI MMIO in kernel page tables → use modern MMIO driver** — The modern MMIO implementation crashed with page fault at 0xFE000014 because the kernel doesn't identity-map the PCI MMIO region. Fixing this would allow the modern interface to work, which uses direct MMIO writes for notifications instead of I/O ports.

2. **Use `-accel tcg`** — Test without KVM acceleration to eliminate potential KVM-specific coherency issues with the legacy I/O interface.

3. **Allocate DMA buffers from kernel PMM** — The .bss section may not be DMA-safe. Use `vxair_pmm_alloc_pages()` to get buffers from guaranteed DMA-able physical pages.

## Required QEMU Command for All Future Tests

For legacy I/O mode (current driver):
```
-device virtio-net-pci,disable-legacy=off,disable-modern=on,netdev=net0
```

For modern MMIO mode (once kernel mapping is fixed):
```
-device virtio-net-pci,netdev=net0
```
(Default transitional mode with both legacy and modern available)
