# Copyright (C) 2026 Steve Madden
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Port = 'COM4',
    [ValidateRange(300, 2000000)]
    [int]$Baud = 115200,
    [ValidateRange(1, 3600)]
    [int]$Seconds = 90,
    [string]$OutputPath = (Join-Path $PSScriptRoot ('..\private\captures\uart-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff') + '.bin'))
)

$ErrorActionPreference = 'Stop'
$OutputPath = [System.IO.Path]::GetFullPath($OutputPath)
if (Test-Path -LiteralPath $OutputPath) { throw 'Capture already exists; choose a new output filename.' }
$parent = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Path $parent -Force | Out-Null
$serial = [System.IO.Ports.SerialPort]::new($Port, $Baud)
$serial.ReadTimeout = 100
$stream = $null
$total = 0L
$complete = $false

try {
    $serial.Open()
    $stream = [System.IO.File]::Open($OutputPath, [System.IO.FileMode]::CreateNew, [System.IO.FileAccess]::Write, [System.IO.FileShare]::Read)
    $watch = [System.Diagnostics.Stopwatch]::StartNew()
    while ($watch.Elapsed.TotalSeconds -lt $Seconds) {
        $count = $serial.BytesToRead
        if ($count -gt 0) {
            $bytes = [byte[]]::new($count)
            $read = $serial.Read($bytes, 0, $count)
            $stream.Write($bytes, 0, $read)
            $total += $read
        }
        Start-Sleep -Milliseconds 10
    }
    $complete = $true
}
finally {
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
    if ($null -ne $stream) {
        $stream.Dispose()
        $state = if ($complete) { 'complete' } else { 'partial; capture was interrupted' }
        Write-Host "Saved $total bytes ($state) to $OutputPath"
    }
}
