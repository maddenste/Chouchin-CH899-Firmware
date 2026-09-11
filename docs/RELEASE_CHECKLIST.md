# v1.0.0 release checklist (completed)

The v1.0.0 source has been compiled, flashed and hardware-validated on the
owner's compatible movement. The source, release assets and checksum manifest
were published on 11 September 2026.

## Offline

- [x] Run `node tools/test_web_ui.cjs` and retain the result (28 checks pass).
- [x] Run `powershell -NoProfile -File tools/check_release.ps1`.
- [x] Compile with the documented ESP8285/core settings and inspect warnings.
- [x] Export the v1.0.0 application image and record its SHA-256.
- [x] Confirm generated `web_ui.h` matches `web/index.html`.

## On a clock, with recovery available

- [x] Fresh provisioning, saved provisioning, wrong Wi-Fi password and hidden SSID.
- [x] AP captive browser, direct AP address and direct LAN address all work.
- [x] Save survives restart; blank password preserves only the same SSID's password.
- [x] Custom NTP works; failure falls back to `pool.ntp.org`; late reply recovers.
- [x] Blank NTP/timezone use documented defaults; invalid inputs are rejected.
- [x] Winter/summer offsets and emitted local `+TIME` format are correct.
- [x] `USER_OK` continues while waiting for valid time; no invented time is sent.
- [x] `+TIME` repeats once per second once ready, under MM32 wake control.
- [x] Page heartbeat starts only from an opened page, is two-second cadence,
      precedes first time announcement when applicable, and caps at one minute.
- [x] Close/reload/two tabs/lost responses do not revive or extend an old session.
- [x] Web Factory reset confirmation, clearing and reboot work.
- [x] M.SET+REC produces one committed clear, `CLEAN_OK` before restart, an
      unconfigured AP after boot, and no unnecessary duplicate flash writes.
- [x] Observe a daily wake from the reviewed replacement at a supported time
      (`09:00`, `10:00`, `21:00`, or `22:00`).
- [x] Recovery from the owner's backup is understood and verified.

## Publication record

- [x] Choose GPL-3.0-or-later and original-project attribution terms.
- [x] Document the trusted-local-network configuration-access policy in [Security](SECURITY.md).
- [x] Audit the public file list and supplied screenshot metadata.
- [x] Exclude vendor binaries, credentials, captures and private research assets.
- [x] Separate old candidate binaries from the v1.0.0 application and factory images.
- [x] Document tested hardware, source revision, settings and limitations.
- [x] Create the initial Git commit and inspect the staged file list.
- [x] Generate `SHA256SUMS.txt`, inspect the release title, tag, assets and checksums, and publish v1.0.0.

The source ZIP generator is a packaging aid. It does not publish a release.
