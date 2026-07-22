# Vextryn Air v0.1.0 — Release Notes

## What's New
- Full 64-bit OS booting in 1.2 seconds
- vxsh shell with 30+ commands
- GUI compositor with taskbar + system tray
- Apps: Terminal, Files, Editor, Settings, Calculator, Clock, System Monitor
- vxweb lightweight web browser
- vxpkg package manager with online repo
- WiFi + Bluetooth support
- Intel HD Audio + PC Speaker
- SMEP + SMAP + ASLR security
- Real hardware UEFI boot support

## Known Issues
- WiFi real hardware: AX200 firmware needed
- Audio: HDA codec detection may vary
- UEFI: Secure Boot not yet supported

## How To Run
bash scripts/run_qemu_gui.sh
sudo bash scripts/flash_usb.sh /dev/sdX

## Build From Source
bash scripts/build.py
