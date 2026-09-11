# Reverse-engineering journey

This is the practical story of developing a replacement ESP firmware for one
owner-supplied CH-899 / CHOUCHIN Wi-Fi clock movement. It is not a claim that
every visually similar movement has the same board, firmware or behaviour.

The companion [hardware and findings document](REVERSE_ENGINEERING.md) is the
concise technical reference. This page records how those findings were reached,
including dead ends and design decisions. It deliberately excludes the original
vendor image, raw UART captures, Wi-Fi credentials, local IP addresses and any
other private data.

![Opened movement with coils and gear train](images/01-open-movement-and-coils.jpg)

## 1. Starting point: identify the movement

The housing was marked **CH-899 CHOUCHIN**. Opening it revealed a conventional
battery-powered gear train with two large actuator coils, plus a two-sided PCB.
The board contained an ESP-01F Wi-Fi module, a MindMotion MM32SPIN-family
microcontroller, a PCF8563 RTC-family device, and labelled programming/test
pads.

The practical split soon became clear: the MM32 runs the clock movement and
controls the ESP's wake window, while the ESP provides Wi-Fi, NTP and the setup
page. The exact power/enable/reset circuit has not been traced. The work
therefore focused on replacing only the ESP firmware.

![Component side of the PCB](images/06-full-pcb-component-side.jpg)

## 2. Make a recoverable backup before changing anything

The ESP module identified itself through the ROM bootloader as an **ESP8285N08**
with 1 MiB of embedded flash and a 26 MHz crystal. A complete 1 MiB flash dump
was made before any writes, and its SHA-256 hash was recorded privately.

This was an important early rule for the project: every clock should have a
personal full-flash backup before it is modified. The original vendor image is
not redistributed here.

## 3. Find a dependable programming connection

The board exposes an ESP UART/programming header labelled `RX`, `TX`, `V` and
`G`, plus nearby power and status pads.

At first, `esptool` produced invalid packet headers or no serial data. A
NodeMCU USB-UART loopback test proved the PC, COM port and USB bridge were
working. Swapping the two data connections then allowed the clock's ESP8285 to
answer. An important documentation correction followed: a NodeMCU's RX/TX
labels refer to its own ESP, not its USB bridge. Standard NodeMCU wiring uses
printed RX to clock RX and printed TX to clock TX, with the NodeMCU's own ESP
held in reset. A dedicated USB-UART adapter instead uses adapter TX to clock
RX and adapter RX to clock TX. The [flashing guide](FLASHING.md) explains both
perspectives and the need to verify clone wiring.

The second issue was the MM32. It shares the ESP UART and can disturb flashing.
Holding the MM32 reset line low while putting the ESP into bootloader mode made
flashing repeatable. `57600` baud proved stable on this setup, whereas a faster
compressed write was not reliable.

![ESP programming header](images/07-esp-programming-header.jpg)

This led to the safe flashing method documented in
[Flashing a clock](FLASHING.md): use 3.3 V logic only, hold ESP `IO0` low for
bootloader entry, reset or power-cycle the ESP with that strap in place, hold
MM32 reset low during the transfer, and verify the write. A normal restart
with IO0 released is then needed to run the application.

## 4. Inspect the stock ESP image without claiming its source code

The stock dump contained a bootloader at `0x000000`, an ESP application
beginning at `0x001000`, and a SPIFFS web-page region at `0x0E4000`. These are
stock-image offsets, not the replacement's flashing instructions: the full
Arduino application image is written at `0x000000`. Extracting the stock page
was useful for understanding the original setup flow, but it did not recover
the vendor's application source code.

The binary also contained signs of a legacy dual-slot HTTP OTA implementation.
There was no confirmed end-user OTA trigger in the stock page, and no confirmed
safe way to start such an update on an unmodified clock. That finding later
became a deliberate product decision: this replacement does **not** include
OTA updates.

## 5. Observe the actual UART conversation

Listening independently to both UART directions was much more useful than
trying to infer behaviour from strings in the binary.

On an MM32-initiated wake, including an M.SET wake, the MM32 repeatedly sent:

```text
USER
```

The stock ESP replied:

```text
USER_OK
```

Once it had NTP time, the stock ESP sent records in this form:

```text
+TIME:Tue Sep  8 20:43:57 2026 +0100 10:00
```

The first date/time is local time, the middle field is the numeric UTC offset,
and the final `HH:MM` strongly appears to be the MM32's next daily wake
schedule. The MM32 typically ended the ESP session after receiving `+TIME`.

This provided an explanation for an initially surprising result: a functioning
page could disappear after a successful time update when the movement ended
the ESP wake window. That observation should not be used to diagnose every
future loss of connectivity as the same cause.

## 6. Learn what keeps a manual setup window open

