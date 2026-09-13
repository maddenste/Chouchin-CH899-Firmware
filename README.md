# CH-899 Clock ESP8285 Firmware

Copyright (C) 2026 Steve Madden.

An independently developed ESP8285 replacement firmware for the CH-899 /
CHOUCHIN Wi-Fi clock movement. It retains the observed MM32-to-ESP UART
handshake, obtains time through NTP, and provides a small phone-friendly setup
page. It is an unofficial compatibility project and is not affiliated with,
endorsed by, or supported by the movement manufacturer.

## Licence and third-party components

The original project source is licensed under
[GPL-3.0-or-later](LICENSE). This permits commercial use, modification and
redistribution provided the corresponding source and GPL terms are supplied.
Released firmware also contains components from the ESP8266 Arduino Core 3.1.2,
which retain their own terms. See [Third-party notices](THIRD_PARTY_NOTICES.md).
The current public release is documented in the [changelog](CHANGELOG.md).

> **v1.0.1 released 13 September 2026.** The source was compiled, flashed and
> validated on the owner's compatible movement. Download the application image,
> user manual and checksum manifest from the
> [v1.0.1 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.1).
> See [Testing status](docs/TESTING.md) and the
> [review record](docs/REVIEW-2026-09-10.md).

![Opened CH-899 movement](docs/images/01-open-movement-and-coils.jpg)

## What this project changes

The clock has two controllers:

- An **MM32SPIN** controller runs the movement, buttons, clock logic and the
  ESP wake/reset behaviour.
- An **ESP8285N08** in an ESP-01F module handles Wi-Fi and NTP.

This project replaces only the ESP firmware. It does not alter or attempt to
replace the MM32 movement firmware.

The replacement provides:

- 2.4 GHz Wi-Fi setup page with nearby-network scan and hidden-SSID support.
- Configurable NTP host, 40 verified timezone presets with automatic/disabled
  daylight saving and a custom POSIX option, plus the MM32's four confirmed
  daily update times: `09:00`, `10:00`, `21:00`, or `22:00`.
- Automatic fallback to `pool.ntp.org` when a custom NTP host fails.
- A setup AP named `wifi-clock-setup-<chip-id>` for first-time provisioning
  and Factory-reset recovery only.
- A unique DHCP hostname and local mDNS address of the form
  `wifi-clock-<chip-id>` and `http://wifi-clock-<chip-id>.local/` after the
  ESP has joined the saved network.
- A visible firmware version and matching `/api/v1/status` response field for
  reliable identification during support and future development.
- A Factory reset button that clears this firmware's stored configuration.
- MM32-compatible `USER` / `USER_OK`, `CLEAN` / `CLEAN_OK`, `+TICK`, and
  `+TIME` behaviour based on direction-labelled captures.

## Important limitations

- The MM32, not the ESP, decides when the ESP wakes and when it is reset/slept.
  Consequently the setup page is normally available only during a wake window.
- There is deliberately **no OTA update feature**. The ESP can be reset by the
  MM32 before an update finishes; an interrupted-update recovery procedure has
  not been validated. Details
  are in [Reverse-engineering findings](docs/REVERSE_ENGINEERING.md#why-this-build-has-no-ota).
- This is for owners of compatible CH-899 movements. Flashing incorrect
  firmware, applying 5 V to 3.3 V logic, or interrupting a write can stop the
  Wi-Fi portion working. Keep a personal full-flash backup before changing a
  clock.
- With valid saved settings, a normal or scheduled wake uses station Wi-Fi
  only. It gives the saved network ten seconds to connect, then stops Wi-Fi
  for that wake if it cannot connect; it does not expose a fallback AP. Use
  Factory reset to return the clock to setup-AP mode after a Wi-Fi change.
- The setup AP and HTTP settings/reset API have no authentication. Anyone able
  to reach the AP while it is in setup mode, or the clock's LAN address while
  it is awake, can change settings. Use only on a trusted local network; never
  expose it through port forwarding or treat it as access-controlled.
- Preset rules were checked against IANA tzdata 2026.3 through 2035, but
  governments can change civil-time policy after release. Custom POSIX rules
  are checked for supported syntax, not geographical correctness.
  See [Security and access limits](docs/SECURITY.md) for the API protections.

## Start here

1. Read the [reverse-engineering journey](docs/JOURNEY.md) for the full,
   chronological investigation, or use [Hardware and findings](docs/REVERSE_ENGINEERING.md)
   as the concise reference.
2. Follow [Flashing a clock](docs/FLASHING.md) exactly.
3. Read the [setup-page guide](docs/USER_GUIDE.md), or the
   [Arduino guide](arduino/CH899_Clock/README.md) to edit or build the source.
4. See [Testing status](docs/TESTING.md) for the initial validation record and
   the v1.0.1 hardware-acceptance results.
5. Read the [10 September review](docs/REVIEW-2026-09-10.md) for source fixes
   and their evidence.

## Repository layout

- `arduino/CH899_Clock/` — editable Arduino source and embedded setup page.
- `docs/` — journey, flashing guide, findings, testing record and labelled
  photographs.
- `tools/` — PowerShell research helpers and dependency-free page tests.
- `private/` — excluded from Git; contains personal UART captures, original
  vendor firmware and historical experiments.

## Downloads and verification

Download only from the
[v1.0.1 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.1).
It contains the application image, user manual and `SHA256SUMS.txt`. Verify the
downloaded files against that manifest before flashing. A clean installation
uses `erase_flash` followed by the application image; no prebuilt full-flash
image is required.

No original vendor ESP firmware, raw UART captures, or extracted stock web
assets are included in the public project.

## v1.0.1 changes

Version 1.0.1 uses a station-only policy once Wi-Fi settings have been saved:
the AP is not broadcast on normal daily wakes, and a failed association ends
the Wi-Fi session after ten seconds. Queued scans and page keepalives are
cancelled when Wi-Fi ends, so they cannot restart the radio for that wake.

The application image, user manual and matching checksum manifest are published
with the v1.0.1 GitHub Release.
