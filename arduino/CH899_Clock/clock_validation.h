// Copyright (C) 2026 Steve Madden
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CH899_CLOCK_VALIDATION_H
#define CH899_CLOCK_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

// Small, allocation-free input validators; no Arduino or network dependency.
// Callers substitute defaults for blank fields before calling these functions.
//
// Time-zone grammar targets the newlib-xtensa parser used by ESP8266 core 3.1.2:
// https://github.com/earlephilhower/newlib-xtensa/blob/6e613cf95041437311f36a17b434d5658245536f/newlib/libc/time/tzset_r.c
// The toolchain reports newlib 4.0.0 but includes that fork's bracket-name fix.
// Unlike libc's permissive parser, this validates the COMPLETE input and
// requires explicit start/end dates whenever a DST abbreviation is present.
// Name limits match the fork: 10 unquoted letters, or 8 characters inside <>.
// Negative transition times and suffixes such as /2u are not supported by that
// parser. Nonnegative transition hours 0..167 include /24 used in core TZ.h.
// UTC offsets must be whole minutes because the clock's +TIME offset is %z.
// This is syntax validation, not a geographical/political time-zone database;
// it does not prove that the MM32 supports every valid region or offset.

namespace clock_validation {
namespace detail {

inline bool digit(char c) { return c >= '0' && c <= '9'; }
inline bool alpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
inline bool alphanumeric(char c) { return alpha(c) || digit(c); }

class Reader {
 public:
  Reader(const char *text, size_t length) : text_(text), length_(length), position_(0) {}
  char current() const { return position_ < length_ ? text_[position_] : '\0'; }
  bool done() const { return position_ == length_; }
  void advance() { if (position_ < length_) ++position_; }
  bool take(char expected) {
    if (done() || current() != expected) return false;
    advance();
    return true;
  }
  bool number(unsigned maximum, unsigned maxDigits, unsigned &value) {
    unsigned result = 0;
    unsigned count = 0;
    while (digit(current())) {
      if (++count > maxDigits) return false;
      result = result * 10 + static_cast<unsigned>(current() - '0');
      if (result > maximum) return false;
      advance();
    }
    if (!count) return false;
    value = result;
    return true;
  }

 private:
  const char *text_;
  size_t length_;
  size_t position_;
};

inline bool abbreviation(Reader &reader) {
  const bool quoted = reader.take('<');
  const unsigned maximum = quoted ? 8 : 10;
  unsigned count = 0;
  while (alpha(reader.current()) ||
         (quoted && (digit(reader.current()) || reader.current() == '+' || reader.current() == '-'))) {
    if (++count > maximum) return false;
    reader.advance();
  }
  return count >= 3 && (!quoted || reader.take('>'));
}

inline bool clockTime(Reader &reader, unsigned maxHour, unsigned hourDigits,
                      bool allowSeconds) {
  unsigned hour = 0;
  unsigned minute = 0;
  unsigned second = 0;
  if (!reader.number(maxHour, hourDigits, hour)) return false;
  if (reader.take(':')) {
    if (!reader.number(59, 2, minute)) return false;
    if (reader.take(':') && !reader.number(59, 2, second)) return false;
  }
  return allowSeconds || second == 0;
}

inline bool offset(Reader &reader) {
  if (reader.current() == '+' || reader.current() == '-') reader.advance();
  return clockTime(reader, 24, 2, false);
}

inline bool transition(Reader &reader) {
  unsigned number = 0;
  if (reader.take('M')) {
    if (!reader.number(12, 2, number) || number == 0 || !reader.take('.')) return false;
    if (!reader.number(5, 1, number) || number == 0 || !reader.take('.')) return false;
    if (!reader.number(6, 1, number)) return false;
  } else if (reader.take('J')) {
    if (!reader.number(365, 3, number) || number == 0) return false;
  } else if (!reader.number(365, 3, number)) {
    return false;
  }
  return !reader.take('/') || clockTime(reader, 167, 3, true);
}

inline bool ipv4(const char *text, size_t length) {
  Reader reader(text, length);
  for (unsigned index = 0; index < 4; ++index) {
    const char first = reader.current();
    unsigned value = 0;
    // Reject leading zeroes: lwIP can otherwise interpret components as octal.
    if (first == '0') {
      reader.advance();
      if (digit(reader.current())) return false;
    } else if (!reader.number(255, 3, value)) {
      return false;
    }
    if (index < 3 && !reader.take('.')) return false;
  }
  return reader.done();
}

}  // namespace detail

inline bool validPosixTimezone(const char *text, size_t length) {
  if (!text || length == 0 || length > 96) return false;
  for (size_t i = 0; i < length; ++i) {
    const unsigned char c = static_cast<unsigned char>(text[i]);
    if (c <= 0x20 || c >= 0x7F) return false;
  }
  detail::Reader reader(text, length);
  if (!detail::abbreviation(reader) || !detail::offset(reader)) return false;
  if (reader.done()) return true;  // Fixed offset: no DST rules required.
  if (!detail::abbreviation(reader)) return false;
  if (reader.current() != ',' && !detail::offset(reader)) return false;
  return reader.take(',') && detail::transition(reader) &&
         reader.take(',') && detail::transition(reader) && reader.done();
}

// Hostname only, not a URL: ASCII DNS labels or canonical dotted-decimal IPv4.
// Single-label local DNS names and a DNS trailing dot are allowed. This does
// not perform DNS/NTP reachability checks. IPv6 needs a different lwIP build
// and is deliberately not accepted by this IPv4 firmware's settings page.
inline bool validNtpHost(const char *text, size_t length) {
  if (!text || length == 0 || length > 253) return false;
  bool numeric = true;
  for (size_t i = 0; i < length; ++i) {
    if (!detail::digit(text[i]) && text[i] != '.') numeric = false;
  }
  if (numeric) return detail::ipv4(text, length);
  if (text[length - 1] == '.') --length;
  if (!length) return false;
  size_t labelLength = 0;
  char previous = '\0';
  for (size_t i = 0; i < length; ++i) {
    const char c = text[i];
    if (c == '.') {
      if (!labelLength || previous == '-') return false;
      labelLength = 0;
    } else {
      if (!detail::alphanumeric(c) && c != '-') return false;
      if ((!labelLength && c == '-') || ++labelLength > 63) return false;
    }
    previous = c;
  }
  return labelLength != 0 && previous != '-';
}

// Strict decimal value for a two-digit clock field. The output is unchanged
// on failure.
inline bool parseScheduleValue(const char *text, size_t length,
                               uint8_t maximum, uint8_t &value) {
  if (!text || length == 0 || length > 2) return false;
  detail::Reader reader(text, length);
  unsigned parsed = 0;
  if (!reader.number(maximum, 2, parsed) || !reader.done()) return false;
  value = static_cast<uint8_t>(parsed);
  return true;
}

// Direction-labelled stock captures show that the ESP will transmit arbitrary
// HH:MM strings, but scheduled MM32 wakes have only succeeded for the four
// values exposed by the original page. Do not offer or store a time that may
// silently leave an end user's clock without its daily update.
inline bool supportedDailyUpdate(uint8_t hour, uint8_t minute) {
  return minute == 0 && (hour == 9 || hour == 10 || hour == 21 || hour == 22);
}

}  // namespace clock_validation

#endif  // CH899_CLOCK_VALIDATION_H
