# N1-PIVOT-FIX-3 Session Report — RX Descriptor Struct/Memory Verification

**Milestone:** N1-PIVOT-FIX-3 — RX Descriptor Struct/Memory Verification  
**Final Verdict:** `N1-PIVOT-FIX-3 PARTIAL` — raw dump and register evidence delivered; RX path still fails; DNS not yet achieved.

---

## 1. Scope and Allowed Files

*   **Allowed files touched:**
    *   `drivers/net/e1000.c`
    *   `drivers/net/e1000.h`
*   **Forbidden files touched:** none
*   **Goal:** Verify the RX descriptor struct layout, add a raw descriptor memory dump, and add a read memory barrier before reading descriptor status. If a memory/struct bug is found, fix it and demonstrate DNS resolution; otherwise report the raw evidence.

---

## 2. Build / ISO / QEMU Commands Used

```bash
# Build kernel
make -j$(nproc) vextryn_air.elf
# Result: build/bin/vextryn_air.elf

# Rebuild ISO
cp build/bin/vextryn_air.elf iso_root/vextryn/kernel.elf
grub-mkrescue -o vextryn-air.iso iso_root/

# QEMU run with e1000 + user-mode networking + packet capture
qemu-system-x86_64 \\
  -cdrom vextryn-air.iso \\
  -m 512M -smp 4 -machine q35 -cpu qemu64 \\
  -device e1000,netdev=net0 \\
  -netdev user,id=net0 \\
  -object filter-dump,id=dump0,netdev=net0,file=/tmp/vxair_n1p3.pcap \\
  -serial file:/tmp/vxair_n1p3.log \\
  -display none -no-reboot
```

---

## 3. Changes Made

### `drivers/net/e1000.h`

Added the RXDCTL register offset and a default threshold value (used as an experiment after the raw dump suggested descriptor memory was never touched):

```c
#define E1000_RDT     0x2818  // RX Descriptor Tail
#define E1000_RXDCTL  0x2828  // RX Descriptor Control
...
// ===== RXDCTL Register Bits =====
// Default threshold configuration used by Linux/standard drivers.
// PTHRESH=1, HTHRESH=1, WTHRESH=1, GRAN=1 (descriptor granularity).
#define E1000_RXDCTL_DEFAULT 0x01010101U
```

### `drivers/net/e1000.c`

1. Added descriptor struct layout/size/offset logging.
2. Fixed the RX ring `memset` to use the correct size.
3. Added a raw 16-byte dump of `rx_desc[0]` after the loopback send.
4. Added a `volatile` pointer + `lfence` read barrier before reading descriptor status.
5. Added an experimental `RXDCTL` write before enabling `RCTL.EN`.

Key excerpt (struct verification + raw dump + read barrier):

```c
// Verify RX descriptor struct layout against Intel 82540EM datasheet
vxair_log_info("E1000: sizeof(e1000_rx_desc_t)=%u expected=16", sizeof(e1000_rx_desc_t));
vxair_log_info("E1000: RX desc offsets: addr=%u len=%u csum=%u status=%u errors=%u special=%u",
               offsetof(e1000_rx_desc_t, addr),
               offsetof(e1000_rx_desc_t, length),
               offsetof(e1000_rx_desc_t, checksum),
               offsetof(e1000_rx_desc_t, status),
               offsetof(e1000_rx_desc_t, errors),
               offsetof(e1000_rx_desc_t, special));
...
// Raw hex dump of descriptor[0] BEFORE polling status, bypassing the struct entirely.
volatile uint8_t *raw = (volatile uint8_t *)&g_e1000.rx_desc[0];
vxair_log_info("E1000: raw RX desc[0] after send = "
               "%02x %02x %02x %02x %02x %02x %02x %02x  "
               "%02x %02x %02x %02x %02x %02x %02x %02x",
               raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], raw[6], raw[7],
               raw[8], raw[9], raw[10], raw[11], raw[12], raw[13], raw[14], raw[15]);
```

Receive path read barrier:

```c
volatile e1000_rx_desc_t *desc = (volatile e1000_rx_desc_t *)&g_e1000.rx_desc[idx];
__asm__ volatile("lfence" ::: "memory");
uint8_t status = desc->status;
```

