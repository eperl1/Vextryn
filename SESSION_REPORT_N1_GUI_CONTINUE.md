# SESSION REPORT — N1-GUI-CONTINUE

**Status: N1-GUI-CONTINUE PARTIAL**
- All structural fixes applied and building cleanly
- RSDP scan fallback works (ACPI init: ✅)
- MCFG table not found despite ACPI init working (blocker for MMCONFIG)
- e1000 RXDCTL readback still 0x0 (separate issue)

---

## Summary of Root Causes Found

### 1. ACPI NEVER Initialized (Primary Blocker)
`vxair_hal_acpi_init()` was **never called** anywhere in the boot sequence. The RSDP address was stored in `boot_info.rsdp_address` by the UEFI bootloader, but:
- When booting via GRUB (multiboot2), the RSDP is not populated by the multiboot2 stub (it only handles tag type 8 = framebuffer)
- Even the UEFI boot path never called `vxair_hal_acpi_init()`
- All ACPI table lookups (MCFG, HPET, FACP) silently returned NULL

### 2. MMIO Pages Mapped Without Cache-Disable
Both the e1000 MMIO region and MMCONFIG region were mapped using only `VXAIR_VMM_PRESENT | VXAIR_VMM_RW`. The PCD (Page-level Cache Disable) bit was not set, meaning CPU caches could buffer MMIO writes — preventing them from reaching device registers. The framebuffer works because VGA framebuffer memory is backed by real RAM and doesn't need strict uncacheable access.

### 3. MMCONFIG Mapping Too Small
The MMCONFIG identity mapping covered only 256KB (0x40000), which can only cover devices 0-7 on bus 0. Expanded to 1MB (0x100000) to cover all 32 devices on bus 0.

### 4. mmcfg_addr Operator Precedence
The MMCONFIG address computation used mixed `+` and `|` operators with no explicit grouping. While both produce the same result for aligned MMCONFIG bases (lower 20 bits clear), this is fragile. Changed to all `+` with explicit `(uint32_t)` casts for bus subtraction.

### 5. Static→Extern Linkage Bug
`mmcfg_addr` was declared `static inline` in `hal_pci.c`, but `e1000.c` tried to call it via an `extern` declaration. `static inline` gives internal linkage — no external symbol is emitted. Fixed by adding a public wrapper API.

---

## Files Changed (6 files)

```
kernel/core/include/vxair_vmm.h     | 1 +
kernel/core/src/vxair_main.c        | 17 ++++++++++++++++-
kernel/hal/hal_acpi.h               | 10 ++++++++++
kernel/hal/hal_acpi.c               | 39 ++++++++++++++++++++++++++++++++++-----
kernel/hal/hal_pci.h                |  4 +++-
kernel/hal/hal_pci.c                | 55 ++++++++++++++++++++++++++++++++++++++++++-----------
drivers/net/e1000.c                 | 14 ++++++++++----
7 files changed, 123 insertions(+), 24 deletions(-)
```

### Fix 1: VXAIR_VMM_NOCACHE flag
**File:** `kernel/core/include/vxair_vmm.h`
Added `#define VXAIR_VMM_NOCACHE (1 << 4)` — bit 4 in x86-64 page tables is PCD (Page-level Cache Disable). With PWT=0 (default), this gives UC (Uncacheable) memory type, correct for device MMIO.

### Fix 2: ACPI init in boot sequence
**File:** `kernel/core/src/vxair_main.c`
Added ACPI initialization early in boot (after memory init, before APIC). First tries `multiboot_info->rsdp_address` (bootloader-provided), then falls back to `vxair_hal_acpi_scan_rsdp()` (memory scan).

### Fix 3: RSDP memory scan fallback
**Files:** `kernel/hal/hal_acpi.h`, `kernel/hal/hal_acpi.c`
Added `vxair_hal_acpi_scan_rsdp()` which scans physical memory 0xE0000–0xFFFF0 in 16-byte steps for the "RSD PTR " signature. Validates both v1 (20-byte) and v2 (36-byte) checksums. Uses the revision field (byte 15) for version detection.

### Fix 4: MMCONFIG path overhaul
**Files:** `kernel/hal/hal_pci.h`, `kernel/hal/hal_pci.c`
- MCFG discovery: logs table presence, number of entries, and per-entry details (base, segment, bus range)
- MMCONFIG mapping: uses `VXAIR_VMM_NOCACHE` flag, expanded from 256KB to 1MB
- Fixed `mmcfg_addr`: uses `+` consistently, adds `(uint32_t)` casts for bus subtraction
- Added public API: `vxair_hal_pci_mmconfig_calc_addr()`, `vxair_hal_pci_mmconfig_is_ready()`

### Fix 5: e1000 NOCACHE mapping + diagnostic
**File:** `drivers/net/e1000.c`
- Changed MMIO page flags from `PRESENT|RW` to `PRESENT|RW|NOCACHE`
- Added diagnostic: logs MMCONFIG physical address, ready status, PCI DEVID/CMD comparison

---

## QEMU Test Results

### ✅ Working
| Component | Result |
|-----------|--------|
| **RSDP scan** | Found at 0xf64f0 — ACPI initialized via memory scan |
| **e1000 MMIO mapping** | `mapping at 0xfebc0000 size=0x6000 (flags=PRESENT|RW|NOCACHE)` |
| **Compositor** | `COMPOSITOR FRAME 420` reached — fast boot, desktop stable |
| **Legacy PCI config** | DEVID=0x100E8086, CMD=0x107 (correct) |

### ❌ Still Failing
| Component | Result | Analysis |
|-----------|--------|----------|
| **MCFG discovery** | `PCIE: MCFG table not found` | ACPI init succeeded but MCFG not in table list. Likely cause: ACPI table pointers (XSDT entries) point to physical addresses above the 1GB identity-mapped range — the multiboot2 stub only maps 0-1GB (512 × 2MB pages). ACPI tables on QEMU with 512MB RAM may be placed above 0x40000000. |
| **MMCONFIG reads** | All 0xFFFFFFFF | Direct consequence of MCFG not found |
| **RXDCTL write** | Wrote=0x01010101, Readback=0x00000000 | NOCACHE alone didn't fix. Possible causes: (1) register is read-only/stubbed in QEMU's 82540EM model, (2) different register offset needed, (3) requires preceding init steps |
| **PCI CMD match** | match=0 | MMCONFIG returns 0xFFFFFFFF (no device); legacy works |

---

## Recommendations for Next Session

### Priority 1: Debug ACPI Table Access
Add diagnostic logging in `vxair_hal_acpi_init()` to print:
- XSDT/RSDT physical address
- First 4 XSDT entry addresses (as hex)
- Whether the memory at those addresses is readable (does it look like an ACPI header?)
- This will confirm whether the identity mapping covers the ACPI tables

### Priority 2: Map More Physical Memory if Needed
If ACPI tables are above 1GB, extend the identity mapping in the multiboot2 stub or in `vxair_kernel_main` to cover all physical RAM (at least up to 4GB for 32-bit I/O region).

### Priority 3: Investigate RXDCTL Readback
If MMCONFIG is fixed and RXDCTL still reads 0x0:
- Verify the register offset against the specific device ID (0x100E = 82540EM)
- Try reading RXDCTL before and after the write (no-op read to check if register exists)
- Consider that RXDCTL may not be the root cause — the broader RX init sequence (RCTL, RAL/RAH, RDBAL/RDBAH) might already be correct and the RX path issue could be elsewhere (e.g., DMA buffer coherency, descriptor format)
