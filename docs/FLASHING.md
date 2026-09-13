# Flashing a CH-899 clock

> **Risk warning:** These instructions are for a compatible CH-899 movement
> that you own. Make and verify a complete 1 MiB backup before changing any
> clock. Never apply 5 V to the ESP's 3.3 V signals.

## Required equipment

- Open CH-899 movement with access to the ESP programming pads.
- 3.3 V USB-to-UART adapter or a NodeMCU used as a USB-UART bridge.
- A stable power source for the clock, common ground and short jumper wires.
- Access to ESP GPIO0 and the MM32 reset pad, plus a verified way to reset or
  power-cycle the clock's ESP.
- `esptool` **4.5.1** for Windows, the version used in these examples.

The commands below use the v4 spelling (`flash_id`, `read_flash`, `no_reset`).
Newer versions may use hyphens. Change `COM4` and the file paths to match your
PC, and close Arduino Serial Monitor or any other program using that port.

Download the matching application image and `SHA256SUMS.txt` from the relevant
[GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases).
The filename below uses v1.0.1. Verify the application's SHA-256 before
flashing:

```powershell
certutil -hashfile 'C:\path\to\CH899-Clock-v1.0.1-esp8285-application.bin' SHA256
```

It must match the value in the downloaded `SHA256SUMS.txt`.

![ESP programming pads](images/07-esp-programming-header.jpg)

## UART wiring

With an ordinary USB-UART adapter, TX and RX are named from the adapter's
perspective:

| USB-UART side | Clock ESP side |
| --- | --- |
| GND | GND |
| TX output | RX input |
| RX input | TX output |

**A NodeMCU's printed RX/TX labels have a different perspective.** They name
the NodeMCU's own ESP pins. For the standard NodeMCU DevKit wiring, use:

| NodeMCU printed pin | USB bridge function | Clock ESP pad |
| --- | --- | --- |
| GND | Common ground | G |
| RX / GPIO3 | USB bridge TX output | RX |
| TX / GPIO1 | USB bridge RX input | TX |
| RST | Holds the NodeMCU's own ESP inactive | NodeMCU GND only |

