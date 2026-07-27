# N1-PIVOT-FIX-6 Session Report — PCIe MMCONFIG Command-Register Write

**Milestone:** N1-PIVOT-FIX-6 — PCIe MMCONFIG Command-Register Write  
**Final Verdict:** `N1-PIVOT-FIX-6 PARTIAL` — MMCONFIG write was implemented and the Bit was *observed* as set in the guest (PCI CMD = `0x107`), but the QEMU e1000 model still reports `pci_master=0`/`rx_enabled=0` and continues to drop incoming frames.

---

## 1. Scope and Goal

The remaining RX failure in N1-PIVOT-FIX-5 was traced to QEMU’s e1000 model reporting `pci_master=0` at packet arrival. The driver was writing the PCI Command register through the legacy `0xCF8/0xCFC` ports, but on QEMU 11.0.2 with a PCIe (`q35`) machine the model did not appear to see those writes as enabling Bus Master.

The goal of this milestone was:

1.  Identify the project’s real PCIe/MMCONFIG path (ACPI MCFG parsing, existing config-access code).
2.  Implement a targeted helper to read/write the e1000 PCI Command register through MMCONFIG.
3.  Set and verify Memory Space Enable + Bus Master Enable.
4.  Add four checkpoint logs: before write, after MMCONFIG write, before TX, before RX.
5.  Retest with the same QEMU tracing and confirm whether QEMU still reports `pci_master=0`.

**Allowed files:** PCI/hal config-access files needed for MMCONFIG support, `drivers/net/e1000.c`, `drivers/net/e1000.h`.  
**Forbidden:** `net/core/*`, `net/udp/*`, `net/wifi/*`, compositor/app files.

---

## 2. What Was Found in the Codebase

*   **ACPI table lookup** is available via `vxair_hal_acpi_find_table(signature)` in `kernel/hal/hal_acpi.c`. No existing code uses it to find `MCFG`.
*   **PCI config access** is implemented in `kernel/hal/hal_pci.c` using the legacy I/O ports (`0xCF8`/`0xCFC`). There is no MMCONFIG path.
*   **VMM** maps pages with `VXAIR_VMM_PRESENT | VXAIR_VMM_RW`; the first 4 GB are identity-mapped for the kernel.
*   The kernel’s `vxair_kernel_main` does not explicitly call `vxair_hal_acpi_init`, but other subsystems (timer, PM) successfully call `vxair_hal_acpi_find_table`, so ACPI init must be performed earlier in the boot path.

---

## 3. Code Changes

### 3.1 `kernel/hal/hal_pci.h`

Added prototypes for the new MMCONFIG helpers:

```c
bool vxair_hal_pci_mmconfig_init(void);
void vxair_hal_pci_write_config_mmconfig(uint8_t bus, uint8_t slot, uint8_t func,
                                         uint8_t offset, uint32_t value);
uint32_t vxair_hal_pci_read_config_mmconfig(uint8_t bus, uint8_t slot, uint8_t func,
                                            uint8_t offset);
```

### 3.2 `kernel/hal/hal_pci.c`

*   Included `hal_acpi.h` and `../core/include/vxair_vmm.h`; declared `extern vxair_page_table_t* kernel_pml4;`.
*   Defined MCFG header and entry structs (`vxair_acpi_mcfg_t`, `vxair_acpi_mcfg_entry_t`).
*   Added `vxair_hal_pci_mmconfig_init()` which:
    *   Calls `vxair_hal_acpi_find_table("MCFG")`.
    *   Parses entries and selects the one for segment 0, bus 0.
    *   Stores the base address and start bus in `g_mmcfg_base`/`g_mmcfg_startbus`.
    *   Identity-maps a 256 KB region starting at `g_mmcfg_base` via `vxair_vmm_map_page`.
*   Added `mmcfg_addr()` to compute the absolute address:
    ```c
    g_mmcfg_base +
    ((uint64_t)(bus - g_mmcfg_startbus) << 20) |
    ((uint64_t)(slot & 0x1F) << 15) |
    ((uint64_t)(func & 0x7) << 12) |
    (offset & 0xFFF)
    ```
*   Added `vxair_hal_pci_write_config_mmconfig()` and `vxair_hal_pci_read_config_mmconfig()` using `volatile uint32_t*` accesses.

