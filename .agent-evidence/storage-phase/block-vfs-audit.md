# Vextryn Air OS - Storage and VFS Audit

## 1. Current Storage Capabilities
Currently, **the OS has zero actual storage capabilities.** 

* **VFS & Filesystems**: The VFS API (`fs/vfs.c`) is a stub. Filesystem implementations for Ext2, FAT32, and custom `vxairfs` (`fs/ext2.c`, `fs/fat32.c`, `fs/vxairfs.c`) are entirely stubbed out, returning `0` or `NULL`.
* **Ramdisk**: A basic RAM disk (`fs/ramdisk.c`) is implemented for memory read/write, but it is not initialized or mounted anywhere in the kernel.
* **Initrd**: Although GRUB passes `initrd.cpio.gz` via Multiboot2 (`iso_root/boot/grub/grub.cfg`), the kernel's early initialization (`boot/early_init.c`) only parses the Framebuffer tag (tag 8) and ignores the Module tag (tag 3). The kernel main routine (`kernel/core/src/vxair_main.c`) simply hardcodes a fake log message (`"INITRD: loaded 3 files"`) rather than actually loading the archive.
* **Drivers**: Storage drivers for AHCI (`ahci.c`) and NVMe (`nvme.c`) are empty stubs returning `0`. There is no IDE, ATA, or VirtIO block driver code present.
* **PCI**: A basic PCI enumerator exists (`bus_pci.c`) and maps devices, but it does not bind to any storage drivers.
* **File APIs (Syscalls)**: While the userspace `libc` (`userspace/libc/src/vxair_libc.c`) defines standard file system syscalls (open, read, write, close, mount), the kernel's syscall handler (`kernel/core/src/vxair_syscall.c`) only implements logging, yielding, and exiting (syscalls 0, 1, 2). File APIs fall through to a default case and return `-1`. 
* **CMake**: `CMakeLists.txt` compiles all `fs/*.c` and `drivers/*.c` files correctly into the kernel executable.

## 2. Shortest Viable Persistent-Disk Route
The shortest viable route to achieving persistent disk I/O on x86 is **ATA PIO Mode**. 
Unlike AHCI or NVMe which require setting up DMA, memory-mapped registers, and queues, ATA PIO only requires basic `inb`/`outb` instructions to the standard I/O ports (e.g., `0x1F0`). Alternatively, implementing a minimal **VirtIO Block** driver is also relatively straightforward since PCI enumeration is already present, but ATA PIO remains the absolute simplest for a rudimentary persistent block device without relying on PCI or DMA.

## 3. Can a Virtual Block Disk be Supported Now?
**No.** A virtual block disk (such as a QEMU IDE, AHCI, or VirtIO disk) cannot be supported in the current state. The OS lacks the necessary block device driver implementations, the VFS logic to route filesystem requests to a block device, and the kernel syscalls to expose file operations to userspace.
