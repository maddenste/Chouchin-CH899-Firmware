# Research tools

- `test_web_ui.cjs` executes the actual setup-page JavaScript with a small
  simulated browser/HTTP environment, using Node.js built-in modules only.
  Run `node .\tools\test_web_ui.cjs` from the repository root. It does not open
  a port, contact a clock or compile firmware.
- `capture_ch899_uart.ps1` records a selected serial port to a binary capture.
  UART traffic can expose Wi-Fi and local-network details; keep output private.
- `CH899_Dual_UART_Monitor/` is a passive NodeMCU sketch that receives clock TX
  and RX independently on D5/D6 and labels both directions over USB. The paired
  `capture_dual_uart_monitor.ps1` records its output with UTC timestamps and
  provides keyboard markers for M.SET/REC tests. See the sketch header before
  wiring; the NodeMCU never powers or transmits to the clock. Build it as
  **NodeMCU 1.0 (ESP-12E Module)** at **160 MHz CPU** using core 3.1.2.
- `extract_ch899_spiffs.ps1` extracts the stock `index.html` from an
  owner-supplied 1 MiB CH-899 ESP image. It requires an explicit input path and
  writes to the ignored `private/` tree by default.

These are analysis helpers, not flashing tools. They should only be used with
hardware and firmware images you are authorised to inspect.

## Offline release preparation

- `check_release.ps1` checks the public file set, generated page, PowerShell
  syntax, local Markdown file links and page regression tests. It does not
  compile firmware, open serial ports or contact the clock.
- `prepare_source_release.ps1` produces a source-only ZIP in ignored
  `release-preparation/`, rereads every archive entry and verifies its SHA-256.
  It excludes release binaries and private material. Run the check first.

```powershell
powershell -NoProfile -File .\tools\check_release.ps1
powershell -NoProfile -File .\tools\prepare_source_release.ps1
```

Neither a passing offline check nor a source ZIP is a hardware release sign-off.
