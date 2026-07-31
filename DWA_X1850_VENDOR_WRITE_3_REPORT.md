# DWA-X1850 B1 Vendor Write 3 Report (DMAC_CLK_EN)

## 1. Reference Analysis

### 1.1 Source Location & Rationale
The very next step in the Linux driver's MAC initialization sequence—immediately after the `DMAC_FUNC_EN` (0x8400) logic finishes—is to supply a dispatcher clock to the DMAC. 

**Exact Linux Source Paths & Line Numbers:**
1. **`scratch/rtl8852au/phl/hal_g6/mac/mac_ax/init.c`** (Lines 184-185)
   - Function: `dmac_pre_init`
   - Logic:
     ```c
     val32 = (B_AX_DISPATCHER_CLK_EN);
     MAC_REG_W32(R_AX_DMAC_CLK_EN, val32);
     ```
   - This performs a direct 32-bit Write to `R_AX_DMAC_CLK_EN` to turn on the Dispatcher Clock.
   - It is the immediate sequential step after the `R_AX_DMAC_FUNC_EN` setup block in the `dmac_pre_init` function. 

### 1.2 Target Register & Constants
- **Target Register Address (`R_AX_DMAC_CLK_EN`):** 
  - Defined in `phl/hal_g6/mac/mac_reg.h:375`
  - `#define R_AX_DMAC_CLK_EN 0x8404`
- **Enable Bits:**
  - `B_AX_DISPATCHER_CLK_EN` = `BIT(18)` (`mac_reg.h:386`)
- **Total Payload Value:** `0x00040000`

### 1.3 Setup Packet Fields (Write)
- **bmRequestType:** `0x40` (Vendor Request, Host to Device, Endpoint 0)
- **bRequest:** `0x05` (Realtek USB Write Request)
- **wValue:** `0x8404` (Lower 16 bits of the register address)
- **wIndex:** `0x0000` (Upper 16 bits of the register address)
- **wLength:** `0x0004` (4 bytes, for a 32-bit write)

### 1.4 Setup Packet Fields (Read-Back)
- **bmRequestType:** `0xC0` (Vendor Request, Device to Host, Endpoint 0)
- **bRequest:** `0x05` (Realtek USB Read Request)
- **wValue:** `0x8404` (Lower 16 bits of the register address)
- **wIndex:** `0x0000` (Upper 16 bits of the register address)
- **wLength:** `0x0004` (4 bytes, for a 32-bit read)

## 2. Execution Evidence

### 2.1 Payload Bytes (Hex)
- **Bytes Written (Little Endian):** `0x00 0x00 0x04 0x00`
- **Bytes Read Back (Little Endian):** `0x00 0x00 0x04 0x00`
- **Interpreted 32-bit Value:** `0x00040000`

### 2.2 Vextryn Serial Log Snippet
```text
[INFO] [RTL8852] Submitting Vendor Control Write for Register 0x8404 (DMAC_CLK_EN) with 0x40000...
[INFO] [xHCI] Event 32 completed with code 15
[INFO] [xHCI] Ignoring non-fatal code 15, waiting for final status...
[INFO] [xHCI] Event 32 completed with code 15
[INFO] [xHCI] Ignoring non-fatal code 15, waiting for final status...
[INFO] [RTL8852] Vendor Control Write to DMAC_CLK_EN completed.
[INFO] [RTL8852] Submitting Vendor Control Readback for Register 0x8404 (DMAC_CLK_EN)...
[INFO] [xHCI] Event 32 completed with code 15
[INFO] [xHCI] Ignoring non-fatal code 15, waiting for final status...
[INFO] [xHCI] Event 32 completed with code 15
[INFO] [xHCI] Ignoring non-fatal code 15, waiting for final status...
[INFO] [RTL8852] Readback DMAC_CLK_EN: 0x40000
```

### 2.3 Expected Value vs Observed Value
- **Expected Value:** `0x00040000`
- **Observed Value:** `0x00040000`
- **Result:** MATCH

## 3. Evaluation

### PROVEN
- The physical adapter is fully enumerated.
- Set Configuration has succeeded.
- The step is strictly source-backed and perfectly in-order.
- Vextryn Air performed the 32-bit vendor write to `0x8404`.
- Vextryn Air performed the 32-bit vendor read-back from `0x8404`.
- The `BIT(18)` clock-enable flag safely latched into hardware memory.

### INFERRED
- Because the readback matched the write, it implies the DMAC subsystem (enabled in the previous step) is actively accepting clock configuration changes. 
- The hardware clock tree for the dispatcher should now be un-gated.

### UNKNOWN
- We do not know if additional subsystems (e.g. `DLE`, `HCI FC`) must be initialized before firmware upload, as the function `dmac_pre_init` subsequently calls `dle_init` and `hfc_init` depending on conditions.
