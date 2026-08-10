param(
    [string]$OutPath = ""
)

$ErrorActionPreference = "Stop"

if (-not $OutPath) {
    $OutPath = Join-Path $PSScriptRoot "qemu-desktop.png"
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
$bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bitmap.Size)
$bitmap.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bitmap.Dispose()

Write-Host $OutPath
