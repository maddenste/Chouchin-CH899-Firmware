# Clock setup

Wake the clock using M.SET and connect your phone to `WiFi-Clock Setup-<id>`.
If the setup page does not appear automatically, open `http://192.168.4.1/`.
The phone may warn that this network has no internet: that is expected.
Once the clock is connected to your LAN, `http://wifi-clock-<chip-id>.local/`
provides a unique friendlier address while the ESP remains awake. The exact
address is shown on the setup page after it connects. mDNS support depends on
the phone or computer and local network; use the clock's IP address if it does
not resolve.

Choose a 2.4 GHz Wi-Fi network from the automatically scanned list, or use the
last option to enter its name manually. **Re-scan networks** refreshes the list.
Enter the network password. With an existing saved SSID, a blank password keeps
its saved password; with a different SSID, blank means an open network.

Select one of the movement's confirmed daily update times: **09:00, 10:00,
21:00, or 22:00**. Stock testing showed that its ESP could transmit other
values, but the MM32 movement did not reliably honour them, so this page does
not offer unsupported choices.

Leave **NTP server** blank to use `pool.ntp.org`, or enter a hostname/IP address,
not a web URL. The clock tries a custom server first, then the default after
eight seconds without a valid response. Wi-Fi connectivity and NTP access are
needed to acquire fresh time; the setup page itself has no external web assets.

Choose a **Timezone**, then select **Automatic** to apply its verified seasonal
clock-change rule or **Disabled** to keep that location's standard time all
year. **Custom rule** enables the POSIX field for regions or future rule changes
not covered by the presets. The presets were checked against IANA tzdata 2026.3
from 10 September 2026 through 2035. Civil-time laws can still change after a
firmware release. The ESP applies the selected rule when sending updated time;
movement behaviour at a DST boundary before its next sync is not established.

**Save & update clock** validates input, saves the settings and restarts the ESP.
It then connects to Wi-Fi, acquires NTP time and sends time to the movement.
The page may become unreachable after the update. If the response is lost,
reconnect and inspect settings rather than assuming the save failed.

While open, the page requests a keepalive every two seconds for up to one
minute. Closing the page normally stops it; if the phone cannot deliver the
close message, it expires after ten seconds of inactivity. The movement can
still end the wake window. This is a setup page, not an always-online clock
dashboard.

**Factory reset** asks for confirmation, clears this firmware's saved settings
and restarts the ESP. It does not reinstall vendor firmware or erase the MM32.
The AP is open: see [security boundaries](SECURITY.md).

The bottom of the setup page displays the installed firmware version. The same
value is available to local tools at `/api/v1/status` as `firmwareVersion`.
