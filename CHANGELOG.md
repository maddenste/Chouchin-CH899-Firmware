# Changelog

All notable changes to this project are documented in this file.

This project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Dates and release assets are added when a version is published.

## [1.0.0] — 2026-09-11

This is the first independently developed replacement ESP8285 firmware for
compatible CH-899 / CHOUCHIN Wi-Fi clock movements. It was compiled, flashed
and validated on the owner's compatible movement. The application image, full
1 MiB factory image and `SHA256SUMS.txt` are available from the
[v1.0.0 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.0).

### Added

- Wi-Fi provisioning page with 2.4 GHz scan, manual/hidden SSID entry and
  saved-password protection.
- Configurable NTP host with fallback to `pool.ntp.org`.
- Forty timezone presets with automatic or disabled daylight saving, plus a
  custom POSIX-rule option.
- The four MM32-observed daily update choices: `09:00`, `10:00`, `21:00`, and
  `22:00`.
- Unique local identity using `WiFi-Clock Setup-<chip-id>` for the AP,
  `wifi-clock-<chip-id>` for DHCP, and
  `wifi-clock-<chip-id>.local` for mDNS while the ESP is awake.
- Setup-page firmware identification and the `firmwareVersion` field in
  `/api/v1/status`.
- Factory reset for this replacement firmware's stored Wi-Fi and time
  configuration.
- MM32-compatible `USER` / `USER_OK`, `CLEAN` / `CLEAN_OK`, `+TICK`, and
  local-time `+TIME` UART behaviour based on direction-labelled captures.

### Deliberately omitted

- OTA firmware updates. The MM32 can reset the ESP during a wake window, so an
  interrupted-update recovery path has not been validated.

### Known limitations

- The MM32 controls ESP wake/reset timing; the page is not an always-on clock
  dashboard.
- The setup AP and local HTTP API are unauthenticated. Use only on a trusted
  local network and never expose the clock through port forwarding.
- Timezone presets were checked against IANA tzdata 2026.3 through 2035; later
  civil-time rule changes require a firmware update or a custom POSIX rule.
- Only the ESP firmware is replaced. The MM32 movement firmware is not
  modified or distributed by this project.

See [Testing status](docs/TESTING.md) and the
[release checklist](docs/RELEASE_CHECKLIST.md) for the validation and release
record.
