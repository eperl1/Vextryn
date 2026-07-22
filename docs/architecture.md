# Vextryn Air Architecture

## Overview
Vextryn Air is an ultra-lightweight, high-performance x86_64 custom operating system. It features a microkernel-hybrid design to keep memory usage minimal (target < 128 MB idle) while ensuring rapid boot times and full system capabilities.

## Subsystems

1. **Bootloader & Early Init**
   - Pure UEFI entry (`EfiMain`) or Multiboot2 fallback.
   - Handles memory map acquisition, GOP framebuffer setup, and transitions to 64-bit long mode with a 4-level paging identity map.
   - Passes `vxair_boot_info` to the kernel.

2. **Kernel Core (`kernel/core/`)**
   - **Memory Management**: Bitmap-based PMM and 4-level VMM with demand paging.
   - **Task Management**: O(1) priority scheduler, thread control blocks, and SMP-ready run queues.
   - **IPC**: Message queues, shared memory, and async channels.
   - **Interrupts**: Full IDT, APIC/IOAPIC integration, and SYSCALL/SYSRET table.

3. **Hardware Abstraction Layer (HAL)**
   - ACPI parsing (RSDP, MADT, FADT, HPET, MCFG).
   - PCI/PCIe enumeration and MSI-X configuration.
   - USB xHCI host controller support.

4. **Wireless & Networking Stack**
   - Native 802.11 MAC layer with WPA3-SAE and WPA2-PSK support.
   - DHCP and DNS resolution.
   - Full TCP/IP stack (IPv4/IPv6), UDP, and minimal TLS 1.3.

5. **File System & Storage**
   - VFS layer abstracting `VxAirFS` (native), FAT32, and ext2.
   - NVMe and AHCI/SATA drivers for fast storage access.

6. **User Space & GUI**
   - `vxlibc` (minimal C stdlib) and `vxcrt` (C++ runtime).
   - `vxsh` system shell and PID 1 init system.
   - `vxcomp` (Wayland-inspired compositor) and `VxUI` widget toolkit for a clean, modern aesthetic.
