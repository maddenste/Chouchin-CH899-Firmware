// Copyright (C) 2026 Steve Madden
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

namespace clock_protocol {

enum class CleanDecision : uint8_t {
  AcknowledgeOnly,
  Arm,
  Wait,
  Confirm,
  Rearm,
};

// Stock waits for a repeated CLEAN before erasing its saved configuration.
// Once the configuration is already blank, acknowledge further CLEAN commands
// without writing flash or restarting again.
constexpr CleanDecision decideClean(const bool configurationUsable,
                                    const bool armed,
                                    const uint32_t elapsedMs,
                                    const uint32_t minimumMs,
                                    const uint32_t maximumMs) {
  return !configurationUsable
             ? CleanDecision::AcknowledgeOnly
             : !armed
                   ? CleanDecision::Arm
                   : elapsedMs < minimumMs
                         ? CleanDecision::Wait
                         : elapsedMs <= maximumMs ? CleanDecision::Confirm
                                                  : CleanDecision::Rearm;
}

static_assert(decideClean(false, false, 0, 1000, 4000) ==
                  CleanDecision::AcknowledgeOnly,
              "Blank settings must not cause another erase or restart");
static_assert(decideClean(true, false, 0, 1000, 4000) == CleanDecision::Arm,
              "The first CLEAN must only arm the reset");
static_assert(decideClean(true, true, 999, 1000, 4000) == CleanDecision::Wait,
              "An immediate duplicate must not confirm the reset");
static_assert(decideClean(true, true, 2062, 1000, 4000) ==
                  CleanDecision::Confirm,
              "The observed stock CLEAN cadence must confirm the reset");
static_assert(decideClean(true, true, 4001, 1000, 4000) ==
                  CleanDecision::Rearm,
              "A stale CLEAN must start a new confirmation window");

}  // namespace clock_protocol
