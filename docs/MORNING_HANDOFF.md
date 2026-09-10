# Release handoff — 11 September 2026

## What is ready

The editable source, embedded setup page, research notes and photographs are
organised for release. The v1.0.0 candidate has been compiled, flashed and
validated on the owner's compatible movement; it is not yet published.

Further source changes after the initial review include:

- POSIX syntax validation for the supported ESP8266/newlib subset; explicit
  DST transition rules are required. NTP hostname and schedule validation.
- Per-boot request tokens and Host/Origin checks on the setup API. These are
  cross-site request protections, not a password or authentication system.
- Page-specific keepalive ownership: retries cannot restart the one-minute
  deadline, and an older page closing cannot close a newer page's session.
- Wi-Fi scan results are copied into a bounded response and SDK results freed,
  even if the browser disappears. Cached responses expire after 15 seconds.
- Safer handling of lost save responses, stale browser pages and restart.
- Research capture/extraction helpers refuse accidental output overwrites.

The two-second heartbeat cadence, one-minute page session cap and eight-second
NTP wait interval remain. Direction-labelled stock captures subsequently showed
that valid NTP is followed by `+TIME` within roughly 0.6–1.5 seconds. The source
therefore removes the unconditional five-second grace: it waits about one
second, with a page-active path sending `+TICK` first. A browser close remains
best effort; an undelivered close expires through the 10-second inactivity
timeout. None of these override the MM32.

M.SET+REC is now confirmed to produce repeated MM32 `CLEAN` commands. Stock
waits for the repeat, commits cleared settings, returns `CLEAN_OK`, then
software-restarts. A first replacement test exposed a reboot loop because each
fresh boot treated the next repeat as a new reset. Source now requires a second
`CLEAN` at the observed cadence and, once configuration is blank, acknowledges
later repeats without another erase or restart. This behaviour was validated on
the v1.0.0 candidate.

Subsequent stock tests showed that its ESP accepts and transmits arbitrary
AutoAdjust strings, but the MM32 does not reliably honour them. A known
`22:00` control wake succeeded. The replacement page and server-side validation
now expose only `09:00`, `10:00`, `21:00`, and `22:00`; an older replacement
schedule is normalised to `10:00` when loaded.

## Completed after the review

The source was compiled and flashed with the documented ESP8285 settings. The
owner completed the hardware validation path, including provisioning, NTP and
fallback, UART timing, reset behaviour and a supported scheduled wake. The
paired v1.0.0 application export and clean 1 MiB image are recorded in
`firmware/release-candidate/README.md`.

Browser simulation tests exercise the real page script, but cannot prove ESP
timing or MM32 behaviour.

All 28 page checks passed, as did the offline public-file, PowerShell syntax
and local Markdown file-link checks. All 40 timezone presets also passed the
IANA tzdata 2026.3 verification through 2035. The stricter stock extractor successfully
read the owner's 1 MiB image into a new ignored private directory (7,853 bytes
from 64 pages). Existing candidate binary hashes remain unchanged.

The publication check caught an original raw dump at the repository root.
It was preserved, not deleted; a global `*.bin` ignore now protects accidental
raw-dump inclusion. A final staged-file and privacy review is still required
immediately before GitHub publication.

## Next session

1. Create the initial local Git commit and inspect the staged source set.
2. Generate a release `SHA256SUMS.txt` from the paired v1.0.0 binaries.
3. Create/push the GitHub repository and inspect the draft release before
   publishing it.

The ignored `release-preparation/` directory holds any locally generated
source-only ZIP and its per-file SHA-256 manifest. These snapshots deliberately
exclude original firmware, private captures, build outputs and candidate bins.
GPL-3.0-or-later has been selected; no public release has been created.
