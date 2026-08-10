param(
    [string]$OutPath = "",
    [int]$Port = 4444
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not $OutPath) {
    $OutPath = Join-Path $root "qemu-framebuffer.ppm"
}

function Read-JsonLine {
    param([System.IO.StreamReader]$Reader)
    while ($true) {
        $line = $Reader.ReadLine()
        if ($null -eq $line) { return $null }
        $trimmed = $line.Trim()
        if ($trimmed.Length -gt 0) { return $trimmed }
    }
}

$client = New-Object System.Net.Sockets.TcpClient
$deadline = (Get-Date).AddSeconds(20)
while (-not $client.Connected -and (Get-Date) -lt $deadline) {
    try {
        $client.Connect("127.0.0.1", $Port)
    } catch {
        Start-Sleep -Milliseconds 250
    }
}

if (-not $client.Connected) {
    throw "Unable to connect to QEMU QMP on port $Port"
}

$stream = $client.GetStream()
$reader = New-Object System.IO.StreamReader($stream, [System.Text.Encoding]::ASCII)
$writer = New-Object System.IO.StreamWriter($stream, [System.Text.Encoding]::ASCII)
$writer.NewLine = "`r`n"
$writer.AutoFlush = $true

$greeting = Read-JsonLine -Reader $reader
if (-not $greeting) { throw "QMP did not send a greeting" }

$writer.WriteLine('{"execute":"qmp_capabilities"}')
$cap = Read-JsonLine -Reader $reader
if (-not $cap) { throw "QMP capabilities handshake failed" }

$escapedOut = $OutPath.Replace('\', '\\')
$cmd = '{"execute":"human-monitor-command","arguments":{"command-line":"screendump ' + $escapedOut + '"}}'
$writer.WriteLine($cmd)

$response = $null
for ($i = 0; $i -lt 20; $i++) {
    $response = Read-JsonLine -Reader $reader
    if ($response) { break }
    Start-Sleep -Milliseconds 100
}

$writer.WriteLine('{"execute":"quit"}')
$client.Close()

if (-not (Test-Path $OutPath)) {
    throw "QEMU did not create the framebuffer dump at $OutPath"
}

Write-Host $OutPath
