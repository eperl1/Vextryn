# N1-PIVOT-FIX-2 Session Report — Packet Capture Ground Truth

**Status:** `N1-PIVOT-FIX-2 PARTIAL` — pcap ground truth obtained, ARP frames are correctly formed, but e1000 RX remains non-functional even in loopback mode. DNS resolution not demonstrated.

---

## Files Changed

- `drivers/net/e1000.c`
- `drivers/net/e1000.h`

Both files are currently **untracked** in this repository.

## Exact Source Excerpts

### `drivers/net/e1000.h` — relevant constants

```c
#define E1000_CTRL_FD     (1 << 0)   // Full Duplex
#define E1000_RCTRL_LBM_MASK   (3 << 6)  // Loopback Mode field mask
#define E1000_RCTRL_LBM_MAC    (1 << 6)  // MAC loopback (01b) per 82540EM spec
#define E1000_RCTRL_SECRC (1 << 26)  // Strip Ethernet CRC
#define E1000_RAH_AV  (1U << 31)  // Address Valid bit in RAH
```

### `drivers/net/e1000.c` — RX init ordering (final)

```c
// Write ring registers, RAL/RAH, then RDT, then RCTRL.EN last.
mmio_write32(mmio, E1000_RDBAL, (uint32_t)(g_e1000.rx_desc_paddr & 0xFFFFFFFF));
mmio_write32(mmio, E1000_RDBAH, (uint32_t)(g_e1000.rx_desc_paddr >> 32));
mmio_write32(mmio, E1000_RDLEN, E1000_RX_RING_SIZE * sizeof(e1000_rx_desc_t));
mmio_write32(mmio, E1000_RDH, 0);
mmio_write32(mmio, E1000_RDT, 0);
mmio_write32(mmio, E1000_RAL, ral);
mmio_write32(mmio, E1000_RAH, rah);
mmio_write32(mmio, E1000_RDT, E1000_RX_RING_SIZE - 1);

rctl |= E1000_RCTRL_EN | E1000_RCTRL_SBP | E1000_RCTRL_UPE |
        E1000_RCTRL_MPE | E1000_RCTRL_BAM | E1000_RCTRL_BSIZE_2048 |
        E1000_RCTRL_SECRC;
rctl &= ~E1000_RCTRL_LBM_MASK;
mmio_write32(mmio, E1000_RCTRL, rctl);
```

### `drivers/net/e1000.c` — loopback test

```c
// Enable MAC loopback mode (RCTL LBM = 01b)
uint32_t rctl_orig = mmio_read32(mmio, E1000_RCTRL);
uint32_t rctl = rctl_orig;
rctl &= ~E1000_RCTRL_LBM_MASK;
rctl |= E1000_RCTRL_LBM_MAC;
mmio_write32(mmio, E1000_RCTRL, rctl);

uint8_t test[64];
memcpy(test, g_e1000.mac_addr, 6);      // dest = own MAC
memcpy(test + 6, g_e1000.mac_addr, 6);  // src  = own MAC
vxair_e1000_send(test, 60);
```

## Exact Build, ISO, and QEMU Commands

```bash
# Build
cd ~/Vextryn_Air/build
make -j$(nproc) vextryn_air.elf

# Rebuild ISO
cd ~/Vextryn_Air
cp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/

# Run with packet capture (final run)
rm -f /tmp/vxair_net_final.pcap /tmp/vxair_final.log
timeout 60 qemu-system-x86_64 \
  -cdrom vextryn-air.iso \
  -m 512M -smp 4 -machine q35 -cpu qemu64 \
  -device e1000,netdev=net0 \
  -netdev user,id=net0 \
  -object filter-dump,id=dump0,netdev=net0,file=/tmp/vxair_net_final.pcap \
  -serial file:/tmp/vxair_final.log \
  -display none -no-reboot
```

## PCAP Analysis

`/tmp/vxair_net_final.pcap` (412 bytes, 5 packets):

| # | Len | Hex (Ethernet + payload) | Type |
|---|-----|----------------------------|------|
| 0 | 60  | `52540012345652540012345688b50e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d0000000000000000000000000000` | LBM test frame (ethertype 0x88b5) |
| 1 | 60  | `ffffffffffff525400123456080600010800060400015254001234560a00020f0000000000000a000203000000000000000000000000000000000000` | ARP Request |
| 2 | 64  | `52540012345652550a0002030806000108000604000252550a0002030a0002035254001234560a00020f00000000000000000000000000000000000000000000` | ARP Reply |
| 3 | 60  | same as packet 1 | ARP Request (retry) |
| 4 | 64  | same as packet 2 | ARP Reply (retry) |

