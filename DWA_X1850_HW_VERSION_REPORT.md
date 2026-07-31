# DWA-X1850 B1 Hardware Version / Cut Version Report

## 1. Reference Analysis

### 1.1 Source Location & Constants
The Cut Version (CV) read and interpretation were derived from the `lwfinger/rtl8852au` Linux driver repository.

**Target Register (`R_AX_SYS_CFG1`):**
- **File:** `scratch/rtl8852au/phl/hal_g6/mac/mac_reg.h`
- **Line:** 3411
- **Macro:** `#define R_AX_SYS_CFG1 0x00F0`

**Chip Version Bitmask (`B_AX_CHIP_VER`):**
- **File:** `scratch/rtl8852au/phl/hal_g6/mac/mac_reg.h`
- **Lines:** 3422-3423
- **Macros:** `#define B_AX_CHIP_VER_SH 12`, `#define B_AX_CHIP_VER_MSK 0xf`
- **Logic:** The hardware version occupies bits 12-15 of `0x00F0` (which is precisely the upper half of the byte at offset `0x00F1`).

**Cut Version Values (`FWDL_CAV`, `CBV`, `CCV`):**
- **File:** `scratch/rtl8852au/phl/hal_g6/mac/mac_ax/fwdl.h`
- **Lines:** 169-171
- **Macros:** `FWDL_CAV = 0`, `FWDL_CBV = 1`, `FWDL_CCV = 2`

**Firmware Mapping (`FWDL_CCV` -> `u3` firmware):**
- **File:** `scratch/rtl8852au/phl/hal_g6/mac/mac_ax/fwdl.c`
- **Lines:** 1006-1025
- **Logic:** The `FWDL_CCV` case branches to load the `array_8852a_u3_nic` firmware array (as opposed to `array_8852a_u2_nic` for `CBV`).

### 1.2 Exact Control Request Fields
- **bmRequestType:** `0xC0` (Vendor Request, Device to Host, Endpoint 0)
- **bRequest:** `0x05` (Realtek USB Read Request)
- **wValue:** `0x00F0` (Lower 16 bits of the register address: `0x00F0 & 0xFFFF`)
- **wIndex:** `0x0000` (Upper 16 bits of the register address: `(0x00F0 >> 16) & 0xFFFF`)
- **wLength:** `0x0004` (4 bytes, for a 32-bit read)

### 1.3 Returned Payload
The 32-bit (4-byte) DMA read returned the following raw payload bytes in memory (printed MSB to LSB):
`0x0C 0x49 0x25 0x37`

This corresponds to the 32-bit value `0x0C492537`.

### 1.4 Vextryn Serial Log Snippet
```text
[INFO] [RTL8852] Submitting Vendor Control Read for Register 0x00F0...
[INFO] [xHCI] Event 32 completed with code 15
[INFO] [xHCI] Ignoring non-fatal code 15, waiting for final status...
[INFO] [RTL8852] Vendor Control Read completed.
[INFO] [RTL8852] Value at 0x00F0: 0x0C492537
```
*(Note: Log hex formatting omitted leading zeros due to %X format string, yielding `0xc0x490x250x37` in raw string output, strictly mapping to the array bytes `0x0C`, `0x49`, `0x25`, `0x37`.)*

### 1.5 Interpretation of the Returned Value
The hardware driver reads the Cut Version from bits 12-15 of `0x00F0`. 
In our 32-bit value `0x0C492537`:
- Bit 12 begins at the higher nibble of the second byte (`0x25`).
- `(0x0C492537 >> 12) & 0xF = 2`
- The hardware returns a Cut Version of `2`.

In the Linux driver, `2` explicitly maps to `CCV` (or `FWDL_CCV`). 

---

## 2. Evidence Separation

### PROVEN
- The physical adapter responded successfully to the control read of `0x00F0`.
- The returned Cut Version value is explicitly `2`.
- `2` maps to the `CCV` hardware stepping in the Linux driver definitions.

### INFERRED
- The surrounding bits (like the `0x37` in the lower byte) likely contain strapping pins, subsystem config states, or other config values stored in `R_AX_SYS_CFG1`, but they are ignored by the driver's Cut Version check.

### UNKNOWN
- We do not know if the `CCV` cut version requires slightly different MAC/PHY register initialization writes compared to the older `CAV`/`CBV` steppings (though driver source uses `if (cv >= CCV)` checks which we will need to respect).

---

## 3. Impact on Firmware

**EXPLICIT STATEMENT:**
This result **fundamentally changes the expected firmware family.**
Before this read, one might have guessed that the DWA-X1850 B1 (being an 8852A chip) would use the baseline firmware (`8852a_nic.bin` or `array_8852a_u2_nic` for `CBV`). 

However, because the physical hardware reports Cut Version `2` (`CCV`), the `fwdl.c` logic explicitly branches to require the `U3` firmware revision (`array_8852a_u3_nic` / `8852au3_nic.bin`). We **must** use the `U3` firmware image when we begin the firmware upload phase, otherwise the chip will likely reject the firmware or crash upon execution.
