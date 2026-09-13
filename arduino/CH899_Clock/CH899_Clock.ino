/*
  Copyright (C) 2026 Steve Madden
  SPDX-License-Identifier: GPL-3.0-or-later

  CH899 Clock replacement ESP8285 firmware

  Known compatible UART protocol:
    MM32 -> ESP: USER\n
    MM32 -> ESP: CLEAN\n
    ESP  -> MM32: USER_OK\r\n
    ESP  -> MM32: CLEAN_OK\r\n
    ESP  -> MM32: +TICK\r\n
    ESP  -> MM32: +TIME:Tue Sep  8 12:53:31 2026 +0100 10:00\r\n
  Serial is reserved for the MM32 clock controller.
*/

#include <Arduino.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "clean_reset_gate.h"
#include "clock_validation.h"
#include "clock_session.h"
#include "web_ui.h"

// Requires Arduino IDE Flash Size: "Mapping defined by Hardware and Sketch".
// This selects the hardware-sized map without a filesystem. The web page is
// compiled into flash; settings live in EEPROM. OTA is deliberately omitted
// from this application, not prevented by the flash map itself.
FLASH_MAP_SETUP_CONFIG(FLASH_MAP_NO_FS)

namespace {
constexpr uint32_t MM32_BAUD = 115200;
// A normal MM32-scheduled wake is short.  Give a saved network enough time to
// obtain DHCP, but do not spend the entire wake retrying or exposing an AP.
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 10000;
constexpr uint32_t NTP_TIMEOUT_MS = 8000;
constexpr uint32_t DAILY_SYNC_MS = 24UL * 60UL * 60UL * 1000UL;
constexpr uint32_t TIME_ANNOUNCE_INTERVAL_MS = 1000;
constexpr uint32_t MM32_TICK_INTERVAL_MS = 2000;
constexpr uint32_t WEB_SESSION_TIMEOUT_MS = 10000;
constexpr uint32_t WEB_SESSION_MAX_MS = 60UL * 1000UL;
constexpr uint32_t CLEAN_CONFIRM_MIN_MS = 1000;
constexpr uint32_t CLEAN_CONFIRM_MAX_MS = 4000;
constexpr uint32_t FACTORY_WIFI_CLEAR_DELAY_MS = 50;
constexpr uint32_t RESTART_DELAY_MS = 250;
// In the observed stock M.SET session, the ESP repeats the time record once
// per second after NTP succeeds. The MM32 ends the wake session; this firmware
// never drives a sleep pin.
constexpr bool AUTO_SEND_TIME_TO_MM32 = true;
constexpr char FIRMWARE_VERSION[] = "v1.0.1";
constexpr char AP_PREFIX[] = "wifi-clock-setup-";
constexpr char DEFAULT_NTP_HOST[] = "pool.ntp.org";
constexpr char DEFAULT_TIMEZONE[] = "GMT0BST,M3.5.0/1,M10.5.0/2";
constexpr size_t EEPROM_BYTES = 512;
constexpr uint32_t LEGACY_CONFIG_MAGIC = 0x43483939;  // "CH99"
constexpr uint32_t CONFIG_MAGIC = 0x4348393A;  // "CH9:"

DNSServer captiveDns;
ESP8266WebServer web(80);

struct ClockConfig {
  String ssid;
  String password;
  String ntpHost = DEFAULT_NTP_HOST;
  // POSIX TZ rule; this default handles UK GMT/BST correctly.
  String timezone = DEFAULT_TIMEZONE;
  uint8_t syncHour = 10;
  uint8_t syncMinute = 0;
} config;

struct StoredConfig {
  uint32_t magic;
  char ssid[33];
  char password[65];
  char ntpHost[254];
  char timezone[97];
  uint8_t syncHour;
  uint8_t syncMinute;
  uint32_t checksum;
};
static_assert(sizeof(StoredConfig) <= EEPROM_BYTES, "Settings exceed EEPROM allocation");
// Keep Arduino's generated prototypes from referring to ClockConfig before
// its definition when preprocessing this sketch.
bool saveConfig(const ClockConfig &candidate);
void scheduleRestart();

enum class NetworkState { Offline, Portal, Connecting, Connected, WaitingForNtp };
NetworkState networkState = NetworkState::Offline;
bool portalActive = false;
bool mdnsActive = false;
bool configurationUsable = false;
bool ntpRequested = false;
bool ntpFallbackAttempted = false;
bool ntpTimedOut = false;
bool ntpResponseReceived = false;
bool timeAnnouncementActive = false;
bool firstTimeAnnouncementPending = false;
ClockWebSession webSession(MM32_TICK_INTERVAL_MS, WEB_SESSION_TIMEOUT_MS, WEB_SESSION_MAX_MS);
bool scanRequested = false;
bool scanRunning = false;
bool scanReady = false;
bool scanFailed = false;
String scanResultJson;
uint32_t scanCompletedAt = 0;
constexpr uint32_t SCAN_CACHE_MS = 15000;
String requestToken;
bool restartScheduled = false;
bool factoryResetPending = false;
bool sdkCredentialsCleared = false;
bool cleanResetArmed = false;
uint32_t cleanResetArmedAt = 0;
uint32_t restartScheduledAt = 0;
uint32_t connectStartedAt = 0;
uint32_t ntpStartedAt = 0;
uint32_t lastSyncAt = 0;
uint32_t lastTimeAnnouncementAt = 0;
constexpr size_t UART_LINE_CAPACITY = 32;
char uartLine[UART_LINE_CAPACITY] = {};
size_t uartLineLength = 0;
bool uartLineDiscarding = false;

uint32_t configChecksum(const uint8_t *data, const size_t length) {
  uint32_t hash = 2166136261UL;
  for (size_t i = 0; i < length; ++i) hash = (hash ^ data[i]) * 16777619UL;
  return hash;
}

void copyText(char *destination, const size_t destinationSize, const String &source) {
  const size_t count = min(destinationSize - 1, source.length());
  memcpy(destination, source.c_str(), count);
  destination[count] = '\0';
}

String jsonEscape(const String &input) {
  String result;
  result.reserve(input.length() + 8);
  for (size_t i = 0; i < input.length(); ++i) {
    const uint8_t c = static_cast<uint8_t>(input[i]);
    if (c < 0x20) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned>(c));
      result += escaped;
    } else {
      if (c == '"' || c == '\\') result += '\\';
      result += static_cast<char>(c);
    }
  }
  return result;
}

