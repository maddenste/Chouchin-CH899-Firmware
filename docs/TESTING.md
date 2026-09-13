# Testing status

**v1.0.0 release, 11 September 2026:** the reviewed source was compiled,
flashed and validated on the owner's compatible CH-899 / CHOUCHIN movement.
The matching application export and its SHA-256 value are published on the
[v1.0.0 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.0).

**v1.0.1 acceptance, 12 September 2026:** final compatible-movement testing
completed successfully. This version adds the station-only wake policy and the
offline scan/page-session shutdown safeguards. Its application image, manual
and checksum manifest are published on the
[v1.0.1 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.1).

| Test | Status | Notes |
| --- | --- | --- |
| Identify ESP and read 1 MiB flash | Confirmed | ESP8285N08, 1 MiB flash. |
| Preserve original backup | Confirmed | Retained privately with SHA-256. |
| ESP bootloader flashing | Confirmed | Reliable at 57600 baud with MM32 held reset. |
| `USER` → `USER_OK` handshake | Confirmed on v1.0.0 | See adapter-specific wiring in FLASHING.md. |
| NTP connection and local `+TIME` format | Confirmed on v1.0.0 | Tested against saved Wi-Fi and NTP. |
| M.SET external ESP wake/reset | Confirmed | ESP boot banner and UART session observed. |
| Setup-page `+TICK` session | Confirmed on v1.0.0 | Browser-driven, two-second cadence, one-minute cap. |
| Factory reset page action | Confirmed on v1.0.0 | Clears replacement settings and residual SDK station credentials, then restarts ESP. |
| Custom NTP fallback | Confirmed on v1.0.0 | Falls back to `pool.ntp.org` after the configured custom-server attempt. |
| Replacement daily wake at supported time | Confirmed on v1.0.0 | Automatic wake succeeded using a supported on-the-hour setting. |
| Stock custom/non-`:00` schedule | Not supported by observed MM32 behaviour | Stock ESP stored and transmitted custom values, but the movement did not reliably wake for them. |
| M.SET + REC `CLEAN` behaviour | Confirmed on v1.0.0 | Repeated command confirms one clear/restart; blank-state retries acknowledge without another erase/restart. |
| OTA update | Deliberately not supported | See findings document. |
| Station-only wake and ten-second failed-association cutoff | Confirmed on v1.0.1 | AP with blank or invalid settings; valid saved settings do not start AP or captive DNS during normal wakes. |
| Offline scan and page-heartbeat shutdown | Confirmed on v1.0.1 | Queued scans cannot restart Wi-Fi; late scan results are discarded; page keepalives stop and buffered scan/keepalive requests are rejected. |

## Review checks completed

`node .\tools\test_web_ui.cjs` passes 28 page checks. These
execute the real page script with simulated network responses and cover:

- One-request save/restart, failed save, missing SSID and reset confirmation.
- Asynchronous scan polling, strongest duplicate SSID and manual entry last.
- Preserving form edits during status refresh and loading settings safely.
- Non-overlapping polling, request timeout, page-close fallback and restored pages.
- Exact equality of the HTML and generated embedded page.
- Token-bearing requests, stale sessions, lost save responses and heartbeat retries.
- The page offers only the four MM32-confirmed daily update choices.
- All 40 timezone presets populate correctly, resolve Automatic and Disabled
  modes to the intended POSIX rule, preserve custom rules, and reject a blank
  custom-rule submission in the browser.

`tools/verify_timezone_presets.py` separately passed all 40 presets against
IANA tzdata 2026.3. Automatic offset behaviour was compared for every UTC hour
from 10 September 2026 through the end of 2035, unless the rule exactly matched
the IANA TZif future POSIX footer. Disabled mode was checked against each
location's non-DST/base offset. This verifies the preset data, not execution on
the ESP or future government policy changes.

The installed ESP8266 Arduino core 3.1.2 source was inspected to confirm NTP
hostname ownership, scan behaviour and flash-map semantics. This complements,
but does not replace, the completed v1.0.0 hardware validation.

### v1.0.1 review and hardware acceptance, 12 September 2026

The offline-state fixes were checked with a temporary, external harness using
mechanically translated sketch functions and simulated Wi-Fi/time/UART APIs.
All 46 scenarios passed, including timeout boundaries, queued and late scans,
offline request rejection, heartbeat shutdown, and continued USER/CLEAN
handling. This is a software simulation, not an on-device test. The existing
28 page checks and 40 timezone-preset checks also passed again. A fresh
ESP8285 build with ESP8266 core 3.1.2 and all warnings enabled succeeded.
Final compatible-movement testing subsequently completed successfully.

## Publication status

The v1.0.0 source, binaries and checksum manifest are published. The release
assets were downloaded after publication and verified against `SHA256SUMS.txt`.
