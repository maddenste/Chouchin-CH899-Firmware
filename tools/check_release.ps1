# Copyright (C) 2026 Steve Madden
# SPDX-License-Identifier: GPL-3.0-or-later

param()
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$files = @(& git -C $root -c core.quotepath=false ls-files --cached --others --exclude-standard | Sort-Object -Unique)
if ($LASTEXITCODE -ne 0 -or $files.Count -eq 0) { throw 'No public Git file set available.' }
foreach ($relative in $files) {
    if ($relative -match '(^|/)(private|build|scratch|release-preparation|\.git)(/|$)' -or $relative -match '\.(bin|elf|map|pcap|zip)$') {
        throw "Private/generated file in public set: $relative"
    }
    $path = Join-Path $root $relative
    $item = Get-Item -LiteralPath $path
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Link in public set: $relative" }
    if ($relative.EndsWith('.ps1')) {
        $tokens = $null; $parseErrors = $null
        [void][Management.Automation.Language.Parser]::ParseFile($path, [ref]$tokens, [ref]$parseErrors)
        if ($parseErrors.Count) { throw "PowerShell syntax error in $relative : $($parseErrors[0].Message)" }
    }
    if ($relative.EndsWith('.md')) {
        $content = [IO.File]::ReadAllText($path)
        foreach ($match in [regex]::Matches($content, '\]\(([^\s)]+)\)')) {
            $target = $match.Groups[1].Value
            if ($target -match '^(https?://|mailto:|#)') { continue }
            $target = [Uri]::UnescapeDataString(($target -split '#', 2)[0])
            if ($target -and -not (Test-Path -LiteralPath (Join-Path (Split-Path $path) $target))) {
                throw "Missing local Markdown target in $relative : $target"
            }
        }
    }
}
Push-Location $root
try {
    & node .\tools\test_web_ui.cjs
    if ($LASTEXITCODE -ne 0) { throw 'Page regression tests failed.' }
} finally { Pop-Location }
Write-Host "PASS: $($files.Count) public files checked; PowerShell syntax and local Markdown file links checked."
Write-Host 'This does not certify absence of secrets or validate image metadata.'
Write-Host 'Before publication, inspect the staged source set and draft release assets, including their SHA-256 values.'
