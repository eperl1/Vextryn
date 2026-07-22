# Building Vextryn Air

## Prerequisites
- **CMake** 3.10 or higher
- **x86_64-elf** cross-compiler toolchain (GCC/Binutils)
- **Python 3** (for the `build.py` orchestrator)
- **QEMU** or **VirtualBox** (for emulation)
- **grub-mkrescue** / **xorriso** (for ISO generation)

## Build Process

Vextryn Air uses CMake wrapped by a custom Python build script.

1. **Run the build orchestrator**:
   ```bash
   python scripts/build.py
   ```
   This will invoke CMake, configure the `x86_64-elf` toolchain, and compile the OS.

2. **Generate a bootable ISO**:
   ```bash
   python scripts/build.py --iso
   # OR
   ./scripts/mkiso.sh
   ```
   This produces `build/vextryn-air.iso`.

3. **Run in QEMU**:
   ```bash
   python scripts/build.py --run
   # OR
   ./scripts/run_qemu.sh
   ```

## Manual CMake Build
If you prefer to run CMake directly:
```bash
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain/x86_64-elf.cmake ..
make -j$(nproc)
```
