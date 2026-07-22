# Booting Vextryn Air on Real Hardware
## Requirements
- x86_64 CPU (2010 or newer)
- 512MB RAM minimum
- UEFI firmware (2011 or newer)
- USB drive 1GB+
## Flash to USB
sudo dd if=vextryn-air.iso of=/dev/sdX bs=4M status=progress conv=fsync
## BIOS Settings
- Secure Boot: DISABLED
- Boot Mode: UEFI (not Legacy/CSM)
- Boot Order: USB first
## Known Working Hardware
- QEMU/KVM (tested)
- VirtualBox (tested)
- Generic x86_64 (should work)
## Serial Debug
Connect USB-Serial adapter to COM1
115200 baud 8N1
minicom -D /dev/ttyUSB0 -b 115200
