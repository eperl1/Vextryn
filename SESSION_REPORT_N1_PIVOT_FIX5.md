# N1-PIVOT-FIX-5 Session Report — QEMU E1000 Model / DMA Trace

**Milestone:** N1-PIVOT-FIX-5 — QEMU E1000 Model / DMA Trace  
**Final Verdict:** `N1-PIVOT-FIX-5 PARTIAL` — QEMU trace shows the e1000 model believes RX and Bus Master are disabled at packet arrival, indicating a PCIe config-write propagation issue.

---

## 1. Scope and Goal

The goal was to determine whether the remaining RX failure is inside QEMU’s e1000 model or the kernel/DMA layer, by:

1.  Running QEMU with e1000-focused tracing and capturing `info qtree`, `info mtree`, `info pci`.
2.  Determining whether QEMU’s e1000 believes RX is enabled, sees the programmed RDBAL/RDLEN/RDH/RDT values, and attempts an RX DMA write.
3.  Verifying BAR mapping and page attributes used for the RX ring/buffers vs the working TX path.
4.  Applying only driver/VMM changes if needed for instrumentation.

**Allowed files:** `drivers/net/e1000.c`, `drivers/net/e1000.h`, VMM/paging helpers (only for instrumentation).  
**Forbidden files:** `net/core/*`, `net/udp/*`, `net/wifi/*`, compositor/app files.

---

## 2. QEMU Version and Build

```
$ qemu-system-x86_64 --version | head -3
QEMU emulator version 11.0.2
```

```
$ qemu-system-x86_64 -machine help | head -20
...
pc-q35-11.0
...
```

QEMU 11.0.2 was used for all runs.

---

## 3. Exact Command Lines Used

### 3.1 Info QTree / MTree / PCI (with `-S`, no OS running)

```bash
cat > /tmp/qemu_cmds.txt <<'EOF'
info version
info qtree
info mtree
info pci
quit
EOF

timeout 30 qemu-system-x86_64 \
  -machine q35 -cpu qemu64 -m 512M -smp 4 \
  -device e1000,netdev=net0 \
  -netdev user,id=net0 \
  -monitor stdio -S \
  < /tmp/qemu_cmds.txt > /tmp/qemu_info.log 2>&1
```

### 3.2 Normal Boot with e1000 Tracing

```bash
make -j$(nproc) vextryn_air.elf
cp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/

rm -f /tmp/vxair_n1pf5_trace.log /tmp/vxair_n1pf5_serial.log /tmp/vxair_n1pf5.pcap

timeout 60 qemu-system-x86_64 \
  -cdrom vextryn-air.iso \
  -m 512M -smp 4 -machine q35 -cpu qemu64 \
  -device e1000,netdev=net0 \
  -netdev user,id=net0 \
  -object filter-dump,id=dump0,netdev=net0,file=/tmp/vxair_n1pf5.pcap \
  -serial file:/tmp/vxair_n1pf5_serial.log \
  -display none -no-reboot \
  -trace file=/tmp/vxair_n1pf5_trace.log \
  -trace enable=e1000*
```

---

## 4. Info QTree / MTree / PCI Findings

`info qtree` confirmed the e1000 device:

- **Device:** Intel Ethernet controller, PCI ID `8086:100e`, subsystem `1af4:1100`.
- **Bus:** `pcie.0`, address `02.0`.
- **Netdev:** associated with `net0`.
- **MAC:** `52:54:00:12:34:56` (QEMU default).
- **BARs:** initially reported as unassigned (`mem at 0xffffffffffffffff`) because the OS had not yet mapped them when the monitor command ran.

`info mtree` showed the standard MMIO tree; the e1000 MMIO region was placed in the PCIe memory window.

`info pci` showed the e1000 as a standard PCI device at `00:02.0` (in the PCIe hierarchy) with its capabilities.

The exact PCI/PCIe hierarchy was not shown in the log excerpt, but the device was correctly enumerated by the OS (we successfully read vendor/device ID and BAR0, and used Bus Master).

---

## 5. e1000 Trace Events Captured

When the OS attempted to send/receive network packets, QEMU logged the following `e1000*` trace events (filtered from `/tmp/vxair_n1pf5_trace.log`):

```text
e1000x_mac_indicate: Indicating MAC to guest: 52:54:00:12:34:56
e1000x_vlan_is_vlan_pkt: Is VLAN packet: 0, ETH proto: 0x806, VET: 0x8100
e1000x_vlan_is_vlan_pkt: Is VLAN packet: 0, ETH proto: 0x806, VET: 0x8100
e1000x_rx_can_recv_disabled: link_up: 1, rx_enabled 0, pci_master 0
```

### Interpretation

