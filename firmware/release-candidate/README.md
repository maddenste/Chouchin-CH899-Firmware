# Release artifact record

This directory records the v1.0.0 release artifacts. The firmware `.bin` files
are release downloads, not repository files; get them from the
[v1.0.0 GitHub Release](https://github.com/maddenste/Chouchin-CH899-Firmware/releases/tag/v1.0.0).

The dated v1.0.0 application export and 1 MiB image were captured on
11 September after flashing the reviewed source and before saving Wi-Fi
settings. They are the paired public release artifacts validated on the
compatible movement.

Retained local artifacts:

| File | Purpose | SHA-256 |
| --- | --- | --- |
| `CH899-Clock-v1.0.0-esp8285-application.bin` | Current v1.0.0 application export for writing at `0x0` with the documented 1 MiB flash arguments. | `A8E3B018228CBB56E9D43F306BB7825C42C9BA665AC2651DAF8E725524FC4788` |
| `CH899-Clock-factory-blank-1MB-20260910.bin` | Current full 1 MiB clean read-back after v1.0.0 flashing, for recovery or factory preparation. | `DA7CEC4D1BCC82D075490A93A90B021474A80112AD2CFF16690C40DD456E6454` |

The v1.0.0 application bytes match the factory image from offset `0x4` onward.
The image-header flash-parameter byte differs (`0x90` in the Arduino export,
`0x20` in the read-back) because `esptool` was instructed to write the correct
1 MiB, DOUT, 40 MHz flash parameters. This is expected and is not a payload
difference.

`SHA256SUMS.txt` in this directory and on the GitHub Release is the verified
checksum manifest for the two public artifacts. Recheck the hashes after every
download and before flashing.
