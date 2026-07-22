# QEMU 9p Host Share Audit

## Overview
This report evaluates whether a QEMU 9p shared directory is realistically supportable in the current Vextryn_Air OS codebase.

## Subsystem Analysis

- **PCI Enumeration:** **Present.** The OS contains functional PCI enumeration in `kernel/hal/hal_pci.c` and `drivers/bus/bus_pci.c` (e.g., `vxair_bus_pci_scan`).
- **Virtio Abstractions:** **Missing.** There are no virtio drivers, no virtio-pci discovery logic, and no virtqueue management primitives implemented anywhere in the source tree.
- **9P Protocol:** **Missing.** The 9P2000 protocol client (for serializing/deserializing messages such as Tversion, Tattach, Twalk, Tread, etc.) does not exist.
- **VFS / Mount Support:** **Incomplete / Non-functional.** A rudimentary VFS node structure exists (`fs/vfs.h`), but it lacks global path resolution and mount registry logic. The global `vxair_vfs_root` is statically set to `NULL` and never populated, meaning `mount` operations currently just return isolated node pointers without integrating them into a file system tree.

## Conclusion & Implementation Plan
**Status: BLOCKED**

Implementing a QEMU 9p shared directory is currently blocked because fundamental prerequisites are entirely missing. Providing a safe, concrete implementation plan for 9p alone is impossible without first undertaking major architectural additions:
1. **VFS Overhaul:** Implementing path resolution (`open_path`), mount points, and dynamic VFS tree management.
2. **Virtio Core:** Building PCI device discovery for Virtio devices and implementing memory-mapped virtqueue management.
3. **9P Client:** Developing the 9P transport and message protocol from scratch.

No fake 9p support should be added. The blocking prerequisites must be implemented sequentially before attempting 9p shared directory support.
