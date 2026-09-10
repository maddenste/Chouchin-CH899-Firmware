# Copyright (C) 2026 Steve Madden
# SPDX-License-Identifier: GPL-3.0-or-later

param(
    [string]$InputHtml = (Join-Path $PSScriptRoot '..\web\index.html'),
    [string]$OutputHeader = (Join-Path $PSScriptRoot '..\web_ui.h')
)

$ErrorActionPreference = 'Stop'
$html = [System.IO.File]::ReadAllText($InputHtml, [Text.Encoding]::UTF8)
$delimiter = 'CH899_WEB'
if ($html.Contains((')' + $delimiter + '"'))) {
    throw ('The HTML contains the generated C++ raw-string terminator: )' + $delimiter + '"')
}

$header = "// Copyright (C) 2026 Steve Madden`n" +
    "// SPDX-License-Identifier: GPL-3.0-or-later`n`n" +
    "#pragma once`n`n" +
    "// Generated from web/index.html by tools/embed_web_ui.ps1. Do not edit here.`n" +
    "// Edit the HTML source, run this script, then compile the Arduino sketch.`n" +
    'static const char CLOCK_WEB_UI[] PROGMEM = R"' + $delimiter + "(`n" +
    $html + "`n)" + $delimiter + '";' + "`n"

[System.IO.File]::WriteAllText($OutputHeader, $header, [Text.UTF8Encoding]::new($false))
Write-Host "Embedded $($html.Length) characters into $OutputHeader"