String deviceHostname() {
  // Lowercase, unique DHCP hostname for a finished clock.
  return String("wifi-clock-") + String(ESP.getChipId(), HEX);
}

String deviceMdnsAddress() {
  // mDNS uses the same unique host label as DHCP, with the local domain.
  return deviceHostname() + ".local";
}

String deviceSetupName() {
  return String(AP_PREFIX) + String(ESP.getChipId(), HEX);
}

String normalizeHost(String host) {
  host.toLowerCase();
  if (host.endsWith(":80")) host.remove(host.length() - 3);
  return host;
}

bool knownHost(const String &host) {
  if (host == deviceHostname()) return true;
  if (host == deviceMdnsAddress()) return true;
  if (portalActive && host == WiFi.softAPIP().toString()) return true;
  return WiFi.status() == WL_CONNECTED && host == WiFi.localIP().toString();
}

void responseHeaders() {
  web.sendHeader("Cache-Control", "no-store");
  web.sendHeader("X-Content-Type-Options", "nosniff");
  web.sendHeader("X-Frame-Options", "DENY");
  web.sendHeader("Referrer-Policy", "no-referrer");
}

bool apiRequestAllowed(bool needsToken, bool allowBodyToken) {
  responseHeaders();
  const String host = normalizeHost(web.hostHeader());
  if (!knownHost(host)) {
    web.send(403, "application/json", "{\"error\":\"Open the clock using its IP address\"}");
    return false;
  }
  String origin = web.header("Origin");
  origin.toLowerCase();
  if (!origin.isEmpty() && (!origin.startsWith("http://") ||
      normalizeHost(origin.substring(7)) != host)) {
    web.send(403, "application/json", "{\"error\":\"Request must come from the clock setup page\"}");
    return false;
  }
  const bool validToken = !requestToken.isEmpty() &&
      (web.header("X-Clock-Token") == requestToken ||
       (allowBodyToken && web.method() == HTTP_POST && web.arg("token") == requestToken));
  if (needsToken && !validToken) {
    web.send(403, "application/json", "{\"error\":\"This page has expired. Reconnect and refresh it\"}");
    return false;
  }
  return true;
}