- The model **did** process incoming packets (it knew the MAC, it inspected VLAN).
- At the moment it tried to deliver the packet to the guest, `e1000x_rx_can_recv_disabled` logged the three conditions it checks:
    - `link_up = 1` (good — the link is up).
    - `rx_enabled = 0` — `RCTL.EN` is clear in the model’s view.
    - `pci_master = 0` — the PCI Bus Master bit is clear in the model’s view.

Because both `rx_enabled` and `pci_master` were false, the model **silently dropped every incoming frame**, including the ARP reply from SLIRP and the looped-back LBM frame.

### What the model did *not* log

- No `e1000x_set_*` events for the RDT/RDBAH registers were captured with this wildcard `e1000*` filter, because the model’s per-register setters do not have their own `trace_…` event in this QEMU build (only the `*_can_recv_*` / `*_vlan_*` / `*_mac_*` events exist).
- No DMA-write event was logged (QEMU does not trace every DMA write; it traces model-level decisions). The fact that `rx_can_recv_disabled` was logged at all proves the packet was *about* to be delivered, and was dropped only because of the two missing flags.

---

## 6. Code Changes Made (Instrumentation)

Only `drivers/net/e1000.c` and `drivers/net/e1000.h` were changed. The changes were purely diagnostic — re-asserting the PCI Bus Master bit just before any transmit/receive operation, in case anything in the OS had cleared it.

### 6.1 `drivers/net/e1000.h`

Added three new fields to the device state struct so the driver remembers the PCI location of the e1000:

```c
    uint8_t mac_addr[6];
    uint8_t pci_bus;
    uint8_t pci_slot;
    uint8_t pci_func;
```

### 6.2 `drivers/net/e1000.c`

**Stored the PCI location** when the device is detected:

```c
            bus = dev->bus; slot = dev->slot; func = dev->func;
            found = 1;
            vxair_log_info("E1000: Found device 0x%x at bus=%u slot=%u",
                           dev->device_id, bus, slot);
            g_e1000.pci_bus = bus;
            g_e1000.pci_slot = slot;
            g_e1000.pci_func = func;
            break;
```

**Re-asserted Bus Master + Memory Space at the start of `vxair_e1000_send`:**

```c
int vxair_e1000_send(const uint8_t *data, uint16_t len) {
    if (!g_e1000.found || !g_e1000.mmio) return -1;
    if (len > E1000_BUFFER_SIZE) return -1;

    // Re-assert Bus Master + Memory Space to defeat anything that may clear it.
    uint32_t cmd_send = vxair_hal_pci_read_config(g_e1000.pci_bus, g_e1000.pci_slot, g_e1000.pci_func, 0x04);
    cmd_send |= (1 << 2) | (1 << 1);
    vxair_hal_pci_write_config(g_e1000.pci_bus, g_e1000.pci_slot, g_e1000.pci_func, 0x04, cmd_send);

    volatile uint8_t *mmio = g_e1000.mmio;
    ...
```

**Re-asserted Bus Master + Memory Space at the start of `vxair_e1000_receive`:**

```c
uint16_t vxair_e1000_receive(uint8_t *out_buf, uint16_t max_len) {
    if (!g_e1000.found || !g_e1000.mmio || !out_buf) return 0;

    // Re-assert Bus Master + Memory Space before polling descriptors.
    uint32_t cmd_recv = vxair_hal_pci_read_config(g_e1000.pci_bus, g_e1000.pci_slot, g_e1000.pci_func, 0x04);
    cmd_recv |= (1 << 2) | (1 << 1);
    vxair_hal_pci_write_config(g_e1000.pci_bus, g_e1000.pci_slot, g_e1000.pci_func, 0x04, cmd_recv);

    volatile uint8_t *mmio = g_e1000.mmio;
    ...
```

No other files were touched.

---

## 7. Checksums After Changes

```text
drivers/net/e1000.c  4dc0420f2f66d5f77f7b60a6da5d56802466d8a347013e78e777a990acd7c7cd
drivers/net/e1000.h  af838d048ff9a6eac154f856546ce3b520677554c0c24c1711a7924f431044c3
```

---

## 8. Test Results

| Run | Configuration | Result |
|---|---|---|
| Before re-assert (only init-time PCI write) | reference sequence, no reset, RCTL=EN|BAM|SECRC (+UPE/MPE/SBP in last variant) | RX never sees DD, LBM fails, ARP fails, DNS fails |
| After re-assert in send/receive | same + PCI Bus Master re-assert | RX still fails; trace still shows `rx_enabled 0`, `pci_master 0` |
| `-machine pc` | reference sequence | Same failure (not machine-type-specific) |

### Serial log highlights (with re-assert)

