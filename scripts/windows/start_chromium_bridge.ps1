param()

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$scriptPath = Join-Path $root "tools\chromium_bridge\vx_chromium_bridge.py"
$logPath = Join-Path $root "chromium-bridge.log"
$errLogPath = Join-Path $root "chromium-bridge.err.log"

if (-not (Test-Path $scriptPath)) {
    throw "Chromium bridge script not found at $scriptPath"
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) {
    throw "Python was not found. Install Python first."
}

$healthUrl = "http://127.0.0.1:8081/health"
try {
    $existing = Invoke-WebRequest -Uri $healthUrl -UseBasicParsing -TimeoutSec 2
    if ($existing.StatusCode -eq 200) {
        Write-Host "Chromium bridge already running at $healthUrl"
        exit 0
    }
} catch {
}

Write-Host "Ensuring Playwright is installed"
& python -m pip install --disable-pip-version-check playwright | Out-Host

Write-Host "Ensuring Chromium browser runtime is installed"
& python -m playwright install chromium | Out-Host

Write-Host "Starting Chromium bridge in background"
$args = @("-u", $scriptPath)
$proc = Start-Process -FilePath $python.Source -ArgumentList $args -WindowStyle Hidden -RedirectStandardOutput $logPath -RedirectStandardError $errLogPath -PassThru

Start-Sleep -Seconds 3

try {
    $resp = Invoke-WebRequest -Uri $healthUrl -UseBasicParsing -TimeoutSec 3
    if ($resp.StatusCode -eq 200) {
        Write-Host "Chromium bridge started"
        Write-Host "  PID: $($proc.Id)"
        Write-Host "  Log: $logPath"
        exit 0
    }
} catch {
}

throw "Chromium bridge failed to start. Check $logPath"