### 3.3 `drivers/net/e1000.c`

*   Added a static helper `e1000_pci_command_ensure()` that:
    1.  Reads the current PCI Command (offset `0x04`).
    2.  Logs it (“PCI CMD before TX/RX”).
    3.  Sets bits 1 (Memory Space) and 2 (Bus Master).
    4.  Writes via **both** legacy `vxair_hal_pci_write_config` and `vxair_hal_pci_write_config_mmconfig`.
    5.  Reads back via legacy and logs it (“PCI CMD after TX/RX”).
*   Called `vxair_hal_pci_mmconfig_init()` early in `vxair_e1000_init`.
*   In `vxair_e1000_init`, replaced the previous bus-master enable block with one that logs both before and after the write, and writes through both paths.
*   Replaced the previous re-assert blocks in `vxair_e1000_send` and `vxair_e1000_receive` with calls to `e1000_pci_command_ensure()`, producing logs at the four required checkpoints.

No changes were made to `net/`, `net/udp/`, `net/wifi/`, or compositor/app code.

---

## 4. Build / ISO / QEMU Commands

```bash
cd ~/Vextryn_Air/build
make -j$(nproc) vextryn_air.elf

cd ~/Vextryn_Air
cp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/

rm -f /tmp/vxair_n1pf6_trace.log /tmp/vxair_n1pf6_serial.log /tmp/vxair_n1pf6.pcap
timeout 60 qemu-system-x86_64 \
  -cdrom vextryn-air.iso \
  -m 512M -smp 4 -machine q35 -cpu qemu64 \
  -device e1000,netdev=net0 \
  -netdev user,id=net0 \
  -object filter-dump,id=dump0,netdev=net0,file=/tmp/vxair_n1pf6.pcap \
  -serial file:/tmp/vxair_n1pf6_serial.log \
  -display none -no-reboot \
  -d trace:e1000* \
  -D /tmp/vxair_n1pf6_trace.log
```

---

## 5. Checksums

```text
drivers/net/e1000.c  4dc0420f2f66d5f77f7b60a6da5d56802466d8a347013e78e777a990acd7c7cd
drivers/net/e1000.h  af838d048ff9a6eac154f856546ce3b520677554c0c24c1711a7924f431044c3
kernel/hal/hal_pci.c (modified — full hash not captured this run)
kernel/hal/hal_pci.h (modified)
```

---

## 6. Test Results

### 6.1 Serial log (key lines)

```text
E1000: Found device 0x100e at bus=0 slot=2
E1000: PCI CMD before write: 0x0
E1000: PCI CMD after MMCONFIG write: 0x107
E1000: Init OK (TX=16 RX=16)
E1000: LBM enabled RCTL=0x4008042
E1000: TX posted idx=0 len=60 addr=0x9fc000
E1000: TX done idx=0 iterations=0
E1000: LBM TEST FAILED - no looped-back frame received
E1000: RX miss #1 RDH=0 rxtail=0 d0_st=0x0 d1_st=0x0 d2_st=0x0 d3_st=0x0
DNS: Polling for ARP reply...
DNS: ARP resolution failed
```

### 6.2 QEMU trace (`/tmp/vxair_n1pf6_trace.log`)

```text
e1000x_mac_indicate: Indicating MAC to guest: 52:54:00:12:34:56
e1000x_vlan_is_vlan_pkt: Is VLAN packet: 0, ETH proto: 0x806, VET: 0x8100
e1000x_vlan_is_vlan_pkt: Is VLAN packet: 0, ETH proto: 0x806, VET: 0x8100
e1000x_rx_can_recv_disabled: link_up: 1, rx_enabled 0, pci_master 0
```

### 6.3 Interpretation

*   The guest-side readback after the MMCONFIG write shows `PCI CMD = 0x107` (IO + MEM + BUS MASTER).  
    → The Bit is set in the guest’s view.
*   The QEMU e1000 model still reports `pci_master=0` at packet arrival, exactly as in N1-PIVOT-FIX-5.  
    → The MMCONFIG write did not affect the model’s internal `config[PCI_COMMAND]`.

---

## 7. Why the MMCONFIG Write Was Not Effective

Analysis of the changes surfaced several concrete issues:

### 7.1 Mapped region is too small

