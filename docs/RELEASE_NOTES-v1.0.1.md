# WiFi Clock Firmware v1.0.1

Final compatible-movement testing completed successfully on 12 September 2026.

This update improves normal daily-wake privacy and radio behaviour after Wi-Fi
has been configured:

- Setup Wi-Fi is now named `wifi-clock-setup-<chip-id>`.
- With valid saved Wi-Fi settings, normal, M.SET and scheduled wakes use
  station mode only; the setup AP and captive DNS do not start.
- The clock makes one ten-second Wi-Fi association/DHCP attempt. A failure—or
  later loss of the station connection—ends Wi-Fi for that wake without an AP
  fallback or automatic retry.
- mDNS starts only after station connection at
  `http://wifi-clock-<chip-id>.local/`.
- Network shutdown cancels queued scans and page heartbeats. Late results and
  buffered page requests cannot restart Wi-Fi or extend `+TICK` after shutdown.

The firmware download is `CH899-Clock-v1.0.1-esp8285-application.bin`. Flash
it at `0x0` with the documented ESP8285, DOUT, 40 MHz and 1 MiB settings. For
a clean installation, erase the ESP flash completely first, then write the
application image. Always retain a personal original-firmware backup.

Verify every download against `SHA256SUMS.txt` before flashing. See
[Flashing a clock](https://github.com/maddenste/Chouchin-CH899-Firmware/blob/v1.0.1/docs/FLASHING.md)
and
[Clock setup](https://github.com/maddenste/Chouchin-CH899-Firmware/blob/v1.0.1/docs/USER_GUIDE.md).

This remains an unofficial, GPL-3.0-or-later replacement for compatible
CH-899 / CHOUCHIN movements. It replaces only the ESP8285 firmware; it does
not modify the MM32 movement controller. OTA updates remain deliberately
unsupported because the MM32 can reset the ESP during its limited wake window.
