# DWA-X1850 B1 DLE Initialization Plan

## 1. Source References

### 1.1 Implementation Locations
- **`dle_init`**: `phl/hal_g6/mac/mac_ax/dle.c` (Lines 2555-2655)
  - This function orchestrates the Data Link Engine (DLE) configuration, setting up the Packet Link Engine (PLE) and Wi-Fi Data Engine (WDE) packet buffers.
- **`dle_func_en`**: `dle.c` (Lines 2226-2239)
- **`dle_clk_en`**: `dle.c` (Lines 2241-2251)
- **`dle_mix_cfg`**: `dle.c` (Lines 2337-2370)
- **`dle_quota_cfg`**: `dle.c` (Lines 2374-2485)

### 1.2 Quota Table Sources
The required memory layouts for the 8852A USB interface are statically defined arrays in `dle.c`:
- **WDE Size Config**: `wde_size24` or `wde_size25` depending on USB speed (`dle.c:186-197`).
- **PLE Size Config**: `ple_size1` (`dle.c:207`).
- **WDE Quota Configs**: e.g., `wde_qt1` (`dle.c:562`).
- **PLE Quota Configs**: e.g., `ple_qt1` (`dle.c:980`).

## 2. Register Interactions

### 2.1 Control Registers (Written)
- **`R_AX_DMAC_FUNC_EN` (0x8400)**: `dle_func_en` modifies `B_AX_DLE_WDE_EN` (`BIT(26)`) and `B_AX_DLE_PLE_EN` (`BIT(23)`).
- **`R_AX_DMAC_CLK_EN` (0x8404)**: `dle_clk_en` modifies `B_AX_DLE_WDE_CLK_EN` (`BIT(26)`) and `B_AX_DLE_PLE_CLK_EN` (`BIT(23)`).
- **`R_AX_WDE_PKTBUF_CFG` (0x8C08)**: Written by `dle_mix_cfg`.
- **`R_AX_PLE_PKTBUF_CFG` (0x9008)**: Written by `dle_mix_cfg`.
- **Over 50+ Quota Registers** (e.g., `0x8C10` to `0x8C60`, `0x9010` to `0x9060`): Written heavily in loops within `dle_quota_cfg`.

### 2.2 Status Registers & Polling (Read-Back)
- **`R_AX_WDE_INI_STATUS` (0x8D00)**
  - **Mask**: `WDE_MGN_INI_RDY` (`BIT(1) | BIT(0)`)
- **`R_AX_PLE_INI_STATUS` (0x9100)**
  - **Mask**: `PLE_MGN_INI_RDY` (`BIT(1) | BIT(0)`)

**Exact Delay / Retry Logic (`dle.c:2628-2652`):**
The Linux driver executes a while-loop for exactly 2000 iterations (`DLE_WAIT_CNT = 2000`). Within each loop, it reads the status register, masks it, checks for readiness, and if not ready, issues a 1 microsecond delay (`DLE_WAIT_US = 1`) before retrying.

## 3. Execution Mechanics

### 3.1 Pure Table Programming vs Handshake/Polling
- **Pure Table Programming**: The entirety of `dle_mix_cfg` and `dle_quota_cfg` is one-way table programming. It involves blindly blasting dozens of 32-bit quota boundaries over the bus.
- **Handshake/Polling**: The final stage of `dle_init` blocks execution and reads `0x8D00` and `0x9100` repeatedly, handshaking with the hardware's internal packet buffer state machine to ensure the table programming actually latched and the buffers initialized without faulting.

### 3.2 Read-Back Validation Scope
The table programming (the 50+ quota registers) could technically be validated purely by read-back (writing a quota value and reading it to ensure it stuck). However, the ultimate success of `dle_init` cannot be validated by reading the quotas back; it *must* be validated by observing the `INI_RDY` bits transition to `1` in the status registers.

## 4. Evaluation & Next Steps

### Can we do one minimal DLE experiment safely now?
**No, not safely or cleanly.** 
While we *could* manually hardcode the `0x8404` DLE clock enable and `0x8400` DLE function enable bits using EP0 array structs in `usb.c`, stopping there is meaningless because the packet engine state machine will not transition to `INI_RDY` without the quota tables programmed. Attempting to program the 50+ quota registers by manually defining 50+ `vxair_usb_setup_t` structs and hardcoded arrays in the USB stack is architecturally toxic and highly error-prone.

### Must we first build a polling abstraction / cleaner register-access layer?
**Yes.**
The Linux driver relies on abstract macros like `MAC_REG_W32()`, `MAC_REG_R32()`, and `PLTFM_DELAY_US()`. To safely implement `dle_init`, Vextryn Air must first establish a generic Realtek MAC access layer (e.g., `vxair_mac_write32(adapter, addr, val)` and `vxair_mac_poll(adapter, addr, mask, target, timeout)`).

### PROVEN / INFERRED / UNKNOWN
- **PROVEN**: The exact register bounds, wait loops, and masks for `dle_init` are fully mapped.
- **INFERRED**: Writing the function enables without the quota tables will either fault the packet engine or leave it permanently un-ready.
- **UNKNOWN**: How the Linux driver selects between `wde_size24` vs `wde_size25` strictly (it appears based on whether USB negotiates at USB2.0 vs USB3.0 speeds, which we need to detect dynamically in our implementation).