void createRequestToken() {
  uint8_t bytes[16];
  ESP.random(bytes, sizeof(bytes));
  char token[33];
  const char digits[] = "0123456789abcdef";
  for (size_t i = 0; i < sizeof(bytes); ++i) {
    token[i * 2] = digits[bytes[i] >> 4];
    token[i * 2 + 1] = digits[bytes[i] & 15];
  }
  token[32] = 0;
  requestToken = token;
}

bool safeText(const String &input, const size_t maxLength) {
  if (input.length() > maxLength) return false;
  for (size_t i = 0; i < input.length(); ++i) {
    if (static_cast<uint8_t>(input[i]) < 0x20) return false;
  }
  return true;
}

bool parseScheduleValue(const String &text, uint8_t maximum, uint8_t &value) {
  return clock_validation::parseScheduleValue(text.c_str(), text.length(), maximum, value);
}

bool loadConfig() {
  StoredConfig stored{};
  EEPROM.get(0, stored);
  const uint32_t expected = configChecksum(reinterpret_cast<const uint8_t *>(&stored), offsetof(StoredConfig, checksum));
  const bool legacyConfig = stored.magic == LEGACY_CONFIG_MAGIC;
  if ((!legacyConfig && stored.magic != CONFIG_MAGIC) || stored.checksum != expected ||
      stored.syncHour > 23 || (!legacyConfig &&
      (stored.syncMinute > 55 || stored.syncMinute % 5 != 0))) return false;
  // Check every fixed-size string before constructing a String from flash.
  if (!memchr(stored.ssid, 0, sizeof(stored.ssid)) ||
      !memchr(stored.password, 0, sizeof(stored.password)) ||
      !memchr(stored.ntpHost, 0, sizeof(stored.ntpHost)) ||
      !memchr(stored.timezone, 0, sizeof(stored.timezone))) return false;
  if (!safeText(stored.ssid, 32) || !safeText(stored.password, 64) ||
      !safeText(stored.ntpHost, 253) || !safeText(stored.timezone, 96)) return false;
  config.ssid = stored.ssid;
  config.password = stored.password;
  config.ntpHost = stored.ntpHost[0] ? stored.ntpHost : DEFAULT_NTP_HOST;
  config.timezone = stored.timezone[0] ? stored.timezone : DEFAULT_TIMEZONE;
  config.syncHour = stored.syncHour;
  config.syncMinute = legacyConfig ? 0 : stored.syncMinute;
  // Earlier replacement builds allowed arbitrary hour/minute pairs. Preserve
  // their Wi-Fi/time-zone settings, but safely migrate an unsupported MM32
  // schedule to the stock default without adding a flash write at boot.
  if (!clock_validation::supportedDailyUpdate(config.syncHour, config.syncMinute)) {
    config.syncHour = 10;
    config.syncMinute = 0;
  }
  // Keep readable old values visible for correction, but do not send time
  // from a malformed rule that a previous firmware happened to accept.
  return !config.ssid.isEmpty() &&
      clock_validation::validNtpHost(config.ntpHost.c_str(), config.ntpHost.length()) &&
      clock_validation::validPosixTimezone(config.timezone.c_str(), config.timezone.length());
}

bool saveConfig(const ClockConfig &candidate) {
  StoredConfig stored{};
  stored.magic = CONFIG_MAGIC;
  copyText(stored.ssid, sizeof(stored.ssid), candidate.ssid);
  copyText(stored.password, sizeof(stored.password), candidate.password);
  copyText(stored.ntpHost, sizeof(stored.ntpHost), candidate.ntpHost);
  copyText(stored.timezone, sizeof(stored.timezone), candidate.timezone);
  stored.syncHour = candidate.syncHour;
  stored.syncMinute = candidate.syncMinute;
  stored.checksum = configChecksum(reinterpret_cast<const uint8_t *>(&stored), offsetof(StoredConfig, checksum));
  EEPROM.put(0, stored);
  return EEPROM.commit();
}