The stock firmware sent another record while a manual setup-page session was
active:

```text
+TICK
```

It appears to extend the MM32-controlled wake window. It was not a permanent
keep-awake mechanism: the MM32 still owns reset and can end the session.

The replacement page therefore starts `+TICK` only after its JavaScript is
actually open in a browser, sends it every two seconds, stops it when the page
closes where the browser permits, and imposes a one-minute maximum. Captive
portal probes and ordinary status polling do not start this keepalive session.

## 7. First replacement experiments and a change of direction

The first experiment changed only the stock SPIFFS web assets. That proved a
better page could be shown, but it still left the clock dependent on an opaque
vendor ESP application and its storage layout.

The project then moved to a fresh Arduino ESP8266/ESP8285 application with the
web page embedded in the firmware. This makes the page and behaviour editable
source code, avoids a separate filesystem flash step, and leaves a simpler
single-image recovery path on the 1 MiB device.

## 8. Design the replacement around the MM32 rather than fighting it

The new firmware preserves the confirmed parts of the conversation:

- `USER` receives `USER_OK`.
- A browser page may use `+TICK` during an M.SET configuration session.
- NTP success produces local time, the applicable numeric offset and the saved
  daily schedule in a `+TIME` record.

It does not request deep sleep to end a wake window. The owner measured about
0.7 V on the ESP reset line while the unit was inactive, and ROM boot output
appeared when M.SET woke it. This supports externally controlled wake/reset,
but a single voltage reading does not establish the full circuit or distinguish
reset, enable control and a switched supply. The MM32 remains in charge of
the observed wake window.

The rear-board indicator is a **red/blue** LED. An earlier visual observation
described its brief blue pulses as white; that colour identification was
corrected before release preparation. The electrical meaning of either colour
has not been traced, so LED colour is not used as protocol evidence.

The page provides 2.4 GHz Wi-Fi scan and hidden-SSID entry, an NTP host, a
POSIX time-zone rule, the four MM32-supported daily update times, a fallback to
`pool.ntp.org`, and a Factory reset action for the replacement firmware's saved
configuration.

A later direction-labelled dual-UART capture resolved the physical reset path.
Holding M.SET+REC for roughly 2–3 seconds makes the MM32 reset/wake the ESP and
repeat `CLEAN` about every two seconds. Stock commits its cleared parameters,
returns `CLEAN_OK`, disconnects and starts a software reboot. Repeating the
operation on an already unconfigured ESP showed that it is idempotent.

The first replacement implementation erased on every first `CLEAN`. A hardware
capture then showed ten acknowledgements and ten software restarts in about 19
seconds: after every boot the MM32's next retry looked new. The revised source
uses the stock-like two-command cadence as confirmation. Once configuration is
blank, further `CLEAN` commands are acknowledged without another write or
restart. This correction was subsequently validated in v1.0.0.

## 9. Deliberate choices for a releasable build

Several features were considered and intentionally excluded:

| Decision | Reason |
| --- | --- |
| No OTA updates | A reliably uninterrupted ESP wake window has not been established, and no safe stock OTA trigger was confirmed. Flash size alone does not rule OTA out; this project omits it because its update/recovery behaviour has not been validated on this hardware. |
| No MM32 rewrite | The movement works already; replacing its firmware would add risk without being required for Wi-Fi/NTP improvements. |
| Embedded web page | One editable Arduino project and one firmware image are easier to reproduce and recover than a separate filesystem image. |
| Direct serial recovery route | It remains available even when Wi-Fi setup fails or the movement has ended a wake session. |

## 10. Open questions, not claims

The following are useful leads, but are deliberately not presented as settled
facts:

- **Daily wake choices:** stock firmware accepted and transmitted custom values
  such as `21:43`, `21:59`, and `22:05`, proving its visible dropdown was not the
  storage restriction. Those values were not reliably honoured by the MM32,
  while a known `22:00` control wake succeeded. The replacement now offers only
  the stock page's four supported times.
- **MM32 SWD port:** pads marked `GND VDD DIO CLK RESET` look like a promising
  read-only SWD route. They have not been used; bypassing read protection or
  using a method that erases the movement firmware is explicitly out of scope.

## 11. Release discipline

The source and documentation were prepared for the public v1.0.0 release. The
source was compiled, flashed and validated on the compatible clock, including a
supported scheduled wake and Factory reset. The GitHub release contains the
application image, full 1 MiB factory image and checksum manifest.

The retained 9 September binaries predate the subsequent source review. The
v1.0.0 application export and paired clean 1 MiB recovery image supersede them
for release preparation; their checksums are published with the
[v1.0.0 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.0).

For the current evidence and test state, see
[Hardware and reverse-engineering findings](REVERSE_ENGINEERING.md) and
[Testing status](TESTING.md).