Keep the NodeMCU's **RST connected to its GND throughout** so its own ESP
cannot transmit onto the shared connection. This reset is separate from the
MM32 reset on the clock. The mapping follows the
[official NodeMCU schematic](https://github.com/nodemcu/nodemcu-devkit-v1.0/blob/master/NODEMCU_DEVKIT_V1.0.PDF)
(USB-UART and IO connector sheets). Verify a clone's wiring before using these
labels; do not assume a generic adapter and a NodeMCU need the same labelled
connections.

In the ESP-header photograph above, the five holes read **IO0, RX, TX, V, G**
from left to right (`IO0` resembles `J00` in the silkscreen). Check continuity
with power removed before soldering. The separate `RESET` pad in the
[MM32 header photograph](images/08-mm32-swd-header.jpg) belongs to the MM32,
not the ESP.

Power must be supplied as well as serial data. If supplying external regulated
3.3 V to the verified ESP `V` rail, remove the clock batteries and do not join
two power supplies. Verify `V` to `G` before connecting; `VIN`, USB 5 V and an
unverified battery pad are not substitutes for the ESP's 3.3 V supply. A
USB-UART adapter's small 3.3 V output may not have enough current for the ESP.
The exact MM32-to-ESP power/enable circuitry has not been traced, so confirm
that the ESP remains powered and out of reset while the MM32 is held reset.

The MM32 shares this UART. Hold the **MM32 reset pad to GND** while using the
bootloader so its traffic cannot corrupt flashing. This was necessary for
reliable writes on the tested clock.

![Historical harness during troubleshooting](images/10-esp-flashing-harness.jpg)

This photograph records an early troubleshooting setup, before the successful
RX/TX correction. Use the tables above for wiring, not the wire colours or
positions in this historical photograph.

## Enter the ESP bootloader

1. With power removed, connect the verified power, ground and serial lines.
   Hold the NodeMCU's own RST low if using it as a bridge.
2. Hold the clock's MM32 `RESET` pad to GND and the ESP `IO0` pad to GND.
3. Apply power, or reset the **clock ESP** while IO0 is low. GPIO0 is sampled
   at reset; grounding it after a normal boot is not sufficient.
4. Keep the MM32 reset asserted throughout the backup or write. The clock ESP
   itself must be released from reset to answer `esptool`.

The `--before no_reset` option deliberately performs no hardware reset. It
does not place the chip into the bootloader or wake an ESP held inactive by
the movement. If the ESP does not answer, check its supply, reset/enable state
and the serial directions before retrying. Do not force an unidentified
reset/enable line high against the MM32's output. The boot-mode requirement is
documented in [Espressif's bootloader guide](https://docs.espressif.com/projects/esptool/en/latest/esp8266/advanced-topics/boot-mode-selection.html).

## Confirm the ESP and make a backup

After the bootloader-entry sequence:

```powershell
& 'E:\esptool.exe' --chip esp8266 --port COM4 --baud 57600 --before no_reset --after no_reset flash_id
```

Expected hardware is ESP8285N08 with 1 MiB flash. If a NodeMCU's own ESP8266 or
a different flash size is reported, stop and check which chip you are talking
to. Read the whole flash and retain it privately. Use new filenames so that
an earlier recovery copy is not overwritten:

```powershell
if (Test-Path 'E:\my-ch899-original-1MB.bin') { throw 'Backup already exists; choose a new filename.' }
& 'E:\esptool.exe' --chip esp8266 --port COM4 --baud 57600 --before no_reset --after no_reset read_flash 0x0 0x100000 'E:\my-ch899-original-1MB.bin'
certutil -hashfile 'E:\my-ch899-original-1MB.bin' SHA256
```

A hash records the file's identity; by itself it does not prove a good read.
Before the first write, make a second full read to a different filename and
compare the two hashes while remaining in the bootloader. Both files must be
1,048,576 bytes and match. Keep a copy away from the working folder.

## Write the replacement application

Use a verified binary exported from the matching source revision:

```powershell
& 'E:\esptool.exe' --chip esp8266 --port COM4 --baud 57600 --before no_reset --after no_reset write_flash --flash_mode dout --flash_freq 40m --flash_size 1MB 0x0 'C:\path\to\CH899-Clock-v1.0.1-esp8285-application.bin'
```

The application SHA-256 must match the selected release manifest. Do not assume
that editing the sketch updates a downloaded `.bin`; export, identify and test
any changed image before distributing it.

After `Hash of data verified`, `Staying in bootloader` is expected because
`--after no_reset` was requested. To run normally, power down the clock and
disconnect the programmer's driven serial/power lines, release ESP IO0 and
MM32 reset, then restore the clock's normal power. A fresh reset or power-up
with IO0 released is required. The movement may need M.SET to start a new wake
window. On a first flash or after Factory reset, look for
`wifi-clock-setup-...`; its availability still depends on the MM32-controlled
wake window. Once valid Wi-Fi settings are saved, normal wakes are
station-only and do not broadcast that AP.

## Restore your original firmware

Re-enter the bootloader with the same MM32-reset precautions, then write your
own verified complete backup at `0x0`, keeping its original flash header:

```powershell
& 'E:\esptool.exe' --chip esp8266 --port COM4 --baud 57600 --before no_reset --after no_reset write_flash --flash_mode keep --flash_freq keep --flash_size keep 0x0 'E:\my-ch899-original-1MB.bin'
```

Wait for verification, then use the normal-power sequence above. This restores
the ESP image and saved ESP settings from the backup; it does not restore or
change anything stored separately by the MM32.

## Clean installation

Only do this after retaining a personal original backup. `erase_flash` wipes
the complete ESP, including saved Wi-Fi settings and any vendor recovery image:

```powershell
& 'E:\esptool.exe' --chip esp8266 --port COM4 --baud 57600 --before no_reset --after no_reset erase_flash
```

Then write the replacement application using the earlier `write_flash` command.
The erase leaves settings and unused flash blank; writing the application at
`0x0` creates the clean replacement state. On first normal boot it starts the
setup AP. A separately distributed full-flash image is not required.