bool eraseSavedConfig() {
  for (size_t address = 0; address < EEPROM_BYTES; ++address) {
    EEPROM.write(address, 0xFF);
  }
  return EEPROM.commit();
}

bool prepareFactoryReset() {
  // Quiesce every protocol producer before the potentially slow flash commit.
  // USER requests can still be acknowledged by the UART parser while the
  // deferred restart is pending.
  timeAnnouncementActive = false;
  firstTimeAnnouncementPending = false;
  ntpRequested = false;
  ntpResponseReceived = false;
  webSession.stop();
  scanRequested = false;
  scanReady = false;
  scanFailed = false;
  scanResultJson = String();

  if (!eraseSavedConfig()) return false;
  config = ClockConfig{};
  configurationUsable = false;
  factoryResetPending = true;
  sdkCredentialsCleared = false;
  return true;
}

void acknowledgeClean() {
  Serial.print(F("CLEAN_OK\r\n"));
  // The stock firmware restarts shortly after the acknowledgement. Ensure the
  // complete record has physically left the UART before starting that timer.
  Serial.flush();
}

void startPortal() {
  WiFi.mode(WIFI_AP_STA);
  const String apName = deviceSetupName();
  WiFi.softAP(apName.c_str());
  captiveDns.start(53, "*", WiFi.softAPIP());
  portalActive = true;
  networkState = NetworkState::Portal;
}

void startMdns() {
  if (mdnsActive || WiFi.status() != WL_CONNECTED) return;
  const String hostname = deviceHostname();
  mdnsActive = MDNS.begin(hostname.c_str());
  if (mdnsActive) MDNS.addService("http", "tcp", 80);
}

void discardWifiScan() {
  scanRequested = false;
  scanReady = false;
  scanFailed = false;
  scanResultJson = String();
  // scanDelete() frees results, but does not cancel an SDK scan in progress.
  // Keep tracking that scan until completion, then discard its late results.
  // Never wait here: MM32 UART handling must remain responsive while offline.
  if (scanRunning && WiFi.scanComplete() == WIFI_SCAN_RUNNING) return;
  scanRunning = false;
  WiFi.scanDelete();
}

void endNetworkWake() {
  // A configured clock must not fall back to a discoverable AP during its
  // daily wake.  The next MM32 reset is a fresh, single station attempt.
  networkState = NetworkState::Offline;
  webSession.stop();
  discardWifiScan();
  ntpRequested = false;
  ntpResponseReceived = false;
  timeAnnouncementActive = false;
  firstTimeAnnouncementPending = false;
  if (portalActive) {
    captiveDns.stop();
    WiFi.softAPdisconnect(false);
    portalActive = false;
  }
  mdnsActive = false;
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
}

