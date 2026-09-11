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
The planned first release is documented in the [changelog](CHANGELOG.md).

> **v1.0.0 candidate status.** The current source was compiled, flashed and
> validated on the owner's compatible movement on 11 September 2026. A matching
> application export and clean 1 MiB recovery image are retained locally with
> SHA-256 values. See
> [Testing status](docs/TESTING.md) and the [review record](docs/REVIEW-2026-09-10.md).

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
- A unique DHCP hostname and local mDNS address of the form
  `wifi-clock-<chip-id>` and `http://wifi-clock-<chip-id>.local/` while the
  ESP is awake.
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
- The setup AP is open and the HTTP settings/reset API has no authentication.
  Anyone able to reach it during a wake window can change settings. v1.0.0 is
  intentionally for a trusted local network; do not expose it through port
  forwarding or treat it as access-controlled.
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
4. See [Testing status](docs/TESTING.md) for what is confirmed and what remains
   under test.
5. Read the [10 September review](docs/REVIEW-2026-09-10.md) for source fixes,
   validation and remaining limitations.
6. Start the next session with the [current handoff](docs/MORNING_HANDOFF.md)
   and [release checklist](docs/RELEASE_CHECKLIST.md).

## Repository layout

- `arduino/CH899_Clock/` — editable Arduino source and embedded setup page.
- `docs/` — journey, flashing guide, findings, testing record and labelled
  photographs.
- `tools/` — PowerShell research helpers and dependency-free page tests.
- `firmware/release-candidate/` — local candidate binaries; intentionally not
  committed. Formal downloads will be attached to GitHub Releases.
- `private/` — excluded from Git; contains personal UART captures, original
  vendor firmware and historical experiments.

## Before the first public release

- Create the initial Git commit and inspect the exact staged file list.
- Generate `SHA256SUMS.txt` from the retained v1.0.0 application and factory
  images.
- Create the GitHub repository, push the source, and inspect the release title,
  tag, assets and checksums before publishing.

No original vendor ESP firmware, raw UART captures, or extracted stock web
assets are included in the public project.
