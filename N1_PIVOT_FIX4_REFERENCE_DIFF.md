# N1-PIVOT-FIX-4 Side-by-Side Init-Sequence Diff

**Reference driver:** JOS 6.828 Lab 6 solution, `gonglinyuan/jos`, file `kern/e1000.c`  
**URL:** https://raw.githubusercontent.com/gonglinyuan/jos/lab6/kern/e1000.c  
**Our driver:** `drivers/net/e1000.c` / `drivers/net/e1000.h` (as of N1-PIVOT-FIX-3)

---

## High-Level Sequence

| Step | Reference (`gonglinyuan/jos`) | Our Driver (`vxair_e1000_init`) |
|---|---|---|
| 1 | PCI enable + MMIO map | PCI enable + MMIO map + identity-map MMIO |
| 2 | **No device reset** | **Reset device (`CTRL.RST`)** |
| 3 | **No link/SLU setup** | **Set `CTRL.SLU` + `CTRL.FD`, wait for link up** |
| 4 | Read MAC from `RAL`/`RAH` (or hardcode) | **Read MAC from EEPROM** |
| 5 | `enable_transmit()` then `enable_receive()` | Allocate rings + buffers, then interleave TX/RX register setup |
| 6 | TX: `TDBAL/TDBAH/TDLEN/TDH/TDT` | TX: `TDBAL/TDBAH/TDLEN/TDH/TDT` |
| 7 | TX: `TIPG = 0x0060200A` | **No `TIPG` write** |
| 8 | TX: `TCTL = EN \| PSP, CT=0x10, COLD=0x40` | TX: `TCTL = EN \| PSP` only |
| 9 | RX: `RAL`/`RAH` + clear MTA (128 bytes) | RX: `RAL`/`RAH` written **after** ring registers, **no MTA clear** |
| 10 | RX: `IMS = 0` | RX: `IMS = 0` **after** TX/RX enable |
| 11 | RX: `RDBAL/RDBAH/RDLEN/RDH/RDT=size-1` | RX: `RDBAL...RDT` written before `RAL`/`RAH` |
| 12 | RX: `RCTL = EN \| BAM \| SECRC` | RX: `RCTL = EN \| SBP \| UPE \| MPE \| BAM \| BSIZE_2048 \| SECRC` |
| 13 | `RDT` remains `size-1`; receive polls `RDH`/`RDT` | `RDT` initially 0, then set to `size-1`; receive polls `g_e1000.rx_tail` |

---

## Detailed Register-by-Register Comparison

### CTRL — Device Control

| Register / Bits | Reference | Our Driver |
|---|---|---|
| Reset (`CTRL.RST` bit 26) | **Not used** | Set and polled until cleared |
| `CTRL.SLU` (bit 6) | **Not set** | Set after reset, link polled |
| `CTRL.FD` (bit 0) | **Not set** | Set after reset |
| `CTRL.ASDE` (bit 5) | N/A (no reset) | Not set |

**Implication:** The reference relies on QEMU’s default post-boot state (link already up) and avoids any reset/CTRL link manipulation. Our reset + `SLU`/`FD` sequence could leave the device in a state the QEMU model does not expect.

---

### TIPG — Transmit Inter-Packet Gap (offset `0x410`)

| Reference | Our Driver |
|---|---|
| `TIPG = 0x0060200A` (IPGT=10, IPGR1=8, IPGR2=6) | Not written |

**Implication:** `TIPG` is required for correct TX timing on 82540EM; missing it may leave the transmitter in an undefined state, although in practice TX currently works.

---

### TCTL — Transmit Control (offset `0x400`)

| Field | Reference | Our Driver |
|---|---|---|
| `TCTL.EN` | Set | Set |
| `TCTL.PSP` | Set | Set |
| `TCTL.CT` | `0x10` (Collision Threshold) | Default (0) |
| `TCTL.COLD` | `0x40` (Collision Distance, full-duplex) | Default (0) |

**Implication:** The datasheet recommends programming `CT` and `COLD` before enabling TX. The reference uses the standard full-duplex values.

---

### RAL / RAH — Receive Address

| Reference | Our Driver |
|---|---|
| Written **before** `RDBAL`/`RDLEN` | Written **after** `RDBAL`/`RDLEN` |
| Hardcoded to the QEMU-default MAC (via `RAL=0x12005452`, `RAH=0x00005634\|AV`) | Computed from EEPROM and written with `AV` bit |
| MTA (Multicast Table Array, 128 bytes at `0x5200`) explicitly cleared to 0 | MTA **not touched** |

**Implication:** The Intel spec says RAL/RAH should be valid before enabling RX, but the exact order relative to the ring registers is not critical. The MTA clear is a notable difference; stale MTA entries could theoretically filter incoming multicast/broadcast traffic, although `UPE`/`MPE`/`BAM` should override.

---

### RCTL — Receive Control (offset `0x100`)

| Bit | Reference | Our Driver |
|---|---|---|
| `EN` (1) | Set | Set |
| `SBP` (2) | Not set | Set |
| `UPE` (3) | Not set | Set |
| `MPE` (4) | Not set | Set |
| `LBM` (6:7) | 00 | 00 |
| `BAM` (15) | Set | Set |
| `BSIZE` (16:17) | 00 (2048) | 00 (2048) |
| `SECRC` (26) | Set | Set |

**Implication:** Our extra `SBP`/`UPE`/`MPE` bits are conservative and should not break reception, but they are not present in the minimal working reference.

---

### IMS — Interrupt Mask Set (offset `0xD0`)

| Reference | Our Driver |
|---|---|
| Set to `0` early, before enabling RCTL/TCTL | Set to `0` late, after TCTL/RCTL enabled |

**Implication:** Minor ordering difference; polling drivers do not rely on interrupts, so this is unlikely to affect RX.

---

## Most Likely Root Cause(s)

1. **Device reset / `CTRL.SLU`/`FD` interaction with QEMU model** — The reference that works in QEMU does not reset the device or touch `SLU`/`FD`. Our reset + link-up sequence may put the device into a state where RX is disabled or the model’s internal state machine is not advanced correctly.
2. **Missing `TIPG` and `TCTL.CT/COLD`** — While these are TX registers, the datasheet says they must be configured before enabling TX, and omitting them may leave the controller in an undefined state that also affects RX.
3. **MTA not cleared** — Low probability, but a stale MTA could filter traffic.

---

## Proposed Minimal Fix

1. Remove the device reset and `CTRL.SLU`/`FD` link-up sequence (match QEMU default state like the reference).
2. Read the MAC from `RAL`/`RAH` instead of EEPROM.
3. Clear the 128-byte MTA before enabling RX.
4. Add `TIPG` programming with the standard `0x0060200A` value.
5. Set `TCTL.CT=0x10` and `TCTL.COLD=0x40` before enabling TX.
6. Reorder to: TX setup → TIPG/TCTL → RX setup (RAL/RAH/MTA, then ring, then RCTL).
7. Simplify `RCTL` to `EN | BAM | SECRC` (remove `SBP | UPE | MPE`, matching the reference).

After applying these changes, retest:
- MAC loopback (`LBM=01b`)
- External ARP request + reply
- DNS A-record resolution