void beginStationConnection() {
  if (!configurationUsable || config.ssid.isEmpty()) {
    if (!portalActive) startPortal();
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  connectStartedAt = millis();
  networkState = NetworkState::Connecting;
}

void sendTimeToClock(const tm &localTime) {
  char timeText[48];
  strftime(timeText, sizeof(timeText), "%a %b %e %T %Y %z", &localTime);
  // Preserve the only observed ESP-to-MM32 time record format.
  Serial.printf("+TIME:%s %02u:%02u\r\n", timeText, config.syncHour, config.syncMinute);
}

void serviceWebSession() {
  if (networkState == NetworkState::Offline) {
    webSession.stop();
    return;
  }
  if (webSession.takeTick(millis())) Serial.print("+TICK\r\n");
}

void beginTimeAnnouncement() {
  timeAnnouncementActive = true;
  firstTimeAnnouncementPending = true;
  const uint32_t nowMs = millis();
  lastTimeAnnouncementAt = nowMs;
}

void serviceTimeAnnouncement() {
  if (!timeAnnouncementActive || restartScheduled) return;

  const uint32_t nowMs = millis();
  if (firstTimeAnnouncementPending) {
    firstTimeAnnouncementPending = false;
    const bool pageIsActive = webSession.active(nowMs);
    if (pageIsActive) {
      // Give the MM32 a full second to process a page-session keepalive before
      // it receives the first time-completion record.
      Serial.print("+TICK\r\n");
      webSession.markTick(nowMs);
      lastTimeAnnouncementAt = nowMs;
      return;
    }
  }
  if (nowMs - lastTimeAnnouncementAt < TIME_ANNOUNCE_INTERVAL_MS) return;

  const time_t now = time(nullptr);
  if (now <= 1700000000) return;
  tm localTime{};
  localtime_r(&now, &localTime);
  sendTimeToClock(localTime);
  lastTimeAnnouncementAt = nowMs;
}

void requestNtpHost(const String &host) {
  ntpRequested = true;
  ntpTimedOut = false;
  ntpStartedAt = millis();
  networkState = NetworkState::WaitingForNtp;
  // The const char* overload borrows server pointers. The String overload
  // retains its own copy, including when the fallback uses a temporary String.
  configTime(config.timezone.c_str(), host);
}

void beginNtpSync() {
  if (WiFi.status() != WL_CONNECTED) return;
  ntpFallbackAttempted = false;
  ntpResponseReceived = false;
  timeAnnouncementActive = false;
  firstTimeAnnouncementPending = false;
  requestNtpHost(config.ntpHost);
}

void serviceNtp() {
  if (!ntpRequested) return;
  const time_t now = time(nullptr);
  if (ntpResponseReceived && now > 1700000000) {
    if (AUTO_SEND_TIME_TO_MM32) beginTimeAnnouncement();
    lastSyncAt = millis();
    ntpRequested = false;
    networkState = NetworkState::Connected;
  } else if (!ntpTimedOut && millis() - ntpStartedAt >= NTP_TIMEOUT_MS) {
    if (!ntpFallbackAttempted && config.ntpHost != DEFAULT_NTP_HOST) {
      ntpFallbackAttempted = true;
      requestNtpHost(DEFAULT_NTP_HOST);
    } else {
      // SNTP continues retrying in the core. Accept a late successful reply
      // for the remainder of this wake instead of silently discarding it.
      ntpTimedOut = true;
    }
  }
}

void serviceNetwork() {
  if (networkState == NetworkState::Connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      startMdns();
      beginNtpSync();
    } else if (millis() - connectStartedAt >= WIFI_CONNECT_TIMEOUT_MS) {
      endNetworkWake();
    }
  }
  if ((networkState == NetworkState::Connected ||
       networkState == NetworkState::WaitingForNtp) && WiFi.status() != WL_CONNECTED) {
    endNetworkWake();
  }
  if (networkState == NetworkState::Connected &&
      millis() - lastSyncAt > DAILY_SYNC_MS) beginNtpSync();
}

void handleUartLine(const char *line) {
  if (strcmp(line, "USER") == 0) {
    Serial.print(F("USER_OK\r\n"));
  } else if (strcmp(line, "CLEAN") == 0) {
    // Stock does not act on the first CLEAN: it waits for the MM32's repeat at
    // roughly two seconds. This confirmation prevents a damaged or isolated
    // line from erasing settings. Once blank, acknowledge retries without
    // another flash write or software-restart loop.
    if (factoryResetPending) {
      acknowledgeClean();
      return;
    }

    const uint32_t nowMs = millis();
    const uint32_t elapsedMs = nowMs - cleanResetArmedAt;
    const clock_protocol::CleanDecision decision = clock_protocol::decideClean(
        configurationUsable, cleanResetArmed, elapsedMs,
        CLEAN_CONFIRM_MIN_MS, CLEAN_CONFIRM_MAX_MS);

    switch (decision) {
      case clock_protocol::CleanDecision::AcknowledgeOnly:
        cleanResetArmed = false;
        acknowledgeClean();
        return;
      case clock_protocol::CleanDecision::Arm:
      case clock_protocol::CleanDecision::Rearm:
        cleanResetArmed = true;
        cleanResetArmedAt = nowMs;
        return;
      case clock_protocol::CleanDecision::Wait:
        return;
      case clock_protocol::CleanDecision::Confirm:
        cleanResetArmed = false;
        if (!prepareFactoryReset()) return;  // No acknowledgement: MM32 retries.
        acknowledgeClean();
        scheduleRestart();
        return;
    }
  }
  // Intentionally ignore unknown commands until they are captured and decoded.
}

