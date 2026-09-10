/*
  Copyright (C) 2026 Steve Madden
  SPDX-License-Identifier: GPL-3.0-or-later

  Passive two-channel CH-899 UART monitor for a NodeMCU ESP8266.

  Clock TX -> 4.7k series resistor -> NodeMCU D5 (GPIO14)
  Clock RX -> 4.7k series resistor -> NodeMCU D6 (GPIO12)
  Clock GND -----------------------> NodeMCU GND

  Do not connect NodeMCU TX/RX, 3V3 or VIN to the clock. Power the clock
  normally and the NodeMCU from USB. Both monitored signals must be 3.3 V.

  The two software UARTs listen at 115200 8-N-1. Labelled text is emitted to
  the computer through the NodeMCU USB serial bridge at 230400 baud.

  Arduino board: NodeMCU 1.0 (ESP-12E Module), CPU Frequency: 160 MHz.
  The higher CPU speed gives two simultaneous 115200 software receivers more
  timing margin. ESP8266 core 3.1.2 was used for the checked build.
*/

#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

namespace {
constexpr uint32_t CLOCK_BAUD = 115200;
constexpr uint32_t USB_BAUD = 230400;
constexpr int ESP_TX_INPUT = D5;
constexpr int MM32_TX_INPUT = D6;
constexpr uint32_t IDLE_FLUSH_MS = 25;
constexpr size_t LINE_CAPACITY = 192;

EspSoftwareSerial::UART espToMm32;
EspSoftwareSerial::UART mm32ToEsp;

struct CapturedLine {
  const char *label;
  uint8_t data[LINE_CAPACITY];
  size_t length = 0;
  uint32_t firstMs = 0;
  uint32_t lastMs = 0;
};

CapturedLine fromEsp{"ESP>MM32"};
CapturedLine fromMm32{"MM32>ESP"};

void printEscaped(uint8_t value) {
  if (value >= 0x20 && value <= 0x7e && value != '\\') {
    Serial.write(value);
  } else if (value == '\\') {
    Serial.print(F("\\\\"));
  } else if (value == '\r') {
    Serial.print(F("\\r"));
  } else {
    static const char hex[] = "0123456789ABCDEF";
    Serial.print(F("\\x"));
    Serial.write(hex[value >> 4]);
    Serial.write(hex[value & 0x0f]);
  }
}

void emit(CapturedLine &line, const __FlashStringHelper *suffix = nullptr) {
  if (!line.length) return;
  Serial.printf("[%010lu..%010lu][%s] ",
                static_cast<unsigned long>(line.firstMs),
                static_cast<unsigned long>(line.lastMs), line.label);
  for (size_t i = 0; i < line.length; ++i) printEscaped(line.data[i]);
  if (suffix) Serial.print(suffix);
  Serial.println();
  line.length = 0;
}

void captureByte(CapturedLine &line, uint8_t value, uint32_t now) {
  if (!line.length) line.firstMs = now;
  line.lastMs = now;
  if (value == '\n') {
    emit(line);
    return;
  }
  if (line.length == LINE_CAPACITY) {
    emit(line, F(" [continued]"));
    line.firstMs = now;
    line.lastMs = now;
  }
  line.data[line.length++] = value;
}

void drainInputs() {
  // Alternate channels one byte at a time. Arrival ordering closer than the
  // software decoder/loop latency cannot be treated as exact timing evidence.
  bool progressed;
  do {
    progressed = false;
    int value = espToMm32.read();
    if (value >= 0) {
      captureByte(fromEsp, static_cast<uint8_t>(value), millis());
      progressed = true;
    }
    value = mm32ToEsp.read();
    if (value >= 0) {
      captureByte(fromMm32, static_cast<uint8_t>(value), millis());
      progressed = true;
    }
  } while (progressed);
}

void flushIdleLines() {
  const uint32_t now = millis();
  if (fromEsp.length && now - fromEsp.lastMs >= IDLE_FLUSH_MS) emit(fromEsp);
  if (fromMm32.length && now - fromMm32.lastMs >= IDLE_FLUSH_MS) emit(fromMm32);
}

void reportOverflow() {
  if (espToMm32.overflow()) {
    Serial.printf("[%010lu][WARN] ESP>MM32 software-UART overflow; bytes were lost\n",
                  static_cast<unsigned long>(millis()));
  }
  if (mm32ToEsp.overflow()) {
    Serial.printf("[%010lu][WARN] MM32>ESP software-UART overflow; bytes were lost\n",
                  static_cast<unsigned long>(millis()));
  }
}

void emitNote(char key) {
  const __FlashStringHelper *note = nullptr;
  switch (key) {
    case '1': note = F("M.SET pressed"); break;
    case '2': note = F("M.SET released"); break;
    case '3': note = F("REC pressed"); break;
    case '4': note = F("REC released"); break;
    case '5': note = F("M.SET+REC pressed"); break;
    case '6': note = F("M.SET+REC released"); break;
    case 'm': case 'M': note = F("manual marker"); break;
    default: return;
  }
  Serial.printf("[%010lu][NOTE] ", static_cast<unsigned long>(millis()));
  Serial.println(note);
}

void handleUsbNotes() {
  while (Serial.available()) emitNote(static_cast<char>(Serial.read()));
}
}  // namespace

void setup() {
  Serial.begin(USB_BAUD);
  Serial.setDebugOutput(false);

  // Radio activity introduces interrupt latency and is unnecessary here.
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);

  // Larger edge buffers provide margin while a formatted USB line is sent.
  espToMm32.begin(CLOCK_BAUD, SWSERIAL_8N1, ESP_TX_INPUT, -1, false, 256, 2048);
  mm32ToEsp.begin(CLOCK_BAUD, SWSERIAL_8N1, MM32_TX_INPUT, -1, false, 256, 2048);
  // Avoid back-powering a sleeping clock through software-UART pull-ups.
  espToMm32.enableRxGPIOPullUp(false);
  mm32ToEsp.enableRxGPIOPullUp(false);

  Serial.println();
  Serial.println(F("MONITOR_READY CH899 dual UART 115200 8N1"));
  Serial.println(F("D5=clock TX (ESP>MM32), D6=clock RX (MM32>ESP)"));
  Serial.println(F("NOTE keys: 1/2 M.SET down/up, 3/4 REC down/up, 5/6 both down/up, M marker"));
}

void loop() {
  drainInputs();
  flushIdleLines();
  reportOverflow();
  handleUsbNotes();
  yield();
}
