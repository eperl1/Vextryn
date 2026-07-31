# Firmware Execution Plan Report

## 1. Files in Vextryn to Add/Change
- `drivers/usb/usb.c`: To implement the top-level loop for chunking the firmware and issuing bulk out transfers, and to add a `vxair_usb_bulk_out` primitive if one does not yet exist.
- `drivers/usb/usb.h`: To export the `vxair_usb_bulk_out` primitive.
- A new file (e.g., `drivers/usb/rtl8852a_fw_bin.h` or `rtl8852a_fw.c`) to store the actual firmware payload statically as a C byte array, since Vextryn may not yet have an active filesystem to read from `/lib/firmware/rtw89/rtw8852a_fw.bin` dynamically.

## 2. Exact Upload Call Sequence
1. Allocate a temporary staging buffer (maximum 2044 bytes).
2. Start a loop that iterates over the static firmware array.
3. For each 2020-byte slice (or remaining bytes if near the end):
   - Clear the staging buffer.
   - Populate the first 24 bytes with the Tx Descriptor.
   - Copy the 2020-byte slice immediately following the descriptor.
   - Invoke `vxair_usb_bulk_out(...)` (or equivalent) to submit the payload to the adapter.
   - Check the return code of the bulk submission.
4. After all chunks are sent, loop terminates.

## 3. Exact Packet Construction Format (One Chunk)
A single chunk wrapper is exactly 24 bytes long (`struct wd_body_t`) followed by the payload.
- **Bytes 0-3** (`dword0`): `0x001C0000` (little-endian: `0x00 0x00 0x1C 0x00`)
- **Bytes 4-7** (`dword1`): `0x00000000`
- **Bytes 8-11** (`dword2`): `chunk_size` (e.g. `2020 = 0x07E4`, little-endian: `0xE4 0x07 0x00 0x00`)
- **Bytes 12-15** (`dword3`): `0x00000000`
- **Bytes 16-19** (`dword4`): `0x00000000`
- **Bytes 20-23** (`dword5`): `0x00000000`
- **Bytes 24-N**: The raw firmware bytes for this chunk (up to 2020 bytes).

## 4. Exact Endpoint Address Target
Vextryn will target endpoint address **`0x07`** (EP 7 OUT). This corresponds to `BULKOUTID2`, which is the 3rd Bulk OUT endpoint enumerated by the adapter.

## 5. Maximum Chunk Payload and Wrapped Transfer Size
- **Max Payload Size**: 2020 bytes.
- **Max Wrapped Transfer Size**: 2044 bytes.

## 6. Preconditions Before First Attempt
- The DLE initialization and native USB data tables must have been successfully applied (already verified).
- The post-DLE / pre-FW boundary operations must have completed successfully, culminating with `mac_enable_cpu(..., dlfw = true)`.
- WDE and PLE memory regions must have reported `INI_RDY` status.

## 7. Observable Success and Failure Signals
- **Success Signal**: The bulk out transfer primitive returns successfully (or a written byte count matching the requested 2044 bytes) for every single chunk.
- **Failure Signal**: The bulk out transfer primitive returns an error (e.g., STALL, timeout, or a short write).

## 8. Rollback/Stop Conditions
- **Condition**: If any chunk transfer times out or is rejected (STALL) by the hardware.
- **Action**: Immediately abort the firmware upload loop. Do NOT attempt to push subsequent chunks. Log the offset and error code for debugging.

## 9. Statement of No Execution
No firmware upload or any bulk transfers have been executed in this planning pass.

## 10. PROVEN / INFERRED / UNKNOWN
- **PROVEN**: Tx Descriptor layout, 2020-byte chunk size, 0x07 endpoint target, required pre-FW initialization steps.
- **INFERRED**: The necessity of compiling the firmware as a static C array due to potential filesystem limitations at this stage of the kernel.
- **UNKNOWN**: The exact firmware payload (`rtw8852a_fw.bin`) byte content has not been downloaded or placed into the repository yet.
