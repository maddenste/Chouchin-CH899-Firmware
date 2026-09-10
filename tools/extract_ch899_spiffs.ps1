# Copyright (C) 2026 Steve Madden
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [Parameter(Mandatory = $true)]
    [string]$InputImage,
    [string]$OutputDirectory = (Join-Path (Split-Path -Parent $PSScriptRoot) 'private\stock-web-ui-extracted')
)

$ErrorActionPreference = 'Stop'
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $OutputDirectory) {
    throw 'Choose a new output directory to preserve earlier extraction evidence.'
}

# Determined from the captured 1 MiB CH-899 ESP8285 image. It is a 128-byte-page
# SPIFFS volume starting at 0xE4000. Object 0x0001 is the web UI, index.html.
$fsStart = 0xE4000
$fsEnd = 0xF0000
$pageSize = 0x80
$pageHeaderSize = 5
$pageDataSize = $pageSize - $pageHeaderSize
$webObjectId = 0x0001
$usedDataFlags = 0xFC

$image = [System.IO.File]::ReadAllBytes($InputImage)
if ($image.Length -ne 0x100000) {
    throw 'Expected a complete 1 MiB stock ESP dump. This helper is not a general SPIFFS parser.'
}

$pages = @{}
for ($offset = $fsStart; $offset -lt $fsEnd; $offset += $pageSize) {
    $objectId = [BitConverter]::ToUInt16($image, $offset)
    $span = [BitConverter]::ToUInt16($image, $offset + 2)
    $flags = $image[$offset + 4]

    if ($objectId -eq $webObjectId -and $flags -eq $usedDataFlags) {
        if ($pages.ContainsKey($span)) {
            throw 'Ambiguous duplicate data span; this dump needs manual SPIFFS analysis.'
        }
        $pages[$span] = $offset
    }
}

if ($pages.Count -eq 0) {
    throw 'No web UI data pages were found. Do not write this image to the clock.'
}

$orderedSpans = @($pages.Keys | Sort-Object)
for ($i = 0; $i -lt $orderedSpans.Count; $i++) {
    if ($orderedSpans[$i] -ne $i) { throw 'Data spans are incomplete or noncontiguous; extraction stopped.' }
}
$stream = [System.IO.MemoryStream]::new()
foreach ($span in $orderedSpans) {
    $pageOffset = $pages[$span] + $pageHeaderSize
    $stream.Write($image, $pageOffset, $pageDataSize)
}
$webBytes = $stream.ToArray()
$stream.Dispose()

# The last SPIFFS page is padded with erased-flash bytes, not part of the HTML.
$lastByte = $webBytes.Length - 1
while ($lastByte -ge 0 -and $webBytes[$lastByte] -eq 0xFF) {
    $lastByte--
}
if ($lastByte -lt 0) { throw 'The selected object contains no data.' }
$webBytes = [byte[]]$webBytes[0..$lastByte]

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
[System.IO.File]::WriteAllBytes((Join-Path $OutputDirectory 'index.html'), $webBytes)

$sha256 = [System.Security.Cryptography.SHA256]::Create()
$hash = ($sha256.ComputeHash($webBytes) | ForEach-Object { $_.ToString('x2') }) -join ''
$sha256.Dispose()
$summary = @(
    'CH-899 ESP8285 web UI extraction',
    "Source image: $InputImage",
    ('SPIFFS range: 0x{0:X6}-0x{1:X6}' -f $fsStart, ($fsEnd - 1)),
    "Data pages: $($orderedSpans.Count)",
    "Extracted bytes: $($webBytes.Length)",
    "SHA-256: $hash",
    '',
    'This directory is for analysis only. Do not flash a modified image until a',
    'separate rebuild and verification process has been completed.'
)
Set-Content -LiteralPath (Join-Path $OutputDirectory 'EXTRACTION.txt') -Value $summary -Encoding utf8

Write-Host "Extracted $($webBytes.Length) bytes from $($orderedSpans.Count) SPIFFS pages to $OutputDirectory"
