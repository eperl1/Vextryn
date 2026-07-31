# D-Link DWA-X1850 Identity Reconciliation

## The Discrepancy
- The expected USB ID based on prior instructions was `2001:3321`.
- The physical device enumerated in Vextryn Air and passed through from the host actually reports `2001:332c`.

## Hardware Identity Resolution
Based on Linux driver databases (`rtw89` / `rtw8852au` / `rtw8852bu` families), these two PIDs represent **different hardware revisions** of the same product name:
1. **D-Link DWA-X1850 Rev A1**: `2001:3321` (This ID was historically used for DWA-182 and then reused for X1850 Rev A1).
2. **D-Link DWA-X1850 Rev B1**: `2001:332c` (This is the newer, physically attached device).

### 1. Host-Side Validation (Before Passthrough)
```
Bus 002 Device 028: ID 2001:332c D-Link Corp. 802.11ax WLAN Adapter
```
The host Linux `lsusb` explicitly identifies the physical hardware on Bus 2 as `2001:332c`. 
No `usb_modeswitch` command changed this ID; attempting to run `usb_modeswitch` returned `Error: can't use storage command ... interface class is 255`. This proves `332c` is its permanent Wi-Fi operating mode, not a temporary CD-ROM mode.

### 2. QEMU Passthrough Arguments
The QEMU invocation was explicitly targeted to pass through the hardware present on the bus:
```bash
-device usb-host,vendorid=0x2001,productid=0x332c,bus=xhci.0
```

### 3. Vextryn Air OS Enumeration Verification
Inside the Vextryn Air kernel, the device descriptor precisely parsed the same VID:PID:
```text
[INFO] [USB DEVICE ENUMERATED]
[INFO]   Port: 5
[INFO]   Speed: 3
[INFO]   VID:PID: 0x2001:0x332c
```

### 4. Conclusion & Official Target
The physical device in our possession is **D-Link DWA-X1850 Rev B1 (`2001:332c`)**.
It is not an alternate mode of `3321`, but a distinct hardware revision. Because we are building the driver against the physical adapter connected to this machine, **the official target hardware must now be permanently considered DWA-X1850 B1 @ `2001:332c`**.
