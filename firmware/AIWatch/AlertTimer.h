#pragma once
#include <stdint.h>

// Full-screen "needs you" alert: fires when attention starts, again every 2 minutes
// while it is still pending, and is silenced by touch/acknowledge in between.
class AlertTimer {
  static constexpr uint32_t reminder = 120 * 1000;
  uint32_t last = 0;
  bool waiting = false;
  bool shown = false;

public:
  // Returns true when the alert (re)appears on this call.
  constexpr bool poll(bool attention, uint32_t now) {
    if (!attention) {
      waiting = shown = false;
      return false;
    }
    if (!waiting || now - last >= reminder) {
      waiting = shown = true;
      last = now;
      return true;
    }
    return false;
  }
  constexpr void dismiss() { shown = false; }
  constexpr bool visible() const { return shown; }
};
