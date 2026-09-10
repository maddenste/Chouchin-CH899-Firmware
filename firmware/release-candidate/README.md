# Local release candidates

This directory holds locally tested binary candidates. The `.bin` files are
ignored by Git and must not be treated as a public release until the testing
checklist is complete.

The 9 September artifacts below pre-date the 10 September source-review fixes
and are retained only as previous candidates. The dated v1.0.0 application
export and 1 MiB image were captured on 11 September after flashing the current
candidate, before saving any Wi-Fi settings. They are the paired recovery and
release-candidate artifacts pending final hardware validation and source-tagged
release packaging.

Retained local artifacts:

| File | Purpose | SHA-256 |
| --- | --- | --- |
| `CH899-Clock-esp8285-application.bin` | Application image written at `0x0` after preserving or erasing the target as appropriate. | `52DAFB24F5BA4DD92A7E97B8D5937E831FCFAC9AF37F9E8012115B936B2469DD` |
| `CH899-Clock-factory-blank-1MB.bin` | Full 1 MiB clean-flash candidate with no saved configuration. | `4695FB69211E1633A9B9D591290AD40950BB98F018AF3FC4CEA36387C3244EF8` |
| `CH899-Clock-v1.0.0-esp8285-application.bin` | Current v1.0.0 application export for writing at `0x0` with the documented 1 MiB flash arguments. | `A8E3B018228CBB56E9D43F306BB7825C42C9BA665AC2651DAF8E725524FC4788` |
| `CH899-Clock-factory-blank-1MB-20260910.bin` | Current full 1 MiB clean read-back after candidate flashing; preserve unchanged for final test and release comparison. | `DA7CEC4D1BCC82D075490A93A90B021474A80112AD2CFF16690C40DD456E6454` |

The v1.0.0 application bytes match the factory image from offset `0x4` onward.
The image-header flash-parameter byte differs (`0x90` in the Arduino export,
`0x20` in the read-back) because `esptool` was instructed to write the correct
1 MiB, DOUT, 40 MHz flash parameters. This is expected and is not a payload
difference.

Before publishing, confirm this exact candidate passes the hardware checklist,
rebuild from the tagged source, regenerate this table, and attach the verified
binaries and a `SHA256SUMS.txt` file to a GitHub Release.
