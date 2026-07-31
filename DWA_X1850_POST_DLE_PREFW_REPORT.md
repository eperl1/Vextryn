# DWA_X1850_POST_DLE_PREFW_REPORT

## 1. Exact Linux Source Paths and Line Ranges
This mapping uses the `rtl8852au` driver source as the proven reference:
- **`hfc_init` (Host Flow Control)**:
  - Call site: `phl/hal_g6/mac/mac_ax/init.c` (line 196, immediately after `dle_init`)
  - Implementation: `phl/hal_g6/mac/mac_ax/hci_fc.c` (lines 1641–1685)
- **`intf_preinit` / `usb_pre_init_8852a`**:
  - Call site: `phl/hal_g6/mac/mac_ax/init.c` (line 403, via function pointer `ops->intf_pre_init`)
  - Implementation: `phl/hal_g6/mac/mac_ax/mac_8852a/_usb_8852a.c` (lines 165–204)
- **`init_firmware` Entry Point**:
  - Implementation: `rtw_hal_download_fw` in `phl/hal_g6/hal_fw.c` (lines 149–190)

## 2. Exact In-Order Sequence Before Firmware Payload Transfer
After `dle_init` concludes successfully, the driver executes the following sequence *before* the first bytes of firmware payload are transmitted:
1. `hfc_init(adapter, 1, 0, 1)`: Initializes host flow control limits.
2. `dmac_func_en` / `cmac_func_en`: Enables MAC engine functionalities.
3. `usb_pre_init_8852a`: Performs USB-specific pre-initialization.
4. `rtw_hal_mac_disable_cpu`: Halts the MAC CPU and clears firmware state bits.
5. `rtw_hal_mac_enable_cpu(hal_info, 0, true /* dlfw */)`: Asserts the `B_AX_WCPU_FWDL_EN` bit, signaling to the hardware that a firmware download is imminent, and re-enables the CPU clock.

## 3. Exact Registers Written in That Path
The critical register writes (all via EP0 Vendor Requests) include:
- **In `usb_pre_init_8852a`**:
  - `R_AX_USB_HOST_REQUEST_2` (`0x14`): Set `B_AX_R_USBIO_MODE`
  - `R_AX_USB_WLAN0_1` (`0x12`): Clear reset bits to fix USB IO hang
  - `R_AX_HCI_FUNC_EN` (`0x10A0`): Toggle `B_AX_HCI_RXDMA_EN` and `B_AX_HCI_TXDMA_EN` (clear then set).
- **In `mac_enable_cpu` (with `dlfw = true`)**:
  - `R_AX_LDM` (`0x20`): Write `0` (Clear FW debug log)
  - `R_AX_SYS_CLK_CTRL` (`0x08`): Set `B_AX_CPU_CLK_EN`
  - `R_AX_WCPU_FW_CTRL` (`0x1D0`): Set `B_AX_WCPU_FWDL_EN`
  - `R_AX_PLATFORM_ENABLE` (`0x88`): Set `B_AX_WCPU_EN`

## 4. The First Step That Truly Begins Firmware Upload
The exact boundary where firmware upload begins is **line 181 of `phl/hal_g6/hal_fw.c`**:
```c
hal_status = rtw_hal_mac_fwdl(hal_info, fw_info->ram_buff, fw_info->ram_size);
```
Everything prior to this call is preparation.

## 5. EP0/Register-Init Work vs. Actual Firmware Transport
- **EP0/Register-Init**: `hfc_init`, `usb_pre_init_8852a`, `mac_disable_cpu`, and `mac_enable_cpu` are strictly register manipulation over the EP0 Control pipe. 
- **Actual Transport**: The execution of `rtw_hal_mac_fwdl` shifts from EP0 register writes to constructing Tx descriptors and chunking the firmware binary.

## 6. Are Bulk Endpoints Required Before Firmware Transfer Begins?
Bulk endpoints are **not required** for the `usb_pre_init_8852a` and `mac_enable_cpu` preparation steps. 
However, bulk endpoints (specifically a Bulk OUT pipe) are theoretically required to sink the payload chunks emitted by `rtw_hal_mac_fwdl`. Whether the physical adapter strictly enforces Bulk transport or if it could technically accept the payload over EP0 fallback is currently *UNKNOWN*, though Linux clearly intends to use the Bulk Tx path.

## 7. Execution Statement
**No firmware upload was executed.** This task was strictly limited to statically mapping the source sequence and identifying the boundary.

## 8. Knowledge Status
- **PROVEN**: The Linux control flow from `dle_init` through the `dlfw` CPU enablement logic relies entirely on EP0 register writes.
- **PROVEN**: The true firmware payload transfer boundary is the invocation of `rtw_hal_mac_fwdl`.
- **INFERRED**: Bulk OUT transport is required to satisfy `rtw_hal_mac_fwdl`'s chunking logic.
- **UNKNOWN**: The exact packet layout (Tx descriptor prefix) and endpoint index required by `rtw_hal_mac_fwdl` for the Bulk transfer.
