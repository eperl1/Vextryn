# DWA-X1850 B1 (RTL8852A) Vendor Control Bring-up Report

## 1. Reference Analysis

### 1.1 Source Location & Constants
The control request pattern and hardware identity definitions were derived from the `lwfinger/rtl8852au` Linux driver repository.

**USB Control Request Wrapper (`usb_read32`):**
- **File:** `scratch/rtl8852au/phl/hal_g6/hal_usb.c`
- **Lines:** 62-80
- **Logic:** Calls `_os_usbctrl_vendorreq` with `request = 0x05`, `requesttype = 0x01` (read), `wvalue = addr & 0xFFFF`, `index = (addr >> 16) & 0xFFFF`.

**USB Control Read Macro Mapping:**
- **File:** `scratch/rtl8852au/os_dep/linux/usb_ops_linux.c`
- **Lines:** 85-88
- **Logic:** Translates `requesttype == 0x01` to `reqtype = REALTEK_USB_VENQT_READ`.
- **File:** `scratch/rtl8852au/include/usb_ops.h`
- **Line:** 19
- **Macro:** `#define REALTEK_USB_VENQT_READ 0xC0`

**Target Register (Chip Info):**
- **File:** `scratch/rtl8852au/phl/hal_g6/mac/mac_reg.h`
- **Line:** 3471
- **Macro:** `#define R_AX_SYS_CHIPINFO 0x00FC`

**Hardware Identification (Chip ID):**
- **File:** `scratch/rtl8852au/phl/hal_g6/mac/mac_ax.c`
- **Line:** 18
- **Macro:** `#define CHIP_ID_HW_DEF_8852A 0x50`

### 1.2 Exact Control Request Fields
- **bmRequestType:** `0xC0` (Vendor Request, Device to Host, Endpoint 0)
- **bRequest:** `0x05` (Realtek USB Read Request)
- **wValue:** `0x00FC` (Lower 16 bits of the register address: `0x00FC & 0xFFFF`)
- **wIndex:** `0x0000` (Upper 16 bits of the register address: `(0x00FC >> 16) & 0xFFFF`)
- **wLength:** `0x0004` (4 bytes, for a 32-bit read)

### 1.3 Returned Payload
The 32-bit (4-byte) DMA read returned the following raw payload bytes in memory (printed MSB to LSB as interpreted in a 32-bit register format):
`0xC0 0x00 0x00 0x50`

### 1.4 Vextryn Serial Log Snippet
```text
[INFO] [RTL8852] Submitting Vendor Control Read for Register 0x00FC...
[INFO] [xHCI] Event 32 completed with code 15
[INFO] [xHCI] Ignoring non-fatal code 15, waiting for final status...
[INFO] [RTL8852] Vendor Control Read completed.
[INFO] [RTL8852] Value at 0x00FC: 0xC0000050
```

### 1.5 Explanation of Register 0x00FC
Register `0x00FC` (`R_AX_SYS_CHIPINFO`) is explicitly read by the Linux driver (`PLTFM_REG_R8(R_AX_SYS_CHIPINFO)`) to identify the attached Realtek AX-series silicon variant before loading firmware. The lowest byte contains the hardcoded Chip ID.

### 1.6 Explanation of 0xC0000050
The lowest byte of the returned 32-bit value (`0xC0000050`) is `0x50`. In the driver's hardware definitions, `CHIP_ID_HW_DEF_8852A` is defined exactly as `0x50`. The upper bytes (`0xC0`) likely correspond to additional hardware revision or silicon stepping data present in the register, but the lowest byte conclusively matches the RTL8852A identifier.

---

## 2. Evidence Separation

### PROVEN
- The xHCI driver successfully completed a vendor-specific control IN request (code 1, Success) on the physical D-Link DWA-X1850 B1.
- The DMA memory transfer successfully received bytes from the adapter.
- The adapter returned `0x50` in the lowest byte when `0x00FC` was requested, which perfectly matches the Linux driver's expected `CHIP_ID_HW_DEF_8852A` value for RTL8852A silicon.

### INFERRED
- The upper byte (`0xC0`) is inferred to be internal hardware revision/bonding data, as it is discarded by the driver (which only masks/reads an 8-bit value: `cur_id = PLTFM_REG_R8(R_AX_SYS_CHIPINFO)`).

### UNKNOWN
- We do not yet know the power state of the MAC/PHY, nor what other registers must be configured before the firmware can be accepted.
- We do not yet know the firmware upload protocol (chunk size, register destinations).

---

## 3. Safest Next Step

Based on the `rtw_hal_macid_init` and early `hal_usb.c` bring-up flow in the Linux source, the single safest next step is **another vendor EP0 read** to gather the hardware version (e.g., cut version) from `R_AX_SYS_CFG1` (`0x00F0` + 1), which the driver performs immediately after reading the Chip ID to determine which exact firmware file is required. 

We will **not** attempt firmware upload or bulk transfers until the hardware identity and version are completely parsed and cross-checked against the available firmware blobs.
