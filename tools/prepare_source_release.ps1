# Copyright (C) 2026 Steve Madden
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '..\release-preparation')
)
$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { throw 'Git is required to enumerate the public source set.' }
$files = @(& git -C $root -c core.quotepath=false ls-files --cached --others --exclude-standard)
if ($LASTEXITCODE -ne 0 -or $files.Count -eq 0) { throw 'Could not enumerate the public source files.' }
$files = @($files | Sort-Object -Unique)
$allowedRootFiles = @('README.md', 'CHANGELOG.md', '.gitignore', '.gitattributes', 'LICENSE', 'LICENSE.md', 'NOTICE', 'NOTICE.md', 'THIRD_PARTY_NOTICES.md')
$allowedExtensions = @('.md', '.ino', '.h', '.html', '.ps1', '.cjs', '.py', '.svg', '.jpg', '.jpeg', '.png')
foreach ($relative in $files) {
    $relative = $relative.Replace('\', '/')
    if ($relative -match '(^|/)(private|build|scratch|release-preparation|\.git)(/|$)' -or
        $relative -match '(^|/)\.\.?(/|$)' -or [IO.Path]::IsPathRooted($relative)) {
        throw 'A private/generated or invalid path appeared in the public file set.'
    }
    $isRoot = $relative.IndexOf('/') -lt 0
    if (($isRoot -and $relative -notin $allowedRootFiles) -or
        (-not $isRoot -and $relative -notmatch '^(arduino/CH899_Clock/|docs/|tools/|firmware/release-candidate/README\.md$)')) {
        throw "Unapproved source path: $relative"
    }
    $extension = [IO.Path]::GetExtension($relative).ToLowerInvariant()
    if (-not $isRoot -and $extension -notin $allowedExtensions -and $relative -ne 'arduino/CH899_Clock/.gitignore') {
        throw "Unapproved source file type: $relative"
    }
    $item = Get-Item -LiteralPath (Join-Path $root $relative)
    if ($item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "Unexpected directory or link: $relative"
    }
}
# Reproduce the embedded-asset check without modifying a source file.
$html = [IO.File]::ReadAllText((Join-Path $root 'arduino/CH899_Clock/web/index.html'))
$header = [IO.File]::ReadAllText((Join-Path $root 'arduino/CH899_Clock/web_ui.h'))
$prefix = 'R"CH899_WEB(' + "`n"
$suffix = "`n" + ')CH899_WEB";'
$start = $header.IndexOf($prefix)
$finish = $header.LastIndexOf($suffix)
if ($start -lt 0 -or $finish -lt $start -or $header.Substring($start + $prefix.Length, $finish - $start - $prefix.Length) -cne $html) {
    throw 'Regenerate web_ui.h before creating a source archive.'
}
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$stamp = [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff')
$zipPath = Join-Path $OutputDirectory ("clock-source-review-$stamp.zip")
$manifestPath = Join-Path $OutputDirectory ("clock-source-review-$stamp.manifest.json")
Add-Type -AssemblyName System.IO.Compression
$entries = @()
$zipStream = [IO.File]::Open($zipPath, [IO.FileMode]::CreateNew, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
$archive = [IO.Compression.ZipArchive]::new($zipStream, [IO.Compression.ZipArchiveMode]::Create, $false)
try {
    foreach ($relative in $files) {
        $relative = $relative.Replace('\', '/')
        $path = Join-Path $root $relative
        $bytes = [IO.File]::ReadAllBytes($path)
        $hasher = [Security.Cryptography.SHA256]::Create()
        try { $hash = ([BitConverter]::ToString($hasher.ComputeHash($bytes))).Replace('-', '').ToLowerInvariant() }
        finally { $hasher.Dispose() }
        $entries += [pscustomobject]@{ path = $relative; bytes = $bytes.Length; sha256 = $hash }
        $entry = $archive.CreateEntry($relative, [IO.Compression.CompressionLevel]::Optimal)
        $entryStream = $entry.Open()
        try { $entryStream.Write($bytes, 0, $bytes.Length) }
        finally { $entryStream.Dispose() }
    }
}
finally { $archive.Dispose(); $zipStream.Dispose() }

# Independently reread every archived entry and verify against the source hash.
$verifyStream = [IO.File]::OpenRead($zipPath)
$verifyArchive = [IO.Compression.ZipArchive]::new($verifyStream, [IO.Compression.ZipArchiveMode]::Read)
try {
    if ($verifyArchive.Entries.Count -ne $entries.Count) { throw 'Archive entry count mismatch.' }
    foreach ($expected in $entries) {
        $entry = $verifyArchive.GetEntry($expected.path)
        if ($null -eq $entry -or $entry.Length -ne $expected.bytes) { throw 'Archive size mismatch.' }
        $stream = $entry.Open()
        $hasher = [Security.Cryptography.SHA256]::Create()
        try { $actual = ([BitConverter]::ToString($hasher.ComputeHash($stream))).Replace('-', '').ToLowerInvariant() }
        finally { $stream.Dispose(); $hasher.Dispose() }
        if ($actual -ne $expected.sha256) { throw 'Archive content hash mismatch.' }
    }
}
finally { $verifyArchive.Dispose(); $verifyStream.Dispose() }
$manifest = [ordered]@{
    status = 'Source-review snapshot; not a compiled or hardware-validated firmware release'
    createdUtc = [DateTime]::UtcNow.ToString('o')
    archive = [IO.Path]::GetFileName($zipPath)
    archiveSha256 = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash.ToLowerInvariant()
    files = $entries
}
$json = $manifest | ConvertTo-Json -Depth 5
[IO.File]::WriteAllText($manifestPath, $json + [Environment]::NewLine, [Text.UTF8Encoding]::new($false))
Write-Host "Verified $($entries.Count) source/archive entries."
Write-Host "Source snapshot: $zipPath"
Write-Host "Manifest: $manifestPath"
Write-Host 'No firmware binaries, private evidence, Git metadata, builds or credentials are intentionally included.'
Write-Host 'This is a local review snapshot. Licence, compilation and hardware release checks remain pending.'
