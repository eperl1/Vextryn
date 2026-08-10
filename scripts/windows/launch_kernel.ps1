param(
    [string]$KernelPath = ""
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

function Resolve-QemuExe {
    $cmd = Get-Command qemu-system-x86_64.exe -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    $candidates = @(
        "${env:ProgramFiles}\qemu\qemu-system-x86_64.exe",
        "${env:ProgramFiles(x86)}\qemu\qemu-system-x86_64.exe"
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path $candidate)) {
            return $candidate
        }
    }

    throw "QEMU was not found. Install it with: winget install --source winget SoftwareFreedomConservancy.QEMU"
}

function Resolve-KernelPath {
    param([string]$RequestedPath)

    if ($RequestedPath) {
        $candidate = Resolve-Path $RequestedPath -ErrorAction SilentlyContinue
        if ($candidate) { return $candidate.Path }
        throw "Kernel ELF not found at $RequestedPath"
    }

    $defaults = @(
        (Join-Path $root "build-out-windows-fresh\bin\vextryn_air.elf"),
        (Join-Path $root "build-out-windows\bin\vextryn_air.elf"),
        (Join-Path $root "build-out\bin\vextryn_air.elf"),
        (Join-Path $root "build\bin\vextryn_air.elf")
    )

    foreach ($candidate in $defaults) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "No kernel ELF found. Build one first or pass -KernelPath."
}

$qemu = Resolve-QemuExe
$kernel = Resolve-KernelPath -RequestedPath $KernelPath

Write-Host "Launching QEMU with direct kernel boot"
Write-Host "  Kernel: $kernel"

& $qemu `
    -kernel $kernel `
    -m 5120M `
    -smp 4 `
    -machine q35 `
    -vga std `
    -netdev user,id=vxnet,hostname=vextryn-air,hostfwd=tcp::8088-:80 `
    -device rtl8139,netdev=vxnet `
    -display sdl `
    -no-reboot
