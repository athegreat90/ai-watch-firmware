#include "../firmware/AIWatch/AlertTimer.h"

constexpr bool remindsEveryTwoMinutes() {
  AlertTimer alert;
  if (alert.poll(false, 0) || alert.visible()) return false;
  if (!alert.poll(true, 1000) || !alert.visible()) return false;
  alert.dismiss();
  if (alert.visible() || alert.poll(true, 60000) || alert.visible()) return false;
  if (!alert.poll(true, 121000) || !alert.visible()) return false;  // reminder
  alert.dismiss();
  alert.poll(false, 130000);  // attention resolved: next one fires immediately
  return alert.poll(true, 131000);
}

constexpr bool wrapsMonotonicClock() {
  AlertTimer alert;
  alert.poll(true, UINT32_MAX - 999);
  alert.dismiss();
  return !alert.poll(true, 100000) && alert.poll(true, 119000);
}

static_assert(remindsEveryTwoMinutes(), "reminder cadence");
static_assert(wrapsMonotonicClock(), "monotonic wraparound");