void serviceMm32Uart() {
  while (Serial.available()) {
    const char received = static_cast<char>(Serial.read());
    if (received == '\r') continue;
    if (received == '\n') {
      if (!uartLineDiscarding && uartLineLength) {
        uartLine[uartLineLength] = '\0';
        handleUartLine(uartLine);
      }
      uartLineLength = 0;
      uartLineDiscarding = false;
    } else if (!uartLineDiscarding) {
      if (uartLineLength + 1 < UART_LINE_CAPACITY) {
        uartLine[uartLineLength++] = received;
      } else {
        // Discard the complete oversized record. Never treat its tail as a
        // fresh command after noise or an unexpected future message.
        uartLineLength = 0;
        uartLineDiscarding = true;
      }
    }
  }
}

void serveIndex() {
  responseHeaders();
  if (!knownHost(normalizeHost(web.hostHeader()))) {
    // Captive-portal probes use unrelated hostnames. Move the actual page to
    // this socket's local IP so its API origin can be checked unambiguously.
    web.sendHeader("Location", String("http://") + web.client().localIP().toString() + "/");
    web.send(302, "text/plain", "Open the clock setup page");
    return;
  }
  web.sendHeader("Content-Security-Policy", "frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
  web.send_P(200, PSTR("text/html; charset=utf-8"), CLOCK_WEB_UI);
}

void keepWebSessionAlive() {
  if (!apiRequestAllowed(true, false)) return;
  // A request already buffered before disconnection must not revive a session.
  if (networkState == NetworkState::Offline) {
    web.send(503, "application/json", "{\"error\":\"Wi-Fi is off for this wake\"}");
    return;
  }
  const String page = web.arg("page");
  if (!ClockWebSession::validId(page.c_str())) {
    web.send(400, "application/json", "{\"error\":\"Invalid page session\"}");
    return;
  }
  webSession.note(page.c_str(), web.arg("start") == "1", millis());
  web.send(204, "text/plain", "");
}

void closeWebSession() {
  if (!apiRequestAllowed(true, true)) return;
  const String page = web.arg("page");
  if (!ClockWebSession::validId(page.c_str())) {
    web.send(400, "application/json", "{\"error\":\"Invalid page session\"}");
    return;
  }
  webSession.close(page.c_str());
  web.send(204, "text/plain", "");
}

void scheduleRestart() {
  restartScheduled = true;
  restartScheduledAt = millis();
}

void serviceRestart() {
  if (!restartScheduled) return;
  const uint32_t elapsed = millis() - restartScheduledAt;
  if (factoryResetPending && !sdkCredentialsCleared &&
      elapsed >= FACTORY_WIFI_CLEAR_DELAY_MS) {
    // Our credentials live in EEPROM, but an application-only flash can leave
    // old SDK station credentials behind. Clear those persistently as part of
    // Factory reset without claiming to erase every historical flash byte.
    WiFi.persistent(true);
    WiFi.disconnect(true, true);
    WiFi.persistent(false);
    sdkCredentialsCleared = true;
  }
  if (elapsed >= RESTART_DELAY_MS) ESP.restart();
}

void eraseConfigAndRestart() {
  if (!apiRequestAllowed(true, false)) return;
  if (restartScheduled) {
    web.send(409, "application/json", "{\"error\":\"Clock is restarting\"}");
    return;
  }
  if (!prepareFactoryReset()) {
    web.send(500, "application/json", "{\"error\":\"settings could not be erased\"}");
    return;
  }
  web.send(202, "application/json", "{\"reset\":\"scheduled\"}");
  scheduleRestart();
}

void sendStatus() {
  if (!apiRequestAllowed(false, false)) return;
  const bool stationConnected = WiFi.status() == WL_CONNECTED;
  const char *mode = stationConnected ? "station" : (portalActive ? "portal" : "offline");
  const String body = String("{\"apiVersion\":1,\"mode\":\"") + mode +
      "\",\"firmwareVersion\":\"" + FIRMWARE_VERSION +
      "\",\"deviceName\":\"" + jsonEscape(deviceSetupName()) +
      "\",\"ssid\":\"" + jsonEscape(config.ssid) + "\",\"ip\":\"" +
      (stationConnected ? WiFi.localIP().toString() : WiFi.softAPIP().toString()) +
      "\",\"mdnsHost\":\"" + jsonEscape(deviceMdnsAddress()) +
      "\",\"stationConnected\":" + (stationConnected ? "true" : "false") + "}";
  web.send(200, "application/json", body);
}

void sendConfig() {
  if (!apiRequestAllowed(false, false)) return;
  const String body = String("{\"ssid\":\"") + jsonEscape(config.ssid) +
      "\",\"ntpHost\":\"" + jsonEscape(config.ntpHost) +
      "\",\"timezone\":\"" + jsonEscape(config.timezone) +
      "\",\"syncHour\":" + String(config.syncHour) +
      ",\"syncMinute\":" + String(config.syncMinute) +
      ",\"hasPassword\":" + (config.password.isEmpty() ? "false" : "true") +
      ",\"requestToken\":\"" + requestToken + "\"}";
  web.send(200, "application/json", body);
}

void updateConfig() {
  if (!apiRequestAllowed(true, false)) return;
  if (restartScheduled) {
    web.send(409, "application/json", "{\"error\":\"Clock is restarting\"}");
    return;
  }
  const String ssid = web.arg("ssid");
  const String password = web.arg("password");
  String ntpHost = web.arg("ntpHost");
  String timezone = web.arg("timezone");
  ntpHost.trim();
  timezone.trim();
  if (ntpHost.isEmpty()) ntpHost = DEFAULT_NTP_HOST;
  if (timezone.isEmpty()) timezone = DEFAULT_TIMEZONE;
  uint8_t syncHour = 0;
  uint8_t syncMinute = 0;
  if (ssid.isEmpty() || !safeText(ssid, 32) || !safeText(password, 64) ||
      !safeText(ntpHost, 253) || !safeText(timezone, 96) ||
      !parseScheduleValue(web.arg("syncHour"), 23, syncHour) ||
      !parseScheduleValue(web.arg("syncMinute"), 59, syncMinute) ||
      !clock_validation::supportedDailyUpdate(syncHour, syncMinute)) {
    web.send(400, "application/json", "{\"error\":\"Invalid configuration\"}");
    return;
  }
  if (!clock_validation::validNtpHost(ntpHost.c_str(), ntpHost.length())) {
    web.send(400, "application/json", "{\"error\":\"Enter an NTP hostname or IPv4 address, without a URL, port or path\"}");
    return;
  }
  if (!clock_validation::validPosixTimezone(timezone.c_str(), timezone.length())) {
    web.send(400, "application/json", "{\"error\":\"Enter a valid POSIX time-zone rule. Daylight-saving rules need both start and end dates\"}");
    return;
  }
  ClockConfig candidate = config;
  // The setup page intentionally does not reveal a saved Wi-Fi password.
  // Retain a blank password only for the SAME network; never reuse another
  // network's credentials. A newly selected open network can then be saved.
  if (!password.isEmpty() || ssid != config.ssid) candidate.password = password;
  candidate.ssid = ssid;
  candidate.ntpHost = ntpHost;
  candidate.timezone = timezone;
  candidate.syncHour = syncHour;
  candidate.syncMinute = syncMinute;
  if (!saveConfig(candidate)) {
    web.send(500, "application/json", "{\"error\":\"Could not save configuration\"}");
    return;
  }
  config = candidate;
  configurationUsable = true;
  // Save and schedule the restart in ONE transaction. A lost second browser
  // request must not leave new settings saved but unapplied for this wake.
  web.send(200, "application/json", "{\"saved\":true,\"reboot\":\"scheduled\"}");
  scheduleRestart();
}

void serviceWifiScan() {
  if (networkState == NetworkState::Offline) {
    discardWifiScan();
    return;
  }
  if (scanRunning) {
    const int found = WiFi.scanComplete();
    if (found == WIFI_SCAN_RUNNING) return;
    scanRunning = false;
    scanFailed = found < 0;
    if (!scanFailed) {
      scanResultJson = "{\"networks\":[";
      for (int i = 0; i < found && i < 40; ++i) {
        if (i) scanResultJson += ',';
        scanResultJson += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) +
            "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
      }
      scanResultJson += "]}";
      scanReady = true;
      scanCompletedAt = millis();
    }
    // Release ALL SDK BSS records even if the page disappeared before polling.
    WiFi.scanDelete();
  }
  if (scanReady && millis() - scanCompletedAt >= SCAN_CACHE_MS) {
    scanResultJson = String();
    scanReady = false;
  }
  if (scanRequested && !restartScheduled && networkState != NetworkState::Connecting) {
    scanRequested = false;
    scanReady = false;
    scanFailed = false;
    scanResultJson = String();
    // A synchronous scan suspends loop(), including MM32 handshakes/ticks.
    // Starting during Connecting also aborts association in core 3.1.2.
    scanRunning = WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING;
    scanFailed = !scanRunning;
  }
}

