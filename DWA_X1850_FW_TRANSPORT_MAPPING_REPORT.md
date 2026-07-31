# Firmware Transport Mapping Report

## 1. Files Examined
The firmware download transport path was traced through the following driver files:
- `phl/hal_g6/mac/mac_ax/fwdl.c`
- `phl/hal_g6/mac/mac_ax/trx_desc.c`
- `phl/hal_g6/mac/mac_def.h`
- `phl/hal_g6/mac/txdesc.h`
- `phl/hal_g6/hal_api_mac.c`
- `phl/phl_api_drv.c`
- `phl/hci/phl_trx_usb.c`
- `phl/hal_g6/rtl8852a/usb/hal_trx_8852au.c`
- `phl/hal_g6/mac/mac_ax/mac_8852a/_usb_8852a.c`
- `os_dep/linux/usb_ops_linux.c`
- `os_dep/linux/usb_intf.c`

## 2. Firmware Chunking Rules
The `rtw_hal_mac_fwdl` function passes the firmware buffer to `mac_fwdl`, which progresses through three download phases. In `fwdl_phase2`, `__sections_download` (in `fwdl.c`) iterates through the firmware payload sections. 

The payload is strictly chunked by **`FWDL_SECTION_PER_PKT_LEN`**, which is set to **2020 bytes**.

## 3. Payload Wrapper (Tx Descriptor Layout)
For each chunk, `__sections_build_txd` constructs an H2C control block and sets `info->type = RTW_PHL_PKT_TYPE_FWDL`. This is passed to `mac_build_txdesc` (in `trx_desc.c`), which routes to `txdes_proc_h2c_fwdl`.

The wrapper prepended to each chunk is exactly **24 bytes** long, defined by `struct wd_body_t` (`WD_BODY_LEN`).

The contents of this 24-byte prefix are strictly deterministic:
- **`dword0`**: `(MAC_AX_DMA_H2C << AX_TXD_CH_DMA_SH) | AX_TXD_FWDL_EN`
  - `MAC_AX_DMA_H2C` is 12 (0xc). `AX_TXD_CH_DMA_SH` is 16.
  - `AX_TXD_FWDL_EN` is `BIT(20)`.
  - Thus, `dword0 = (0xc << 16) | (1 << 20)` = **`0x001C0000`** in little-endian format.
- **`dword1`**: `0x00000000`
- **`dword2`**: `(info->pktlen & AX_TXD_TXPKTSIZE_MSK)`
  - Contains the size of the firmware chunk (up to 2020) in the lowest 14 bits (`AX_TXD_TXPKTSIZE_MSK` = `0x3FFF`).
- **`dword3`**: `0x00000000`
- **`dword4`**: `0x00000000`
- **`dword5`**: `0x00000000`

The maximum transfer size sent to the hardware is 24 + 2020 = **2044 bytes**.

## 4. Endpoint/Pipe Mapping from EP0 to Transfer Path
The firmware transport does **not** use EP0. It hands off to a specific Bulk OUT endpoint via the following resolution chain:

1. `__sections_download` calls `PLTFM_TX(h2cb->data, h2cb->len)`.
2. `PLTFM_TX` maps through the MAC operation interface to `hal_pltfm_tx` -> `rtw_phl_pltfm_tx` -> `phl_pltfm_tx_usb`.
3. In `phl_pltfm_tx_usb` (in `phl_trx_usb.c`), the DMA channel is queried via `rtw_hal_get_fwcmd_queue_idx()`.
4. `rtw_hal_get_fwcmd_queue_idx` routes to `hal_get_fwcmd_queue_idx_8852au`, which returns `FWCMD_QUEUE_IDX_8852A` (**12**). This corresponds to `MAC_AX_DMA_H2C`.
5. The specific Bulk ID is resolved via `rtw_hal_get_bulkout_id(hal, dma_ch=12, mode=0)`, which maps to `get_bulkout_id_8852a()`.
6. In `get_bulkout_id_8852a` (in `_usb_8852a.c`), if `ch_dma == MAC_AX_DMA_H2C`, it sets `bulkout_id = BULKOUTID2` (**2**).
7. The packet is then submitted to the OS dependency layer `os_usb_tx(..., bulk_id=2, ...)`.
8. In Linux (`usb_ops_linux.c`), `bulkid2pipe` looks up the physical endpoint number in `usb_data->RtOutPipe[2]`.
9. Because `RtOutPipe` is populated linearly during USB interface probing (`usb_intf.c`), `BULKOUTID2` translates precisely to the **3rd Bulk OUT endpoint** enumerated by the adapter.

**Conclusion**: The firmware chunks are prepended with a deterministic 24-byte TX descriptor and transmitted natively as Bulk transfers via the **3rd Bulk OUT endpoint** of the physical adapter.
