#include "../firmware/AIWatch/FocusTimer.h"

constexpr bool focusLifecycle() {
  FocusTimer timer;
  if (timer.seconds(0) != 1500 || timer.active()) return false;
  timer.toggle(100);
  timer.toggle(5100);
  if (timer.active() || timer.seconds(9000) != 1495) return false;
  timer.toggle(9000);
  timer.tick(1504000);
  if (timer.active() || timer.seconds(1504000) != 0) return false;
  timer.reset();
  return timer.seconds(1504000) == 1500 && !timer.active();
}

constexpr bool wrapsMonotonicClock() {
  FocusTimer timer;
  timer.toggle(UINT32_MAX - 499);
  return timer.seconds(500) == 1499;
}

static_assert(focusLifecycle(), "focus lifecycle");
static_assert(wrapsMonotonicClock(), "monotonic wraparound");