void scanNetworks() {
  if (!apiRequestAllowed(true, false)) return;
  if (networkState == NetworkState::Offline) {
    web.send(503, "application/json", "{\"error\":\"Wi-Fi is off for this wake\"}");
    return;
  }
  if (web.arg("start") == "1" && !scanRunning) scanRequested = true;
  if (scanRequested || scanRunning) {
    web.send(202, "application/json", "{\"scanning\":true}");
    return;
  }
  if (scanFailed || !scanReady) {
    web.send(503, "application/json", "{\"error\":\"Network scan failed; try again or enter a network manually\"}");
    return;
  }
  web.send(200, "application/json", scanResultJson);
}

void setupWebServer() {
  web.collectHeaders("Origin", "X-Clock-Token");
  web.on("/", HTTP_GET, serveIndex);
  web.on("/api/v1/status", HTTP_GET, sendStatus);
  web.on("/api/v1/config", HTTP_GET, sendConfig);
  web.on("/api/v1/config", HTTP_POST, updateConfig);
  web.on("/api/v1/scan", HTTP_GET, scanNetworks);
  web.on("/api/v1/keepalive", HTTP_POST, keepWebSessionAlive);
  web.on("/api/v1/session/close", HTTP_POST, closeWebSession);
  web.on("/api/v1/factory-reset", HTTP_POST, eraseConfigAndRestart);
  web.on("/api/v1/reboot", HTTP_POST, []() {
    if (!apiRequestAllowed(true, false)) return;
    web.send(202, "application/json", "{\"reboot\":\"scheduled\"}");
    if (!restartScheduled) scheduleRestart();
  });
  web.on("/generate_204", HTTP_GET, serveIndex);
  web.on("/hotspot-detect.html", HTTP_GET, serveIndex);
  web.onNotFound([]() {
    if (web.uri().startsWith("/api/") || web.method() != HTTP_GET) {
      web.send(404, "application/json", "{\"error\":\"Unknown endpoint\"}");
    } else serveIndex();
  });
  web.begin();
}
}  // namespace

