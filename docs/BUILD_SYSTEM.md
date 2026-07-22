# Vextryn Air OS - Build System Documentation

## Overview
The Vextryn Air build system uses **CMake** for dependency management and compilation, wrapped in a Python orchestrator (`scripts/build.py`) for ease of use.

## Toolchain
We use a custom cross-compiler toolchain defined in `toolchain/x86_64-elf.cmake`. This prevents the host OS's standard libraries from polluting the kernel build.

## Directory Structure
- `build/`: Contains linker scripts (`linker.ld`) and other low-level build configurations.
- `build_out/`: Automatically generated directory for compilation artifacts and ISO output.
- `scripts/`: Helper scripts for building, generating ISOs, and running emulators.
- `toolchain/`: CMake toolchain configurations.

## How to Build
```bash
# Clean build, compile, and generate bootable ISO
python3 scripts/build.py --clean --iso

# Build and run in QEMU immediately
python3 scripts/build.py --iso --run
```

## TODOs
- Finalize kernel base address in `build/linker.ld`.
- Integrate actual cross-compiler download/setup in CI.
- Add targets in `CMakeLists.txt` for kernel, modules, and userland.
