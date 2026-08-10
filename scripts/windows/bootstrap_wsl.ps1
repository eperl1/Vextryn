param(
    [string]$Distro = "Ubuntu"
)

$ErrorActionPreference = "Stop"

Write-Host "Installing WSL and the $Distro distro."
Write-Host "You may need to reboot after the WSL feature installs."

Start-Process -FilePath "wsl.exe" -ArgumentList @("--install", "--no-distribution") -Wait
Start-Process -FilePath "wsl.exe" -ArgumentList @("--install", "-d", $Distro) -Wait

Write-Host ""
Write-Host "After first boot into WSL, install the Linux build deps:"
Write-Host "  sudo apt update"
Write-Host "  sudo DEBIAN_FRONTEND=noninteractive apt install -y build-essential cmake ninja-build python3 python3-pip nasm gcc g++ binutils xorriso mtools ovmf qemu-system-x86 qemu-utils git curl wget gdb libgmp-dev libmpfr-dev libmpc-dev flex bison texinfo grub-pc-bin grub-efi-amd64-bin"