---

## 4. Checksums

```text
drivers/net/e1000.c  f7a4801db7f80e958a81ccafe0f42c967d47ff8fe9f6b8740a74a8ebb338cad4
drivers/net/e1000.h  b37be64727c46e9646e610e8a2f64e748a82bd7c60aae9cd1cac6b5663159666
```

---

## 5. Evidence

### 5.1 RX Descriptor Struct Layout

Log output:

```text
E1000: sizeof(e1000_rx_desc_t)=16 expected=16
E1000: RX desc offsets: addr=0 len=8 csum=10 status=12 errors=13 special=14
```

**Conclusion:** The `e1000_rx_desc_t` struct is exactly 16 bytes and every field offset matches the Intel 82540EM datasheet layout (`addr` 8 bytes, `length` 2, `checksum` 2, `status` 1, `errors` 1, `special` 2). No struct/padding bug.

### 5.2 RX Ring Register State

```text
E1000: RX ring: RDBAL=0x9fb000 RDBAH=0x0 RDLEN=256 RDH=0 RDT=15
```

*   Ring base is page-aligned.
*   `RDLEN = 256 = 16 descriptors × 16 bytes`.
*   `RDH = 0`, `RDT = 15`, giving the hardware the full ring.

### 5.3 Raw 16-Byte Dump of RX Descriptor[0]

Taken **after** the loopback test frame was transmitted and before any status poll:

```text
E1000: raw RX desc[0] after send = 0x0 0x40 0xa0 0x0 0x0 0x0 0x0 0x0  0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0
```

Decoding the first 8 bytes as little-endian `addr`:

```text
addr  = 0x0000000000A04000
guess = g_e1000.rx_bufs_paddr + 0 * E1000_BUFFER_SIZE
```

This is the expected physical buffer address — the descriptor was initialized correctly. However, the **status byte at offset 12 is `0x00`**; the hardware has **not** set the DD (Descriptor Done) bit, and `RDH` never advanced from 0.

### 5.4 RCTL Readback

```text
E1000: RCTL orig=0x0 set=0x400801e readback=0x400801e (EN=1 UPE=1 MPE=1 BAM=1 LBM=0 BSIZE=0 SECRC=1)
```

All requested receive-enable, promiscuous, broadcast-accept, and strip-CRC bits are set.

### 5.5 RXDCTL Experiment

After the raw dump showed the descriptor untouched, an `RXDCTL` write was attempted (this is outside the strict N1-PIVOT-FIX-3 scope but was the only remaining register-level hypothesis):

```text
E1000: RXDCTL set=0x0x1010101 readback=0x0x0
```

The register **does not hold the write**. This strongly implies `RXDCTL` at offset `0x2828` is **not implemented/reserved on this QEMU 82540EM model**, so the RXDCTL-threshold hypothesis is invalid here.

### 5.6 LBM (Loopback) Test

```text
E1000: LBM enabled RCTL=0x400805e
E1000: TX posted idx=0 len=60 addr=0x9fc000
E1000: TX done idx=0 iterations=0
E1000: raw RX desc[0] after send = 0x0 0x40 0xa0 0x0 0x0 0x0 0x0 0x0  0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0
E1000: LBM TEST FAILED - no looped-back frame received
```

*   TX completes successfully.
*   The loopback frame never appears in the RX ring.
*   `RDH` stays at 0.

### 5.7 External Traffic / PCAP Ground Truth

The packet capture shows **both the ARP request and the SLIRP ARP reply** on the virtual wire, proving the networking layer above the driver is functioning and SLIRP is responding:

```text
Packet 0 (len 60): 52540012345652540012345688b50e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d0000000000000000000000000000
Packet 1 (len 60): ffffffffffff525400123456080600010800060400015254001234560a00020f0000000000000a000203000000000000000000000000000000000000
Packet 2 (len 64): 52540012345652550a0002030806000108000604000252550a0002030a0002035254001234560a00020f00000000000000000000000000000000000000000000
Packet 3 (len 60): ffffffffffff525400123456080600010800060400015254001234560a00020f0000000000000a000203000000000000000000000000000000000000
Packet 4 (len 64): 52540012345652550a0002030806000108000604000252550a0002030a0002035254001234560a00020f00000000000000000000000000000000000000000000
```

