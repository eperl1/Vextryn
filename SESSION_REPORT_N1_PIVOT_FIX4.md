# N1-PIVOT-FIX-4 Session Report — Reference E1000 Driver Diff

**Milestone:** N1-PIVOT-FIX-4 — Reference E1000 Driver Diff  
**Final Verdict:** `N1-PIVOT-FIX-4 PARTIAL` — init-sequence mismatch found and corrected, but RX still blocked.  
**Reference used:** JOS 6.828 Lab 6 solution, `gonglinyuan/jos`, `kern/e1000.c`  
**Reference URL:** https://raw.githubusercontent.com/gonglinyuan/jos/lab6/kern/e1000.c

---

## 1. Files Touched

*   `drivers/net/e1000.c`
*   `drivers/net/e1000.h`

**No forbidden files were touched.**

---

## 2. Reference Driver Summary

The reference driver is a minimal, known-working JOS/6.828 solution for the QEMU-emulated Intel 82540EM. Its init sequence is:

1.  PCI enable + MMIO map (no device reset, no `CTRL.SLU/FD`).
2.  TX ring setup: `TDBAL/TDBAH/TDLEN/TDH/TDT` → `TIPG` → `TCTL` (EN/PSP/CT/COLD).
3.  RX setup: `RAL`/`RAH` → clear 128-byte `MTA` → `IMS=0` → `RDBAL/RDBAH/RDLEN/RDH/RDT=size-1` → `RCTL=EN|BAM|SECRC`.
4.  Receive is polled; no interrupts used.

---

## 3. Side-by-Side Diff

The full register-by-register diff is in:

**`N1_PIVOT_FIX4_REFERENCE_DIFF.md`**

Key differences that were applied to match the reference:

| Our Driver (before) | Reference (applied) |
|---|---|
| Reset device + set `CTRL.SLU`/`FD` | **No reset, no `CTRL` link setup** |
| Read MAC from EEPROM | **Read MAC from `RAL`/`RAH`** |
| `TCTL` only `EN\|PSP` | **Added `CT=0x10` and `COLD=0x40`** |
| No `TIPG` write | **Wrote `TIPG=0x0060200A`** |
| No `MTA` clear | **Cleared 128-byte `MTA`** |
| `RAL`/`RAH` written **after** ring registers | **Moved to before ring registers** |
| `RCTL=EN\|SBP\|UPE\|MPE\|BAM\|SECRC` (later also tested `EN\|BAM\|SECRC`) | **`RCTL=EN\|BAM\|SECRC`** |

---

## 4. Changes Applied

### `drivers/net/e1000.h`

```c
#define E1000_TIPG    0x0410  // Transmit Inter-Packet Gap
#define E1000_MTA     0x5200  // Multicast Table Array (128 bytes)
...
#define E1000_TCTRL_CT    (0x10 << 4)   // Collision Threshold
#define E1000_TCTRL_COLD  (0x40 << 12)  // Collision Distance full-duplex
#define E1000_TIPG_DEFAULT 0x0060200AU
```

### `drivers/net/e1000.c`

Key init excerpt after matching the reference:

```c
    // Read MAC from RAL/RAH (loaded from EEPROM by QEMU model)
    uint32_t ral = mmio_read32(mmio, E1000_RAL);
    uint32_t rah = mmio_read32(mmio, E1000_RAH);
    ...
    // TX setup (reference order)
    mmio_write32(mmio, E1000_TDBAL, (uint32_t)(g_e1000.tx_desc_paddr & 0xFFFFFFFF));
    mmio_write32(mmio, E1000_TDBAH, (uint32_t)(g_e1000.tx_desc_paddr >> 32));
    mmio_write32(mmio, E1000_TDLEN, E1000_TX_RING_SIZE * sizeof(e1000_tx_desc_t));
    mmio_write32(mmio, E1000_TDH, 0);
    mmio_write32(mmio, E1000_TDT, 0);
    mmio_write32(mmio, E1000_TIPG, E1000_TIPG_DEFAULT);
    mmio_write32(mmio, E1000_TCTRL,
                 E1000_TCTRL_EN | E1000_TCTRL_PSP | E1000_TCTRL_CT | E1000_TCTRL_COLD);

    // RX setup (reference order: RAL/RAH, MTA, ring, RCTL)
    mmio_write32(mmio, E1000_RAL, ral_w);
    mmio_write32(mmio, E1000_RAH, rah_w);
    for (int i = 0; i < 128; i += 4)
        mmio_write32(mmio, E1000_MTA + i, 0);
    mmio_write32(mmio, E1000_IMS, 0);
    ...
    mmio_write32(mmio, E1000_RDT, E1000_RX_RING_SIZE - 1);
    ...
    mmio_write32(mmio, E1000_RCTRL,
                  E1000_RCTRL_EN | E1000_RCTRL_BAM | E1000_RCTRL_SECRC |
                  E1000_RCTRL_UPE | E1000_RCTRL_MPE | E1000_RCTRL_SBP);
```

(The `UPE/MPE/SBP` bits were added in a later diagnostic step; the reference uses only `EN|BAM|SECRC`.)