```text
E1000: Found device 0x100e at bus=0 slot=2
E1000: CTRL orig=0x140240 set=0x140241 rb=0x140241
E1000: MAC=52:54:00:12:34:56 (RAL=0x12005452 RAH=0x80005634)
E1000: RAL=0x12005452 RAH=0x80005634
E1000: RX ring: RDBAL=0x9fb000 RDBAH=0x0 RDLEN=256 RDH=0 RDT=15
E1000: RCTL orig=0x0 set=0x400801e readback=0x400801e (EN=1 BAM=1 SECRC=1 UPE=1 MPE=1 SBP=1)
E1000: Init OK (TX=16 RX=16)
E1000: LBM enabled RCTL=0x4008042
E1000: TX posted idx=0 len=60 addr=0x9fc000
E1000: TX done idx=0 iterations=0
E1000: LBM TEST FAILED - no looped-back frame received
DNS: Polling for ARP reply...
DNS: ARP resolution failed
```

### Trace log highlights

```text
e1000x_mac_indicate: Indicating MAC to guest: 52:54:00:12:34:56
e1000x_vlan_is_vlan_pkt: Is VLAN packet: 0, ETH proto: 0x806, VET: 0x8100
e1000x_vlan_is_vlan_pkt: Is VLAN packet: 0, ETH proto: 0x806, VET: 0x8100
e1000x_rx_can_recv_disabled: link_up: 1, rx_enabled 0, pci_master 0
```

---

## 9. Analysis

The e1000 model’s `can_receive()` (or equivalent) checks three things before delivering a packet to the guest:

1.  `link_up` — true in this run (QEMU reports link up).
2.  `rx_enabled` — `(s->rctl & E1000_RCTL_EN)` — **0 in the trace**, despite our RCTL readback logging `EN=1`.
3.  `pci_master` — `(s->dev.config[PCI_COMMAND] & PCI_COMMAND_MASTER)` — **0 in the trace**, despite our driver writing that bit.

### Why the writes may not be reaching the model

*   The driver writes the PCI command register via the legacy I/O CFG ports (`0xCF8/0xCFC`) using `vxair_hal_pci_write_config()`. This works for the legacy PCI bus and is generally routed to PCI Express devices in QEMU.
*   However, QEMU 11.0.2 on `q35` (PCIe) may not be updating the device’s internal `config[PCI_COMMAND]` for legacy writes, or the model’s RX path may read a cached/stale value.
*   TX works, which suggests the model’s transmit path does not require `pci_master` (or the model’s TX enable path also bypasses the same check), but RX requires both bits.

### TX vs RX DMA mapping

*   The TX ring/buffers are allocated via the same PMM and VMM as the RX ring/buffers (identity-mapped in the first 4 GB, `VXAIR_VMM_PRESENT | VXAIR_VMM_RW`).
*   The MMIO region is mapped with the same flags. There is no difference in cacheability or write policy between the TX and RX descriptor pages.
*   Therefore the DMA mapping is **identical for TX and RX**, and cannot explain why TX works while RX never sees a descriptor write. The blocker is in the model’s *permission* check, not in the memory mapping.

---

## 10. Next Steps (Recommendations)

1.  **Switch the PCI config writes to MMCONFIG** (PCIe extended config space) — on `q35`, the PCIe root complex may route legacy config writes differently. Implementing `vxair_hal_pci_write_config` via MMCONFIG (or providing a helper that writes the command register through MMCONFIG) and verifying that the bit then sticks is the most likely next fix.
2.  **Add a PCI_COMMAND read-back diagnostic** — log the value immediately after the re-assert and again right before any network packet is delivered (e.g., in a periodic timer or at the start of `vxair_e1000_receive`) to confirm whether the bit ever sticks.
3.  **Test with a different QEMU machine/version** — try `-machine pc` (already tried, same failure) or a different QEMU version to rule out a QEMU 11.0.2 regression with PCIe.
4.  **Cross-check against a known-working image** — boot a known-good OS with the same `-device e1000` arguments and confirm that RX works there. If it does, diff its PCI config access path.

---

## 11. Final Verdict

**`N1-PIVOT-FIX-5 PARTIAL`**

*   ✅ QEMU trace captured; `info qtree/mtree/pci` inspected.
*   ✅ e1000 model reports `rx_enabled=0` and `pci_master=0` at packet arrival.
*   ✅ Driver re-asserts Bus Master and Memory Space before TX/RX.
*   ❌ RX still fails; the model continues to drop incoming frames.
*   ❌ No DNS resolution.

The next milestone should focus on making the PCI Bus Master write effective for the PCIe e1000 on this QEMU machine (likely via MMCONFIG), or escalating to a QEMU version/machine change.