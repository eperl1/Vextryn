# Vextryn Air
**Lightweight Custom Operating System**

Vextryn Air is a high-performance, ultra-lightweight custom operating system designed from scratch for the x86_64 architecture.

## Design Philosophy
- **Minimal footprint, maximum capability**
- **Air-light memory usage** (Target: < 128 MB idle RAM)
- **Sub-3-second cold boot to desktop**
- **Native wireless-first** (WiFi + Bluetooth from day one)
- **Clean, modern, beautiful minimal UI**

## Architecture
- **Boot**: UEFI + Multiboot2 (Fallback)
- **Kernel**: Microkernel-hybrid design (C17)
- **User Space**: C++20 framework with custom standard library (`vxlibc` / `vxcrt`)
- **Display**: Wayland-inspired compositing window manager (`vxcomp`)
- **File System**: `VxAirFS` native journal-lite extent-based FS

## Directory Structure
- `boot/` - UEFI loader and early initialization
- `kernel/` - Core kernel (PMM, VMM, Scheduler, IPC, HAL)
- `drivers/` - Storage, Network, Input, GPU bus drivers
- `net/` - TCP/IP stack, WiFi, Bluetooth
- `fs/` - Virtual File System and storage drivers
- `userspace/` - Standard libraries and system shell (`vxsh`)
- `gui/` - Compositor, Font renderer, and UI toolkit (`VxUI`)
- `build/` & `toolchain/` - Build system, linker scripts, and cross-compiler config

## Building
Use the top-level Makefile which wraps the underlying CMake build system:
```sh
make all
make run      # Boots Vextryn Air in QEMU
make iso      # Generates a bootable ISO
```

## License
Proprietary — Vextryn Project