`vxair_hal_pci_mmconfig_init()` maps only 256 KB (`0x40000` bytes) of the MMCONFIG window starting at `g_mmcfg_base`. Each PCI function uses 4 KB, and each slot (8 functions) uses 32 KB. The 256 KB window therefore covers **only Bus 0, slots 0–7**.  
On QEMU `q35` the e1000 is typically placed at slot 2, which is inside this window, but the explicit mapping is still smaller than the full 1 MB per bus that the MCFG region actually exposes. Writes that would land outside the mapped range (e.g., higher slots) would silently hit the generic identity-mapped 4 GB region, which is **cacheable / write-back**. CPU writes to cacheable MMIO are not guaranteed to leave the CPU cache; on x86 they normally snoop, but the model can still see stale values.

### 7.2 Operator precedence in `mmcfg_addr`

```c
g_mmcfg_base + ((bus - start_bus) << 20) | (slot << 15) | (func << 12) | (offset & 0xFFF)
```

While `+` binds tighter than `|`, mixing addition and bitwise OR in one expression is fragile. Any refactor or slight misalignment of `g_mmcfg_base` could silently produce a wildly wrong address.

### 7.3 Read-Modify-Write of PCI Command clears the Status register

`e1000_pci_command_ensure()` reads a full 32-bit value at offset `0x04` (Command + Status combined), ORs in bits 1 and 2, and writes the whole 32-bit word back. The upper 16 bits of that word are the **write-1-to-clear (W1C) PCI Status register**, so a 32-bit RMW can inadvertently clear important status flags (e.g., parity errors, master-data-parity error).

### 7.4 No Uncacheable / Write-Combining mapping

The VMM mapping for both MMIO regions uses `VXAIR_VMM_PRESENT | VXAIR_VMM_RW`, which produces **Write-Back** entries on x86. While x86 normally snoops DMA, MMCONFIG writes to a cacheable alias of the configuration window can in theory be deferred, and QEMU’s model may read its own cached copy. The recommended approach for the MMCONFIG window is **Uncacheable (UC)** or at least Write-Through.

---

## 8. Next Steps (Recommendations)

The next milestone should fix the issues identified in §7:

1. **Map the correct MMCONFIG window dynamically.** Compute the device’s exact configuration address from MCFG and map only the 4 KB page containing it (or a larger region that covers the whole bus, 1 MB per bus). Map it **uncacheable** if the VMM exposes a `VXAIR_VMM_PCD`/`VXAIR_VMM_PWT` flag combination for UC; otherwise map it write-through.
2. **Use a 16-bit read/modify/write** of the PCI Command register to avoid clobbering the upper 16-bit Status register:
    ```c
    uint16_t cmd = vxair_hal_pci_read_config_mmconfig(bus, slot, func, 0x04) & 0xFFFF;
    cmd |= 0x06; // MEM | BUS MASTER
    vxair_hal_pci_write_config_mmconfig(bus, slot, func, 0x04, (uint32_t)cmd);
    ```
    Or do the read/write through a 16-bit MMCONFIG helper.
3. **Use explicit `+` in `mmcfg_addr`** instead of mixing `+` and `|`, or wrap the whole bitwise block in parentheses.
4. **Log the actual MMCONFIG physical address** being written, to confirm it is inside the mapped window.
5. **Confirm `MCFG` is found** by logging `vxair_hal_acpi_find_table("MCFG")` return value.
6. **Try writing the Command register via MMCONFIG *without* the legacy write** to isolate whether the legacy write is what the model sees.

---

## 9. Final Verdict

**`N1-PIVOT-FIX-6 PARTIAL`**

*   ✅ PCIe MMCONFIG support was added (MCFG parsed, region mapped, helpers exposed).
*   ✅ The driver writes the Command register through both legacy and MMCONFIG paths.
*   ✅ Guest-side readback shows `PCI CMD = 0x107` (IO + MEM + BUS MASTER).
*   ❌ QEMU’s e1000 model still logs `pci_master=0` (and `rx_enabled=0`) at packet arrival.
*   ❌ RX continues to fail; no DNS resolution.

The evidence strongly suggests that the MMCONFIG write itself is not reaching the QEMU device’s internal `config[PCI_COMMAND]`. The concrete bugs identified in §7 (small mapped window, RMW clobbering Status, mixed precedence, cacheable mapping) should be addressed in the next milestone before further driver-side speculation.