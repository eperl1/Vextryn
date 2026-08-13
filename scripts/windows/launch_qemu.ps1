param(
    [string]$IsoPath = ""
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

function Resolve-IsoPath {
    param([string]$RequestedPath)

    if ($RequestedPath) {
        $candidate = Resolve-Path $RequestedPath -ErrorAction SilentlyContinue
        if ($candidate) { return $candidate.Path }
        throw "ISO not found at $RequestedPath"
    }

    $defaults = @(
        (Join-Path $root "vextryn-air-updated.iso"),
        (Join-Path $root "build_out\vextryn_air.iso"),
        (Join-Path $root "build\vextryn_air.iso"),
        (Join-Path $root "vextryn-air.iso")
    )

    foreach ($candidate in $defaults) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    throw "No ISO found. Build one first or pass -IsoPath."
}

$qemu = Resolve-QemuExe
$iso = Resolve-IsoPath -RequestedPath $IsoPath
$serialLog = Join-Path $root "vxair-serial.log"
$qmpSocket = "tcp:127.0.0.1:4444,server,nowait"
$bridgeScript = Join-Path $PSScriptRoot "start_chromium_bridge.ps1"
$accelArgs = @("-accel", "tcg,thread=multi,tb-size=1024")
$cpuArg = @("-cpu", "max")

$hypervisorPlatform = Get-WindowsOptionalFeature -Online -FeatureName HypervisorPlatform -ErrorAction SilentlyContinue
if ($hypervisorPlatform -and $hypervisorPlatform.State -eq "Enabled") {
    $accelArgs = @("-accel", "whpx")
    $cpuArg = @("-cpu", "qemu64")
}

if (Test-Path $bridgeScript) {
    Write-Host "Starting host Chromium bridge"
    & powershell -ExecutionPolicy Bypass -File $bridgeScript
}

$args = @(
    "-cdrom", $iso,
    "-m", "5120M",
    "-smp", "4",
    $cpuArg,
    "-machine", "q35",
    $accelArgs,
    "-vga", "std",
    "-netdev", "user,id=vxnet,hostname=vextryn-air,hostfwd=tcp::8088-:80",
    "-device", "rtl8139,netdev=vxnet",
    "-display", "gtk,show-cursor=off",
    "-no-reboot",
    "-serial", "file:$serialLog",
    "-qmp", $qmpSocket
)

Write-Host "Launching QEMU in a visible window"
Write-Host "  ISO: $iso"
Write-Host "  Serial log: $serialLog"
Write-Host "  QMP: $qmpSocket"
Write-Host "  Acceleration: $($accelArgs[1])"

& $qemu @args