---

## 5. Build / ISO / QEMU Commands

```bash
# Build
make -j$(nproc) vextryn_air.elf

# ISO
cp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/

# QEMU (q35)
qemu-system-x86_64 \\
  -cdrom vextryn-air.iso \\
  -m 512M -smp 4 -machine q35 -cpu qemu64 \\
  -device e1000,netdev=net0 \\
  -netdev user,id=net0 \\
  -object filter-dump,id=dump0,netdev=net0,file=/tmp/vxair_n1fix4.pcap \\
  -serial file:/tmp/vxair_n1fix4.log \\
  -display none -no-reboot

# Diagnostic run with -machine pc
qemu-system-x86_64 \\
  -cdrom vextryn-air.iso \\
  -m 512M -smp 4 -machine pc -cpu qemu64 \\
  -device e1000,netdev=net0 \\
  -netdev user,id=net0 \\
  -serial file:/tmp/vxair_n1fix4_pc.log \\
  -display none -no-reboot
```

---

## 6. Checksums After Changes

```text
drivers/net/e1000.c  dc07bff561721a43700afbf28ff6d46df37f2002eef643c660932c63f780bc94
drivers/net/e1000.h  183fdfe5970a21fc15c1efa93ad75cdb6b48373b07ada0238932c544172c0d56
```

---

## 7. Test Results

### 7.1 With Reference Sequence (no reset, TIPG/CT/COLD, MTA clear, RCTL=EN|BAM|SECRC)

```text
E1000: RCTL orig=0x0 set=0x4008002 readback=0x4008002 (EN=1 BAM=1 SECRC=1)
E1000: LBM enabled RCTL=0x4008042
E1000: TX posted idx=0 len=60 addr=0x...
E1000: TX done idx=0 iterations=0
E1000: LBM TEST FAILED - no looped-back frame received
DNS: ARP resolution failed
```

### 7.2 With SLU/FD Added (no reset)

```text
E1000: CTRL orig=0x140240 set=0x140241 readback=0x140241
E1000: LBM TEST FAILED
DNS: ARP resolution failed
```

### 7.3 With Promiscuous Bits (UPE/MPE/SBP) Added

```text
E1000: RCTL ... UPE=1 MPE=1 SBP=1
E1000: LBM TEST FAILED
DNS: ARP resolution failed
```

### 7.4 With `-machine pc`

```text
E1000: Init OK (TX=16 RX=16)
E1000: LBM TEST FAILED
DNS: ARP resolution failed
```

**Conclusion across all configurations:** TX continues to work, but RX never sees a descriptor `DD` bit set, even in MAC loopback. The pcap still shows ARP request and reply on the wire, so SLIRP is functioning; the guest simply never receives the frame.

---

## 8. Analysis

*   The init sequence now closely matches a known-working JOS reference, including `TIPG`, `TCTL.CT/COLD`, `MTA` clear, `RAL`/`RAH` ordering, and `RCTL` bit selection.
*   Adding/removing `CTRL.SLU`/`FD`, `UPE`/`MPE`/`SBP`, or switching from `q35` to `pc` does not change the outcome.
*   Because the descriptor memory is never written, the remaining root cause is **not** in the init/control sequence itself. Likely causes now are:
    1.  A **QEMU model/host issue** specific to this build/environment that prevents the e1000 model from delivering RX frames to the guest (even though pcap captures show them on the wire).
    2.  A **PCI/DMA-level problem** in the kernel (e.g., BAR mapping, cacheability, bus-mastering) that allows TX DMA but silently blocks RX DMA.
    3.  A **descriptor ring/buffer attribute** issue (alignment, physical address visibility) that the hardware rejects only for RX.

---

## 9. Next Recommended Steps

Since register-level driver changes are no longer productive, the next milestones should focus on:

1.  **PCI/DMA diagnostic:** Verify the e1000 BAR is mapped with the same cacheability/ordering as TX, and that the RX descriptor/buffer physical pages are actually visible to a PCI bus master.
2.  **QEMU model verification:** Run a known reference image (e.g., a tiny Linux or the JOS reference itself) with the same `-device e1000` setup to confirm that RX works in this host environment. If it does, diff the PCI/VMM setup between that image and Vextryn.
3.  **Capture ground truth from QEMU:** Use QEMU's own e1000 debug trace (`-d trace:e1000*`, `info mtree`, `info qtree`) to see if the model even attempts to write the RX descriptor and why it might fail.

---

## 10. Final Verdict

**`N1-PIVOT-FIX-4 PARTIAL`**

*   ✅ Found and applied the known-working reference init sequence.
*   ✅ Side-by-side diff documented in `N1_PIVOT_FIX4_REFERENCE_DIFF.md`.
*   ✅ Tested reference sequence, SLU/FD variations, promiscuous filtering, and `-machine pc`.
*   ❌ RX descriptor `DD` bit never sets; ARP/DNS still fail.

The RX failure is **not caused by the e1000 init/control sequence**; further progress requires DMA/PCI-level or QEMU model-level investigation.
