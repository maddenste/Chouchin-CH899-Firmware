# CH-899 Clock Arduino Source

This folder is the editable ESP8285 replacement firmware used by the project.
It is a single application image: the setup page is compiled into flash from
`web/index.html`, so there is no LittleFS/SPIFFS upload step.

## Arduino IDE settings

The v1.0.1 release uses **ESP8266 by ESP8266 Community 3.1.2**.
Install that version in Boards Manager to reproduce the build environment,
then use:

| Setting | Value |
| --- | --- |
| Board | Generic ESP8285 Module |
| CPU frequency | 80 MHz |
| Flash size | Mapping defined by Hardware and Sketch |
| Flash mode | DOUT |
| Flash frequency | 40 MHz |
| Crystal frequency | 26 MHz |
| Upload speed | 115200 (Arduino export only; external flashing is normally 57600) |
| Reset method | no dtr (aka ck); actual flashing uses the explicit esptool options in the guide |

The retained Arduino FQBN is `esp8266:esp8266:esp8285:eesz=autoflash` with the
remaining core 3.1.2 board defaults. DOUT and 40 MHz are fixed by that ESP8285
board definition, so they may not appear as separate menu choices.

The sketch uses `FLASH_MAP_NO_FS`, giving no filesystem on the detected 1 MiB
flash. EEPROM emulation and SDK system sectors still occupy the end of flash.
OTA is absent because this project includes no update handler; the no-filesystem
map alone is not an OTA prohibition. The exported auto-mapped binary can carry
a larger nominal flash-size header, which the documented esptool command
overrides to the verified **1MB** size when writing.

## Edit the setup page

1. Edit `web/index.html`.
2. Regenerate the compiled header:

   ```powershell
   & 'E:\CH899-Clock-Firmware\arduino\CH899_Clock\tools\embed_web_ui.ps1'
   ```

3. In Arduino IDE, use **Verify**, then **Export compiled Binary**.
4. Locate the newly exported `CH899_Clock.ino.bin` (depending on IDE version,
   under the sketch or `build/esp8266.esp8266.esp8285/`). Record its SHA-256 and
   source revision; use that exact file in the flashing command or deliberately
   copy it to a versioned release filename after preserving the previous image.
5. Follow the project [flashing guide](../../docs/FLASHING.md).

`web_ui.h` is generated and must stay alongside the sketch. Do not edit it
directly.

Keep `clock_validation.h` and `clock_session.h` alongside the sketch too; they
implement input validation and page-session ownership.

The source identifies itself as `v1.0.1`. Final compatible-movement testing
completed on 12 September 2026, and the matching application image is published
on the
[v1.0.1 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.1).
Documentation or source changes do not rebuild that binary; a modified build
needs a fresh export, checksum and hardware test before distribution.

## Current behaviour

- With blank settings (a first flash or Factory reset), the clock starts the
  open setup AP `wifi-clock-setup-<chip-id>`. Once valid Wi-Fi settings are
  saved, all normal and scheduled wakes use station Wi-Fi only: the AP and
  captive DNS are not started.
- A saved network has ten seconds to associate and obtain a connection. If it
  fails, Wi-Fi is switched off for that wake without a retry or fallback AP.
  The next MM32 wake starts one fresh station attempt. Factory reset is the
  intentional recovery route after a router, SSID, or password change.
- A queued scan is cancelled when Wi-Fi is switched off. Late scan results and
  buffered page keepalives are discarded, so they cannot restart station Wi-Fi
  or extend `+TICK` after that wake has ended.
- The DHCP hostname is `wifi-clock-<chip-id>` and mDNS begins only after the
  station has connected, at `http://wifi-clock-<chip-id>.local/`.
- MM32 UART: accepts `USER` and returns `USER_OK` at 115200 baud. M.SET+REC
  produces repeated `CLEAN` commands. Like stock, the first arms the operation
  and a second command at the observed cadence confirms it; after a successful
  settings erase the ESP returns `CLEAN_OK` and restarts. If settings are
  already blank, later `CLEAN` commands receive `CLEAN_OK` without another
  erase or restart. Protocol output uses the stock CRLF ending.
- After a successful NTP response, sends a local-time `+TIME` record once per
  second until the MM32 ends the ESP wake window. The first record follows
  valid NTP by approximately one second. An active page session receives
  `+TICK` first and one further second before that first `+TIME`.
- Daily update time is restricted to the four values confirmed by stock page
  options and hardware testing: `09:00`, `10:00`, `21:00`, and `22:00`.
  Arbitrary values can be transmitted by the stock ESP but were not reliably
  honoured by the MM32. Older replacement settings are safely normalised to
  `10:00` when loaded.
- The page offers 40 timezone presets. Automatic mode uses the preset's complete
  POSIX DST rule; Disabled uses its fixed standard-time offset; Custom rule
  submits the entered POSIX string. Only the final rule is stored, preserving
  the existing EEPROM format. Presets were checked against IANA tzdata 2026.3
  from 10 September 2026 through 2035.
- A browser page session sends `+TICK` every two seconds, for up to one minute.
  A received browser close/navigation request stops it; a 10-second timeout
  is the fallback when a phone disappears or the browser does not deliver the
  request. Closing a browser cannot guarantee immediate delivery to the clock.
- The setup page refreshes connection status every two seconds.
- Save & update clock commits the new settings and schedules a restart in
  the same HTTP request. The browser stops polling when the request succeeds;
  after reboot, the firmware connects, requests NTP and resumes normal time
  announcements. A failed commit leaves the running settings unchanged.
- A blank password retains the password only when keeping the same SSID.
  Choosing another SSID with a blank password selects an open network. To clear
  credentials while keeping the exact same SSID, use Factory reset and set up
  the clock again.
- Wi-Fi scans run asynchronously and wait for an ongoing connection attempt to
  finish. The response is bounded to 40 scan entries; the page merges duplicate
  SSIDs and orders them by signal strength, with manual entry last.
- The chosen NTP host gets eight seconds before fallback to `pool.ntp.org`.
  If no reply arrives by the next deadline, the core continues its normal SNTP
  retries; a later reply can still update the clock within that wake window.
- Factory reset clears the 512-byte EEPROM configuration and residual SDK
  station credentials, then restarts the ESP. It does not erase the application
  firmware or guarantee secure erasure of every historical flash byte.

Unknown MM32 UART input is intentionally ignored until it has been captured and
decoded. Do not use `Serial` for diagnostic logging on a clock: the MM32 sees
the same UART and could interpret unexpected text as protocol data.

## Page regression checks

With Node.js installed, run from the repository root:

```powershell
node .\tools\test_web_ui.cjs
```

These checks execute the actual page JavaScript with a simulated browser and
HTTP responses, and verify the generated header matches the HTML. They do not
compile or exercise the ESP firmware or MM32 hardware.

With Python 3, `python-dateutil`, and `tzdata` installed, recheck every preset
against the IANA database with:

```powershell
python .\tools\verify_timezone_presets.py
```
