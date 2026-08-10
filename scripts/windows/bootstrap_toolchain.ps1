param(
    [string]$InstallRoot = ""
)

$ErrorActionPreference = "Stop"

if (-not $InstallRoot) {
    $InstallRoot = Join-Path $env:LOCALAPPDATA "Vextryn-Air\toolchains\x86_64-elf-tools"
}

$zip = Join-Path $env:TEMP "x86_64-elf-tools-windows.zip"
$url = "https://github.com/lordmilko/i686-elf-tools/releases/download/15.2.0/x86_64-elf-tools-windows.zip"

Write-Host "Downloading x86_64-elf toolchain to $zip"
curl.exe -L -o $zip $url

Write-Host "Installing toolchain to $InstallRoot"
New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
Expand-Archive -Path $zip -DestinationPath $InstallRoot -Force

$env:VEXTRYN_ELF_TOOLCHAIN = $InstallRoot
Write-Host "Toolchain installed."
Write-Host "Set VEXTRYN_ELF_TOOLCHAIN to: $InstallRoot"
