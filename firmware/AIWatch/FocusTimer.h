#pragma once
#include <stdint.h>

class FocusTimer {
  static constexpr uint32_t duration = 25 * 60 * 1000;
  uint32_t left = duration;
  uint32_t started = 0;
  bool running = false;

public:
  constexpr void reset() { left = duration; running = false; }
  constexpr void tick(uint32_t now) {
    if (running && now - started >= left) {
      left = 0;
      running = false;
    }
  }
  constexpr void toggle(uint32_t now) {
    tick(now);
    if (running) left -= now - started;
    else {
      if (!left) left = duration;
      started = now;
    }
    running = !running;
  }
  constexpr uint32_t seconds(uint32_t now) const {
    const uint32_t elapsed = running ? now - started : 0;
    return elapsed >= left ? 0 : (left - elapsed + 999) / 1000;
  }
  constexpr bool active() const { return running; }
};
