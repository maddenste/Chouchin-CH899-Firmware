# Third-party notices

This repository's original source code is licensed under
[GPL-3.0-or-later](LICENSE). The project is built against software that retains
its own licence terms. Those terms are not replaced by the project licence.

## ESP8266 Arduino Core 3.1.2

Released firmware is built with **ESP8266 by ESP8266 Community 3.1.2**. The
ESP8266 Arduino core is licensed under the GNU Lesser General Public License,
version 2.1 or later (LGPL-2.1-or-later). Source and licence information are
available from the official project:

- https://github.com/esp8266/Arduino/tree/3.1.2
- https://github.com/esp8266/Arduino/blob/3.1.2/LICENSE

The core bundles additional components under their respective terms, including
Espressif's NONOS SDK (MIT), BearSSL (MIT), and other components documented by
the core project. A binary release identifies this exact core version and
provides this notice with the source and flashing instructions.

## ESP8266 mDNS

The `ESP8266mDNS` / LEAmDNS implementation used for
`wifi-clock-<chip-id>.local` is included with the ESP8266 Arduino core and is
MIT-licensed. Its source and copyright notices are available in the same
versioned core repository:

- https://github.com/esp8266/Arduino/tree/3.1.2/libraries/ESP8266mDNS

## No vendor firmware redistribution

The original CH-899 / CHOUCHIN ESP firmware, raw flash dumps, extracted stock
web assets, Wi-Fi credentials and UART captures are not included. Compatibility
was developed through independent observation of owner-supplied hardware.
