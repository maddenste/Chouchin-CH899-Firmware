# CH-899 · Wi-Fi-only upgrade

**For the older MM32 revision of the Chouchin CH899 clock.** This version improves Wi-Fi setup and internet timekeeping while leaving the factory hand-movement firmware untouched.

![Chouchin-CH899 WiFi Clock v1.0 — ESP8285 Wi-Fi-only upgrade](docs/assets/wifi-clock-v1.0-banner.png)

## Check your clock

Look for the circuit board shown below. This project updates only its Wi-Fi module. [See the board close-ups →](docs/REVERSE_ENGINEERING.md#hardware-identified)

<img src="docs/images/06-full-pcb-component-side.jpg" alt="The older MM32 clock board supported by the Wi-Fi-only upgrade" width="450">

## What you gain

- **Phone-friendly Wi-Fi setup** — choose a nearby network or enter a hidden network yourself.
- **Timezone and summer/winter time settings** — with a choice of daily updates at 09:00, 10:00, 21:00 or 22:00.
- **A choice of internet time server** — with a backup server used automatically if yours fails.

Hand movement remains as supplied by the manufacturer. For movement styles and seconds-hand parking, see the v1.5 option below.

## Install

**[Download v1.0.1](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.1)** · **[Follow the installation guide →](docs/FLASHING.md)**

You will need to open the clock and connect a serial programmer. The guide covers the connections, keeping a personal backup and installing the prepared firmware; no compiling is needed.

The clock switches Wi-Fi off after updating to save battery power. **To change settings later, press M.SET, then [open its settings page](docs/USER_GUIDE.md#open-the-settings-page-again).**

[Settings and everyday use](docs/USER_GUIDE.md) · [Printable manual](docs/WiFi-Clock-User-Manual-v1.0.1.pdf) · [Help or report a bug](https://github.com/maddenste/Chouchin-CH899-Firmware/issues)

## Want more control, or have a different board?

- **[WiFi Clock v1.5 for this same MM32 board](https://github.com/maddenste/Chouchin-CH899-MM32-ESP8285)** — adds movement styles and seconds-hand parking. It also replaces the factory movement firmware, which cannot be recovered from the chip afterwards; read its installation warning first.
- **[WiFi Clock v2.0 for the newer HC32 revision](https://github.com/maddenste/Chouchin-899-latest-revision)** — for different electronics. [Check its board photos](https://github.com/maddenste/Chouchin-899-latest-revision/blob/main/docs/HARDWARE.md#board-photographs).

Use the files and installation guide for the project you choose; the firmware is not interchangeable.

---

Unofficial project by Steve Madden · 2026. Not affiliated with the clock manufacturer.

[How we investigated the clock](docs/JOURNEY.md) · [Source and building](arduino/CH899_Clock/README.md) · [Testing](docs/TESTING.md) · [Changelog](CHANGELOG.md) · [GPL-3.0-or-later](LICENSE) · [Third-party notices](THIRD_PARTY_NOTICES.md)
