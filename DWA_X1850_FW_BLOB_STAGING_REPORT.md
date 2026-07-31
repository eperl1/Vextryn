# Firmware Blob Staging Report

## 1. Firmware Source Identity
In the Realtek reference driver, the firmware is dynamically loaded from `/lib/firmware/rtw89/rtw8852a_fw.bin` or fallback `rtl8852afw.bin`. To support environments without a filesystem, the reference driver embeds a static fallback in `phl/hal_g6/mac/fw_ax/rtl8852a/hal8852a_fw.c`. For our U2 Cut NIC adapter, the specific array used is `array_8852a_u2_nic`.

## 2. Exact Firmware Byte Size
The staged firmware payload is exactly **360304** bytes in size.

## 3. Exact Checksum
The SHA256 hash of the extracted and staged `array_8852a_u2_nic` bytes is:
`6b3143d1580dbeb7ec364d01125825eaeb23d2f603ca9dbc42862f078e5ac1f9`

## 4. Vextryn Files Added/Changed
The following files were created in Vextryn to stage the payload:
- `drivers/usb/rtl8852a_fw.c`
- `drivers/usb/rtl8852a_fw.h`

The build system (`CMakeLists.txt`) automatically ingested the `.c` file and linking was verified to succeed.

## 5. Exact Symbol/Buffer Name
Vextryn will use the following exposed symbols at upload time:
- Buffer: `rtl8852a_fw_u2_nic`
- Length: `rtl8852a_fw_u2_nic_len`

## 6. Staging Method
The firmware blob is **embedded statically** directly into the Vextryn kernel as a C byte array. This choice ensures deterministic memory access and circumvents the requirement for a functional Virtual File System (VFS) to be operational at this stage of the boot sequence.

## 7. Statement of No Execution
**No firmware bytes were transmitted in this pass.** The blob was merely extracted, hashed, generated as C source, and successfully compiled/linked into the Vextryn kernel image.

## 8. PROVEN / INFERRED / UNKNOWN
- **PROVEN**: The firmware blob identity, size, SHA-256 hash, symbol names, static embedding mechanism, and the Vextryn files altered to support the staging.
- **INFERRED**: None.
- **UNKNOWN**: None.
