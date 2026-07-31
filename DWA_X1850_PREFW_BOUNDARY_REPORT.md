# DWA-X1850 B1 Pre-Firmware Boundary Analysis Report

## 1. Reference Overview

### 1.1 Source Context
The MAC initialization path is centrally defined in `phl/hal_g6/mac/mac_ax/init.c` within the function `mac_hal_init` (which maps to the overall `hal_init` sequence). This sequence establishes the base hardware state prior to the crucial `init_firmware` block.

**File:** `scratch/rtl8852au/phl/hal_g6/mac/mac_ax/init.c`
**Line Range:** 380 - 450 (`mac_hal_init`) and 125 - 203 (`dmac_pre_init`)

## 2. In-Order Execution State

### 2.1 Steps Already Reproduced (Proven)
1. **`pwr_switch`** (USB no-op, `init.c:380`)
2. **`hci_func_en`** (`init.c:389`) -> Wrote `0x03` to `R_AX_HCI_FUNC_EN` (`0x8380`), validated by read-back.
3. **`dmac_pre_init` entry** (`init.c:397`) -> Wrote `0x60440000` to `R_AX_DMAC_FUNC_EN` (`0x8400`), validated by read-back.
4. **`dmac_clk_en` step** (`init.c:184-185`) -> Wrote `0x00040000` to `R_AX_DMAC_CLK_EN` (`0x8404`), validated by read-back.

### 2.2 Immediate Next Steps (Not Yet Reproduced)
Inside `dmac_pre_init` (`init.c:190-200`):
5. **`dle_init`**: Data Link Engine initialization. Configures Packet Link Engine (PLE) and Wi-Fi Data Engine (WDE) memory quotas and sizes, then polls `R_AX_WDE_INI_STATUS` and `R_AX_PLE_INI_STATUS` to wait for hardware readiness.
6. **`hfc_init`**: Hardware Flow Control initialization.

Back in `mac_hal_init` (`init.c:403-450`):
7. **`intf_pre_init`** (`usb_pre_init_8852a`): Clears USB IO/TRX hang conditions (`R_AX_USB_WLAN0_1`, `R_AX_HCI_FUNC_EN`), reads endpoint capability maps.
8. **Firmware Upload Block (`init_firmware` / `romdl` / `enable_fw`)**: Deposits the `8852au3` bin payloads into the MAC's now-initialized packet buffers via bulk endpoints or EP0 segmented writes.

## 3. Boundary Identification

### 3.1 Last Clearly Safe Pre-Firmware Step
**The `dmac_clk_en` step (Step 4, already completed).**
Up to this point, all register writes were simple subsystem power-on / clock-un-gating flags. The hardware latches these bits immediately with no complex state-machine handshakes, and they do not require external payload tables.

### 3.2 First Potentially Firmware-Dependent or State-Sensitive Step
**The `dle_init` step (Step 5).**

### 3.3 Exact Reason for the Boundary
**Missing Polling Semantics & Missing Quota Tables.**
1. **Quota Complexity:** `dle_init` relies on massive hardcoded tables (`get_dle_mem_cfg`) to define memory boundaries for TX/RX buffers. Attempting to replicate this requires extracting and hardcoding over 50 specific quota registers (e.g. `R_AX_WDE_QTA`).
2. **State-Sensitive Polling:** After writing the configuration, `dle_init` explicitly loops and delays (`PLTFM_DELAY_US`), polling `R_AX_WDE_INI_STATUS` (`0x8410`) until the `WDE_MGN_INI_RDY` bit sets. If we submit this blindly without a polling mechanism or if we submit it via asynchronous xHCI transfers, we cannot safely guarantee the Data Link Engine is ready before we push firmware.
3. **Firmware Premise:** The entire purpose of `dle_init` and `hfc_init` is to prepare the MAC's packet buffers so that they can receive the firmware payload. Firmware upload in Realtek USB devices typically shifts from EP0 Control Transfers to EP-Bulk-Out transfers to achieve required speeds. We do not yet have a Bulk Out transfer ring implemented in the Vextryn xHCI driver.

## 4. Recommendation for the Single Safest Next Milestone

**Recommendation:** Stop EP0 register initialization here and **prepare the firmware staging infrastructure.**

**Justification:**
Continuing to blindly translate `dle_init` into EP0 control writes will clutter the USB driver with hardcoded Realtek quota tables and polling loops. Instead, we should pivot our effort to:
1. Extracting and embedding the exact `8852au3` firmware binary into the Vextryn OS image.
2. Building the Bulk OUT/IN transfer mechanisms in our xHCI driver (which the firmware upload will need).
3. Implementing a flexible register read/write polling macro in Vextryn so that `dle_init` can be implemented natively rather than as a manual sequence of hardcoded USB control arrays.

## 5. Evaluation Status
- **PROVEN:** The MAC power-on and un-gating phase is complete.
- **INFERRED:** The `dle_init` phase is the strict boundary where simple bit-flipping transitions into complex memory layout management and hardware polling.
- **UNKNOWN:** Whether the Realtek hardware strictly requires Bulk transfers for firmware, or if EP0 is permitted but just slower. Either way, `dle_init` must be modeled cleanly before firmware is attempted.