#### ARP Request (Packet 1)

| Field | Value |
|-------|-------|
| Ethernet dest | `ff:ff:ff:ff:ff:ff` (broadcast) |
| Ethernet src | `52:54:00:12:34:56` (guest) |
| Ethertype | `0x0806` (ARP) |
| Hardware type | `0x0001` (Ethernet) |
| Protocol type | `0x0800` (IPv4) |
| HLEN / PLEN | `6` / `4` |
| Opcode | `0x0001` (request) |
| Sender MAC | `52:54:00:12:34:56` |
| Sender IP | `10.0.2.15` |
| Target MAC | `00:00:00:00:00:00` |
| Target IP | `10.0.2.3` (DNS resolver) |

#### ARP Reply (Packet 2)

| Field | Value |
|-------|-------|
| Ethernet dest | `52:54:00:12:34:56` (guest) |
| Ethernet src | `52:55:0a:00:02:03` (SLIRP gateway) |
| Opcode | `0x0002` (reply) |
| Sender MAC/IP | `52:55:0a:00:02:03` / `10.0.2.3` |
| Target MAC/IP | `52:54:00:12:34:56` / `10.0.2.15` |

The ARP frame is well-formed and the reply is correctly addressed to the guest. The guest’s e1000 driver simply never sees it.

### 5.8 DNS / ARP Result

```text
DNS: Polling for ARP reply...
DNS: ARP resolution failed
NET: DNS test completed (no response — this may be expected if QEMU user-mode networking is offline)
```

DNS cannot proceed because ARP resolution fails; ARP fails because the RX path never reports a received frame.

---

## 6. Analysis

*   **Struct/memory is correct.** The descriptor is the right size, packed, aligned, and the raw dump confirms the hardware has the correct buffer address.
*   **The read barrier (`lfence` + `volatile`) does not change behavior.** The status is truly `0x00`, not a stale CPU cache read.
*   **`RXDCTL` is not the answer.** The register at `0x2828` is read-only-zero in this QEMU model, so writing it had no effect.
*   **Loopback and external RX both fail.** Since even internal MAC loopback does not produce a descriptor update, the bug is not in ARP, SLIRP, or packet filtering.
*   **Most likely remaining root causes:**
    1.  The QEMU `e1000` model is not actually delivering RX frames to the guest for this particular driver configuration, possibly because of a missing or mis-set `CTRL`/device mode bit that is not obvious from the 82540EM spec.
    2.  A kernel-level issue with PCI bus-mastering/DMA coherency or page-table attributes for the descriptor/buffer pages, despite the fact that TX works.
    3.  The e1000 device in this QEMU version has a quirk not covered by the 82540EM datasheet (e.g., the modern `e1000-82540em` model may need a different reset/CTRL sequence than real hardware).

---

## 7. Next Recommended Step

Because the driver-level register configuration now matches the datasheet and the struct/memory path is fully verified, **further register-level guessing is no longer productive.** The next milestone should be one of the following:

1.  **Compare against a known-working minimal e1000 driver** (e.g., the OSDev wiki reference or a tiny OS’s driver) and diff the init sequence register-by-register.
2.  **Escalate to a QEMU/device model investigation:** capture `info qtree`, `info pci`, and try a different QEMU version or the `-nic user,model=e1000` shorthand to see if the issue is QEMU invocation-specific.
3.  **Verify DMA coherency / page-table attributes:** ensure the pages returned by `vxair_pmm_alloc_page/pages()` are actually writable by a PCI bus master and are not marked cache-disable/uncacheable in a way that confuses the device.

---

## 8. Final Verdict

**`N1-PIVOT-FIX-3 PARTIAL`**

*   ✅ RX descriptor struct layout verified against datasheet.
*   ✅ Raw 16-byte descriptor dump captured.
*   ✅ Read memory barrier (`lfence`) added and tested.
*   ✅ RXDCTL experiment performed; register is read-only-zero on this model.
*   ❌ RX descriptor DD bit never set, even in MAC loopback.
*   ❌ DNS/ARP not yet working.

The evidence is complete for the memory/struct verification; the remaining RX failure is not caused by descriptor layout, padding, or cache coherency at the CPU side.
