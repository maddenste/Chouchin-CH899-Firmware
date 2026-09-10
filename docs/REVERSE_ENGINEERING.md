# Hardware and reverse-engineering findings

This document separates **observed behaviour** from **working hypotheses**.
The work concerns an owner-supplied CH-899 / CHOUCHIN movement and is not a
claim that every visually similar clock is identical.

For the chronological investigation, including the failed approaches and
design decisions, see the [reverse-engineering journey](JOURNEY.md).

![Full PCB](images/06-full-pcb-component-side.jpg)

## Hardware identified

| Item | Evidence | Role |
| --- | --- | --- |
| CH-899 / CHOUCHIN movement | Housing marking | Battery-powered clock movement. |
| ESP-01F / ESP8285N08 | ESP module marking and `esptool` identification | Wi-Fi, setup web page and NTP client. Embedded 1 MiB flash. |
| MindMotion MM32SPIN-family MCU | Package marking and board layout | Main movement/clock controller. |
| PCF8563 | U7 package marking | RTC-family device present on the board; its exact role on this design is not yet mapped. |
| Two large coils and gear train | Physical inspection | Electromagnetic movement actuator. |

![ESP-01F module and UART header](images/03-esp01f-module-and-uart-header.jpg)

## Confirmed ESP facts

- The ESP is an **ESP8285N08**, with 1 MiB embedded flash and a 26 MHz crystal.
- The captured stock image used DOUT / 40 MHz flash settings.
- ESP ROM boot text is emitted at 74880 baud. The running application UART is
  115200 baud.
- The stock Wi-Fi application was read non-destructively from the ESP and a
  SHA-256 backup retained privately.
- The stock image contains a bootloader at `0x000000` and starts its application
  at flash offset `0x001000`; its stock SPIFFS volume occupies
  `0x0E4000` through `0x0EFFFF` (48 KiB). These offsets were checked in the
  retained original dump. The replacement Arduino image is flashed at `0x0`.

## Confirmed MM32 ↔ ESP protocol

| Direction | Record | Interpretation |
| --- | --- | --- |
| MM32 → ESP | `USER\n` | Repeated wake-session handshake request. |
| ESP → MM32 | `USER_OK\r\n` | Immediate handshake reply. |
| MM32 → ESP | `CLEAN\n` | Repeated after M.SET+REC is held for roughly 2–3 seconds. |
| ESP → MM32 | `CLEAN_OK\r\n` | Sent after stock settings are cleared and before its software restart. |
| ESP → MM32 | `+TICK\r\n` | Seen every two seconds during a stock manual setup-page session; observations suggest it extends that wake window. |
| ESP → MM32 | `+TIME:Tue Sep  8 20:43:57 2026 +0100 10:00\r\n` | Local time and numeric UTC offset; the trailing `HH:MM` is the webpage's AutoAdjust value. A stock `22:00` scheduled wake was confirmed. |

The observed stock ESP repeatedly transmits `+TIME` after an NTP success. The
MM32 may then end the ESP wake window; timing varied with the session. This
replacement follows that observed
behaviour rather than trying to force the ESP to sleep itself.

## Wake and manual setup behaviour

M.SET causes an ESP boot/reset sequence. The ESP ROM banner appears, then the
MM32 sends repeated `USER` records. The owner measured approximately **0.7 V**
on the ESP reset line while the unit was inactive. The captured `rst cause:2`
is consistent with an external reset, supporting MM32 control of the wake/reset
path. The circuit has not been traced, so neither the measurement nor the boot
log alone establishes whether reset, enable or supply switching is the complete
mechanism.

The replacement's setup page can send `+TICK` while open, but `+TIME` still
means the MM32 may end a normal update session immediately. Page availability
therefore cannot be guaranteed by the ESP firmware alone.

## Further findings and remaining tests

- The trailing `HH:MM` value controls the MM32's daily wake schedule. The stock
  ESP accepted and transmitted arbitrary values such as `22:05`, but the MM32
  did not reliably wake for them. A `22:00` control wake succeeded. Replacement
  firmware therefore restricts selection to the stock page's `09:00`, `10:00`,
  `21:00`, and `22:00` choices.
- M.SET+REC held for roughly 2–3 seconds resets/wakes the ESP and makes the MM32
  repeat `CLEAN` about every two seconds. Stock commits cleared parameters,
  returns `CLEAN_OK`, disconnects and software-restarts only after the repeat.
  Replacement source now uses the same two-command confirmation and treats an
  already blank configuration as acknowledged without another erase/restart.
  This behaviour was validated on the v1.0.0 candidate.
- The MM32 SWD pad group is labelled `GND VDD DIO CLK RESET`. It is a promising
  read-only debug route, but the MM32 has not been read or modified.

![MM32 controller and crystal](images/05-mm32spin-controller-and-crystal.jpg)

## Why this build has no OTA

The stock ESP image contains a legacy dual-slot HTTP OTA client. Request
templates refer to `user1.bin` and `user2.bin`; the expected second slot at
`0x081000` was erased in the retained dump. Its presence is not proof of a usable
end-user update function or automatic rollback. It is not used by this project
because:

1. The stock web UI exposes no confirmed public OTA trigger.
2. The MM32 can reset the ESP before an update transaction completes.
3. The legacy client uses HTTP and has no confirmed signature-verification path.
4. No complete update, interrupted-update recovery and rollback procedure has
   been validated on this clock.

A 1 MiB ESP can support OTA with suitably sized firmware; capacity alone is not
the reason it is excluded here. `FLASH_MAP_NO_FS` removes the filesystem, not
the technical possibility of OTA. The replacement intentionally includes no
OTA update endpoint or handler. See the
[ESP8266 Arduino OTA documentation](https://arduino-esp8266.readthedocs.io/en/3.1.2/ota_updates/readme.html)
for the distinction between available flash space and an implemented updater.

The replacement therefore uses a single application image and direct serial
flashing. This is deliberate: predictable recovery is more valuable than an
unverified remote-update feature.
