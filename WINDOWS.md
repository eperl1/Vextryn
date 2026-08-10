# Vextryn Air On Windows

This repo keeps the OS source unchanged. The pure Windows workflow is:

1. Install the Windows tools:
   ```powershell
   winget install --source winget Kitware.CMake Ninja-build.Ninja NASM.NASM SoftwareFreedomConservancy.QEMU
   ```
2. Install the Windows-hosted `x86_64-elf` cross toolchain:
   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts/windows/bootstrap_toolchain.ps1
   ```
3. Build the OS from Windows:
   ```powershell
   python scripts/build.py
   ```
4. Boot the kernel directly in a visible QEMU window:
   ```powershell
   powershell -ExecutionPolicy Bypass -File scripts/windows/launch_kernel.ps1
   ```

The build script uses the Windows `x86_64-elf` toolchain and generates the small standard-header shims it needs at build time. That keeps the OS code unchanged while still letting the project compile natively on Windows.

If you only want to boot the OS, you do not need WSL or GRUB. The direct kernel launcher looks for `build-out-windows\bin\vextryn_air.elf`, then `build-out\bin\vextryn_air.elf`, then `build\bin\vextryn_air.elf`.
