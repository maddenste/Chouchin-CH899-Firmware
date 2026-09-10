# Copyright (C) 2026 Steve Madden
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$Port = 'COM4',
    [ValidateRange(300, 2000000)]
    [int]$Baud = 230400,
    [ValidateRange(0.01, 168)]
    [double]$Hours = 24,
    [string]$OutputFile = (Join-Path (Split-Path -Parent $PSScriptRoot) ("private\captures\dual-uart-{0}.log" -f [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmssZ')))
)

$ErrorActionPreference = 'Stop'
$OutputFile = [IO.Path]::GetFullPath($OutputFile)
if (Test-Path -LiteralPath $OutputFile) {
    throw 'Capture destination already exists; choose a new filename.'
}
$parent = Split-Path -Parent $OutputFile
if ($parent) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }

$serial = [IO.Ports.SerialPort]::new($Port, $Baud, [IO.Ports.Parity]::None, 8, [IO.Ports.StopBits]::One)
$serial.Handshake = [IO.Ports.Handshake]::None
$serial.DtrEnable = $false
$serial.RtsEnable = $false
$serial.ReadTimeout = 100
$serial.NewLine = "`n"
$serial.Encoding = [Text.Encoding]::ASCII
$writer = [IO.StreamWriter]::new($OutputFile, $false, [Text.UTF8Encoding]::new($false))
$writer.AutoFlush = $true
$deadline = [DateTime]::UtcNow.AddHours($Hours)
$stopped = $false

Write-Host "Capturing $Port at $Baud until $($deadline.ToString('o'))"
Write-Host "Output: $OutputFile"
Write-Host 'Marker keys: 1/2 M.SET down/up; 3/4 REC down/up; 5/6 both down/up; M manual; Q finish.'
try {
    $serial.Open()
    while (-not $stopped -and [DateTime]::UtcNow -lt $deadline) {
        try {
            $line = $serial.ReadLine().TrimEnd("`r")
            $writer.WriteLine("{0} {1}", [DateTime]::UtcNow.ToString('o'), $line)
        } catch [TimeoutException] {
            # A quiet UART is expected between wake sessions.
        }
        try {
            if ([Console]::KeyAvailable) {
                $key = [Console]::ReadKey($true).KeyChar
                if ($key -eq 'q' -or $key -eq 'Q') { $stopped = $true }
                elseif ('123456mM'.Contains($key)) { $serial.Write([string]$key) }
            }
        } catch [InvalidOperationException] {
            # Non-interactive consoles cannot add markers; capture continues.
        }
    }
} finally {
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
    $writer.Dispose()
}
Write-Host "Capture complete: $OutputFile"