void setup() {
  Serial.begin(MM32_BAUD);
  Serial.setDebugOutput(false);
  WiFi.persistent(false);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  // Only the core's SNTP update callback counts as a successful validation.
  // An earlier, already-held time must not validate a newly entered server.
  settimeofday_cb([](bool fromSntp) {
    if (fromSntp && ntpRequested) ntpResponseReceived = true;
  });
  EEPROM.begin(EEPROM_BYTES);
  configurationUsable = loadConfig();
  const String hostname = deviceHostname();
  WiFi.hostname(hostname.c_str());
  createRequestToken();
  setupWebServer();
  if (configurationUsable) {
    // Normal and scheduled wakes are station-only.  This limits radio time and
    // prevents a configuration AP appearing whenever the clock updates.
    beginStationConnection();
  } else {
    // A blank (first-flashed or factory-reset) unit needs a local setup page.
    // Clear only RAM-held SDK credentials before bringing up its AP.
    WiFi.disconnect(false, true);
    startPortal();
  }
}

void loop() {
  serviceMm32Uart();
  // Do not accept another slow HTTP body/response after acknowledging restart.
  if (restartScheduled) {
    serviceRestart();
    yield();
    return;
  }
  serviceNetwork();
  serviceNtp();
  serviceTimeAnnouncement();
  serviceWebSession();
  if (portalActive) captiveDns.processNextRequest();
  if (mdnsActive) MDNS.update();
  web.handleClient();
  serviceWifiScan();
  serviceRestart();
  yield();
}
