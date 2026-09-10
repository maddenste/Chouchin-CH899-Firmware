// Copyright (C) 2026 Steve Madden
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CH899_CLOCK_SESSION_H
#define CH899_CLOCK_SESSION_H
#include <stdint.h>
#include <string.h>

// One active browser owner, bounded storage, no Arduino/network dependency.
// IDs distinguish pages; the separate per-boot API token is the CSRF guard.
class ClockWebSession {
 public:
  ClockWebSession(uint32_t interval, uint32_t timeout, uint32_t maximum)
      : interval_(interval), timeout_(timeout), maximum_(maximum) {}

  static bool validId(const char *id) {
    if (!id) return false;
    for (unsigned i = 0; i < 32; ++i) {
      if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) return false;
    }
    return id[32] == '\0';
  }

  void note(const char *id, bool start, uint32_t now) {
    if (!validId(id)) return;
    expire(now);
    if (start) {
      if (retired(id)) return;
      if (strcmp(owner_, id) == 0) {
        // A lost start response can be retried without moving the deadline.
        if (active_) lastSeen_ = now;
        return;
      }
      if (owner_[0]) remember(owner_);
      memcpy(owner_, id, sizeof(owner_));
      started_ = lastSeen_ = now;
      lastTick_ = now - interval_;
      active_ = true;
    } else if (active_ && strcmp(owner_, id) == 0) {
      lastSeen_ = now;
    }
  }

  void close(const char *id) {
    if (!validId(id)) return;
    // Remember close-before-start too; a delayed initial request must not
    // resurrect a just-closed page. Four tombstones bound memory consumption.
    remember(id);
    if (strcmp(owner_, id) == 0) active_ = false;
  }

  void stop() {
    active_ = false;
    if (owner_[0]) remember(owner_);
  }

  void expire(uint32_t now) {
    if (active_ && (now - started_ >= maximum_ || now - lastSeen_ >= timeout_)) stop();
  }

  bool active(uint32_t now) { expire(now); return active_; }

  bool takeTick(uint32_t now) {
    if (!active(now) || now - lastTick_ < interval_) return false;
    lastTick_ = now;
    return true;
  }

  void markTick(uint32_t now) { if (active(now)) lastTick_ = now; }

 private:
  bool retired(const char *id) const {
    for (unsigned i = 0; i < 4; ++i) if (strcmp(closed_[i], id) == 0) return true;
    return false;
  }

  void remember(const char *id) {
    if (!id[0] || retired(id)) return;
    memcpy(closed_[nextClosed_], id, sizeof(owner_));
    nextClosed_ = (nextClosed_ + 1) % 4;
  }

  const uint32_t interval_, timeout_, maximum_;
  char owner_[33] = {};
  char closed_[4][33] = {};
  unsigned nextClosed_ = 0;
  bool active_ = false;
  uint32_t started_ = 0, lastSeen_ = 0, lastTick_ = 0;
};
#endif
