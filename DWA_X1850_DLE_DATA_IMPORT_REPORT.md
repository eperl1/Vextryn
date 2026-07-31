# DWA_X1850_DLE_DATA_IMPORT_REPORT

## 1. Exact Files Added/Changed
- **Added**: `drivers/usb/rtl8852_dle.h`
  - Created native Vextryn structures and imported static constant tables for the USB2 DLE profile.

## 2. Linux Source Files and Lines
All structures and tables were mapped verbatim from:
- **File**: `phl/hal_g6/mac/mac_ax/dle.c`
  - `wde_size25`: Lines 193–197
  - `ple_size27`: Lines 389–393
  - `wde_qt25`: Lines 596–601
  - `ple_qt61` (min): Lines 1580–1593
  - `ple_qt62` (max): Lines 1596–1609

## 3. Exact Vextryn Structs Introduced
I introduced the following exact structs to mirror the Linux driver:
- `vxair_dle_size_t` (containing `pge_size`, `lnk_pge_num`, `unlnk_pge_num`)
- `vxair_wde_quota_t` (containing `hif`, `wcpu`, `pkt_in`, `cpu_io`)
- `vxair_ple_quota_t` (containing `cma0_tx`, `cma1_tx`, `c2h`, `h2c`, `wcpu`, `mpdu_proc`, `cma0_dma`, `cma1_dma`, `bb_rpt`, `wd_rel`, `cpu_io`, `tx_rpt`)

## 4. Exact Static Table Instances Imported
I defined the USB2 High-Speed profile (`dle_mem_usb2_8852b`) explicitly in `rtl8852_dle.h`:
- `dle_wde_size25` (64-byte pages, 242 linked, 14 unlinked)
- `dle_ple_size27` (128-byte pages, 1402 linked, 6 unlinked)
- `dle_wde_qt25`
- `dle_ple_qt61` (Min quota for PLE)
- `dle_ple_qt62` (Max quota for PLE)

## 5. Register Targets Count
When this profile is eventually executed, it will map to exactly **17 register targets**:
- 2 configuration registers (`R_AX_WDE_PKTBUF_CFG`, `R_AX_PLE_PKTBUF_CFG`) using the sizes.
- 4 WDE quota registers (`QTA0`, `QTA1`, `QTA3`, `QTA4`) combining min/max values.
- 11 PLE quota registers (`QTA0` through `QTA10`) combining min/max values.

## 6. Verbatim vs. Computed Fields
- **Copied Verbatim**: The data structures and static values (page counts, quotas, page sizes).
- **Computed at Runtime**: The "Start Bound" field written into `R_AX_WDE_PKTBUF_CFG` and `R_AX_PLE_PKTBUF_CFG` is computed dynamically via the formula: `(lnk_pge_num + unlnk_pge_num) * pge_size / 256`. The min and max quotas from the tables are also combined dynamically (shifting max quota up 16 bits and ORing it with min quota) before writing to each `QTA` register.

## 7. Explicit Hardware Writes Statement
**No DLE hardware writes, nor quota table programming of any kind, were executed.**

## 8. Knowledge Status
- **PROVEN**: The exact memory layout, field ordering, and static quotas for the USB2 High-Speed DLE profile have been successfully bridged natively into Vextryn.
- **UNKNOWN**: None. The system is strictly poised to safely execute the first physical programming pass over these tables.