### ARP Request (Packet 1) — RFC 826 field-by-field

| Field | Offset | Bytes | Value | Verdict |
|-------|--------|-------|-------|---------|
| Ethernet dst MAC | 0  | `ff ff ff ff ff ff` | broadcast | OK |
| Ethernet src MAC | 6  | `52 54 00 12 34 56` | guest MAC | OK |
| EtherType        | 12 | `08 06` | ARP | OK |
| Hardware type    | 14 | `00 01` | Ethernet (1) | OK |
| Protocol type    | 16 | `08 00` | IPv4 | OK |
| Hardware len     | 18 | `06` | 6 | OK |
| Protocol len     | 19 | `04` | 4 | OK |
| Opcode           | 20 | `00 01` | Request (1) | OK |
| Sender MAC       | 22 | `52 54 00 12 34 56` | guest MAC | OK |
| Sender IP        | 28 | `0a 00 02 0f` | 10.0.2.15 | OK |
| Target MAC       | 32 | `00 00 00 00 00 00` | unknown | OK |
| Target IP        | 38 | `0a 00 02 03` | 10.0.2.3 | OK |

### ARP Reply (Packet 2) — RFC 826 field-by-field

| Field | Offset | Bytes | Value | Verdict |
|-------|--------|-------|-------|---------|
| Ethernet dst MAC | 0  | `52 54 00 12 34 56` | guest MAC | OK |
| Ethernet src MAC | 6  | `52 55 0a 00 02 03` | QEMU/SLIRP MAC | OK |
| EtherType        | 12 | `08 06` | ARP | OK |
| Hardware type    | 14 | `00 01` | Ethernet (1) | OK |
| Protocol type    | 16 | `08 00` | IPv4 | OK |
| Hardware len     | 18 | `06` | 6 | OK |
| Protocol len     | 19 | `04` | 4 | OK |
| Opcode           | 20 | `00 02` | Reply (2) | OK |
| Sender MAC       | 22 | `52 55 0a 00 02 03` | 10.0.2.3 MAC | OK |
| Sender IP        | 28 | `0a 00 02 03` | 10.0.2.3 | OK |
| Target MAC       | 32 | `52 54 00 12 34 56` | guest MAC | OK |
| Target IP        | 38 | `0a 00 02 0f` | 10.0.2.15 | OK |

**Conclusion from pcap:** The ARP request is correctly formed per RFC 826, and SLIRP is generating a valid ARP reply. The bug is not in `net/core/arp.c`.

## Loopback Mode (LBM) Test

- The user requested `RCTL bits 6-7 = 11`. That value is not a valid loopback encoding on the 82540EM; the first LBM test with 11 caused the test frame to escape to the wire (pcap packet 0).
- We therefore switched to the correct MAC-loopback value `01b` (`E1000_RCTRL_LBM_MAC`).
- The loopback test sends a 60-byte frame to the card's own MAC, then polls `vxair_e1000_receive()`.
- Result: **LBM TEST FAILED** — no looped-back frame was received; `RX miss` diagnostics showed all descriptor statuses zero.
- RCTL readback at the moment of the test: `0x400805e` (EN=1, SBP=1, UPE=1, MPE=1, BAM=1, SECRC=1, LBM=01).
- The exact same failure occurred with multiple register-order variations (RDT before/after RCTRL, RAL/RAH before RCTRL, etc.).

## `-nic user,model=e1000` Diagnostic

Attempted with:

```bash
qemu-system-x86_64 ... -nic user,model=e1000 -object filter-dump,id=dump0,netdev=nic0,file=/tmp/vxair_net3.pcap ...
```

Result: QEMU failed with `Parameter 'netdev' expects a network backend id`. The `-nic` shorthand does not expose a backend id in a way that can be referenced by `-object filter-dump` in this QEMU version. Using `-netdev user,id=net0 -device e1000,netdev=net0` (already tested) is equivalent and already captures the same backend path.

## Required Evidence Checklist

- ✅ PCAP captured and parsed.
- ✅ ARP request hex + field-by-field RFC 826 breakdown.
- ✅ ARP reply present in capture.
- ❌ Loopback test did not succeed.
- ❌ DNS A-record not resolved (blocked by RX failure).

## Final Verdict

`N1-PIVOT-FIX-2 PARTIAL`

The packet-capture ground truth proves that the ARP layer is correctly generating requests and that QEMU's SLIRP backend is producing replies. The remaining failure is in the e1000 driver's receive path, which does not receive frames even under MAC loopback. Further driver-level debugging (DMA memory attributes, cache coherency, descriptor format/alignment, or comparison with a known-working minimal e1000 driver) is required before DNS resolution can succeed.
