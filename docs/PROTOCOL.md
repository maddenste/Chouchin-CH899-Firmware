# Observed UART protocol

Application traffic is 115200 baud. The ESP ROM boot banner uses 74880 baud;
garbled ROM text at the application baud is not a separate MM32 command.

| Direction | Record | Evidence and limits |
|---|---|---|
| MM32 → ESP | `USER\n` | Repeated in captured reset and M.SET sessions, about every two seconds. |
| ESP → MM32 | `USER_OK\r\n` | Immediate stock reply, also present before NTP becomes ready. |
| ESP → MM32 | `+TIME:Tue Sep  8 20:43:57 2026 +0100 10:00\r\n` | Local civil time with numeric UTC offset, followed by the configured daily update time. Stock records repeat at one-second cadence. |
| ESP → MM32 | `+TICK\r\n` | Stock sends this every two seconds while its setup page is open. Keepalive effect is observed primarily in M.SET sessions; exact MM32 timer is not established. |
| MM32 → ESP | `CLEAN\n` | M.SET+REC held for roughly 2–3 seconds resets/wakes the ESP and repeats this about every two seconds until acknowledged. |
| ESP → MM32 | `CLEAN_OK\r\n` | Stock sends this after committing cleared settings, then performs a software restart. |

The replacement accepts the MM32's LF-terminated input and uses the stock
CRLF ending for every reply and outgoing protocol record. The date example
above is a historical capture, **not a command to paste as today's time**.
Preserve the space-padded day (`Sep  8`) and offset. The trailing `10:00` is not
a UTC offset.

The ESP alone cannot reliably distinguish daily wake from M.SET using these
captures: both can contain `USER`. The MM32 owns the wake window; a low reset
measurement suggests external control but is not a complete traced schematic.

Stock captures consistently contain a first `CLEAN`, a repeat about 2.06 seconds
later, then the settings commit and `CLEAN_OK` roughly 107–108 ms after that
repeat. The software reboot begins roughly 112–120 ms after acknowledgement.
This supports a two-command confirmation rather than treating either command
independently.

The replacement arms on the first valid `CLEAN` and confirms a reset only when
the repeat arrives 1–4 seconds later. A later command starts a fresh window. If
replacement settings are already blank, `CLEAN` receives `CLEAN_OK` without a
flash write or another software restart. This avoids the repeated reboot loop
seen with the earlier one-command implementation.

The source does not reimplement stock SDK boot chatter, Wi-Fi diagnostics or
`scandone` messages. There is no evidence those are required MM32 commands.
Stock tests showed that arbitrary `HH:MM` strings can reach this record, but the
MM32 did not reliably schedule them. A known `22:00` control wake succeeded;
the replacement therefore exposes only the stock page's four supported values:
`09:00`, `10:00`, `21:00`, and `22:00`.

See [the investigation](REVERSE_ENGINEERING.md) and [journey](JOURNEY.md).
