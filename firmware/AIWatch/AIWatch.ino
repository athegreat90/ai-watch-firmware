/*
 * AI Watch for Waveshare ESP32-S3-Touch-AMOLED-1.8 V2
 * Arduino IDE 2.x / arduino-esp32 core 3.3.11
 * CO5300 (QSPI) + CST820 (Arduino_CST816x-compatible driver)
 *
 * Arduino libraries:
 *   GFX Library for Arduino 1.6.4 (Waveshare Arduino V2 bundled copy)
 *   Arduino_DriveBus (Waveshare Arduino V2 bundled copy)
 *   Adafruit XCA9554 (Waveshare Arduino V2 bundled copy)
 *   Adafruit BusIO (library manager)
 *   ArduinoJson 7.x (library manager)
 *
 * AI events come from the companion bridge/server.py running on a PC.
 * UI is drawn directly through Arduino_GFX -- no LVGL setup required.
 */
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <Arduino_DriveBus_Library.h>
#include <Adafruit_XCA9554.h>
#include <memory>
#include "pin_config.h"
#if __has_include("config.h")
#include "config.h"
#else
#include "config.example.h"
#define AIWATCH_OFFLINE_PREVIEW
#endif
#include "FocusTimer.h"
#include "AlertTimer.h"

#ifndef POLL_INTERVAL_MS
#define POLL_INTERVAL_MS 3000
#endif
#ifndef IDLE_MS
#define IDLE_MS (15UL * 60UL * 1000UL)  // idle character after 15 quiet minutes
#endif
static constexpr uint8_t MAX_SESSIONS = 8;
static constexpr uint32_t CELEBRATE_MS = 4000;
static constexpr uint8_t MEETING_BRIGHTNESS = 40;

static const uint16_t BG        = 0x0803; // dark almost black/navy
static const uint16_t CARD      = 0x10C7;
static const uint16_t WHITE_TXT = 0xF79E;
static const uint16_t MUTED     = 0x9CF3;
static const uint16_t BLUE      = 0x6C5F;
static const uint16_t GREEN     = 0x4E91;
static const uint16_t AMBER     = 0xFD0A;
static const uint16_t BTN_BLUE  = 0x1B30;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *gfx = new Arduino_CO5300(
    bus, GFX_NOT_DEFINED, 0, LCD_WIDTH, LCD_HEIGHT, 16, 0, 0, 0);

Adafruit_XCA9554 expander;
std::shared_ptr<Arduino_IIC_DriveBus> iicBus =
    std::make_shared<Arduino_HWIIC>(IIC_SDA, IIC_SCL, &Wire);

void touchInterrupt();
std::unique_ptr<Arduino_IIC> touch(new Arduino_CST816x(
    iicBus, CST816T_DEVICE_ADDRESS, DRIVEBUS_DEFAULT_VALUE,
    TP_INT, touchInterrupt));

struct Session {
  char id[81], agent[25], project[41], status[21], detail[67];
  unsigned long duration;
  double updated;
  bool finishedLong;
  unsigned long waiting;
};

struct Window {
  float used;
  uint32_t resetsIn;
};

struct Limit {
  bool valid;
  Window five, week;
};

struct Snapshot {
  Session sessions[MAX_SESSIONS];
  Limit claude, codex, antigravity;
  uint8_t count;
  unsigned total;
  int code;
  bool wifi;
};

static QueueHandle_t inbox;
static TaskHandle_t networkWorker;
static Snapshot shown{};
static FocusTimer focus;
static bool touchAvailable, touchPressed;
static uint8_t brightness = 165;
enum class Screen { Home, Sessions, Detail, Focus, Limits };
enum class Overlay { None, Alert, Celebrate, Idle };
static Screen screen = Screen::Home;
static int selected = 0, page = 0;
struct Acknowledgement {
  char id[81], agent[25];
  double updated;
};
static Acknowledgement acknowledged[MAX_SESSIONS]{};
static uint8_t nextAcknowledgement;
static bool repaint = true;
static AlertTimer alert;
static bool meetingManual;
static uint32_t lastActivity, celebrateUntil;
static char celebrateText[40];
static Overlay overlay = Overlay::None;
static uint32_t overlayPhase;
static uint8_t appliedBrightness = 165;

void drawDuration(const Session &session);
int fetch(Snapshot &next);

void touchInterrupt() {
  touch->IIC_Interrupt_Flag = true;
}

String clipped(const String &value, size_t limit) {
  if (value.length() <= limit) return value;
  if (limit <= 3) return value.substring(0, limit);
  return value.substring(0, limit - 3) + "...";
}

void label(int16_t x, int16_t y, const String &value, uint16_t color, uint8_t scale) {
  gfx->setTextSize(scale);
  gfx->setTextColor(color);
  gfx->setCursor(x, y);
  gfx->print(value);
}

uint16_t stateColor(const String &state) {
  if (state == "needs_attention") return AMBER;
  if (state == "working") return BLUE;
  if (state == "done") return GREEN;
  if (state == "error") return 0xF9C7;
  return MUTED;
}

const char *stateLabel(const String &state) {
  if (state == "needs_attention") return "Needs you";
  if (state == "working") return "Working";
  if (state == "done") return "Completed";
  if (state == "error") return "Error";
  if (state == "notice") return "Notification";
  return "Idle";
}

void button(int x, int y, int width, const String &title) {
  gfx->fillRoundRect(x, y, width, 46, 10, BTN_BLUE);
  label(x + 10, y + 15, title, WHITE_TXT, 2);
}

void drawTimer(uint32_t now) {
  unsigned long remaining = focus.seconds(now);
  char time[20];
  snprintf(time, sizeof(time), "%02lu:%02lu", remaining / 60, remaining % 60);
  gfx->fillRect(16, 300, 336, 72, BG);
  label(26, 324, remaining ? String("Focus  ") + time : "Focus complete",
        remaining ? WHITE_TXT : GREEN, 3);
}

void drawDuration(const Session &session) {
  gfx->fillRect(30, 251, 300, 19, CARD);
  label(30, 251, "Session " + String(session.duration / 60) + "m " +
        String(session.duration % 60) + "s", MUTED, 2);
}

bool meeting() { return meetingManual || focus.active(); }

bool isAcknowledged(const Session &s) {
  for (const auto &entry : acknowledged) {
    if (!strcmp(entry.id, s.id) && !strcmp(entry.agent, s.agent) &&
        entry.updated == s.updated) return true;
  }
  return false;
}

const Session *pendingAttention() {
  if (shown.code != 200) return nullptr;
  for (int i = 0; i < shown.count; ++i) {
    if (!strcmp(shown.sessions[i].status, "needs_attention") &&
        !isAcknowledged(shown.sessions[i])) return &shown.sessions[i];
  }
  return nullptr;
}

void drawRobot(int16_t cx, int16_t cy, uint16_t eye, bool blink) {
  gfx->drawLine(cx, cy - 46, cx, cy - 66, WHITE_TXT);
  gfx->fillCircle(cx, cy - 70, 6, AMBER);
  gfx->fillRoundRect(cx - 60, cy - 46, 120, 92, 20, WHITE_TXT);
  gfx->fillRoundRect(cx - 46, cy - 32, 92, 60, 14, BG);
  if (blink) {
    gfx->fillRect(cx - 28, cy - 4, 20, 4, eye);
    gfx->fillRect(cx + 8, cy - 4, 20, 4, eye);
  } else {
    gfx->fillCircle(cx - 18, cy - 2, 10, eye);
    gfx->fillCircle(cx + 18, cy - 2, 10, eye);
  }
  gfx->fillRoundRect(cx - 34, cy + 52, 68, 36, 10, WHITE_TXT);
}

void drawOverlay(uint32_t now) {
  gfx->fillScreen(BG);
  if (overlay == Overlay::Alert) {
    const Session *s = pendingAttention();
    const uint16_t edge = (now / 500) & 1 ? AMBER : BG;  // blinking border
    if (edge != BG) {  // filled shapes only: the CO5300 drops the pixel writes drawRoundRect uses
      gfx->fillRoundRect(8, 8, LCD_WIDTH - 16, LCD_HEIGHT - 16, 24, edge);
      gfx->fillRoundRect(14, 14, LCD_WIDTH - 28, LCD_HEIGHT - 28, 20, BG);
    }
    label(76, 70, "NEEDS YOU", AMBER, 4);
    if (s) {
      label(20, 150, clipped(s->project, 20), WHITE_TXT, 3);
      label(20, 200, s->agent, BLUE, 2);
      label(20, 235, clipped(s->detail, 28), MUTED, 2);
      label(20, 285, s->waiting < 60 ? "Waiting " + String(s->waiting) + "s" : "Waiting " + String(s->waiting / 60) + "m", MUTED, 2);
    }
    label(60, 380, "Tap to dismiss", MUTED, 2);
  } else if (overlay == Overlay::Celebrate) {
    static const uint16_t colors[4] = {GREEN, AMBER, BLUE, WHITE_TXT};
    uint32_t seed = now / 250;
    for (int i = 0; i < 30; ++i) {
      seed = seed * 1664525u + 1013904223u;
      const int16_t x = (seed >> 8) % LCD_WIDTH;
      seed = seed * 1664525u + 1013904223u;
      const int16_t y = (seed >> 8) % LCD_HEIGHT;
      gfx->fillRect(x, y, 8, 8, colors[i % 4]);
    }
    drawRobot(LCD_WIDTH / 2, 170, GREEN, false);
    label(64, 300, "Task done!", GREEN, 4);
    label(20, 350, celebrateText, WHITE_TXT, 2);
  } else if (overlay == Overlay::Idle) {
    // Moves between four spots every 30 s to avoid AMOLED burn-in.
    const uint32_t slot = (now / 30000) % 4;
    drawRobot((slot & 1) ? 220 : 150, (slot & 2) ? 250 : 190, BLUE, (now / 600) % 6 == 0);
  }
}

String resetText(uint32_t secs) {
  if (secs >= 86400) return "in " + String(secs / 86400) + "d" + String((secs % 86400) / 3600) + "h";
  if (secs >= 3600) return "in " + String(secs / 3600) + "h" + String((secs % 3600) / 60);
  return "in " + String(secs / 60) + "m";
}

// Rounded-rect bars, not circles: the CO5300 mangles the per-line writes of fillCircle/fillArc.
void drawLimitBar(int16_t y, const char *tag, const Window &window) {
  const float used = constrain(window.used, 0.0f, 100.0f);
  const uint16_t color = used >= 90 ? 0xF9C7 : (used >= 70 ? AMBER : BLUE);
  label(20, y, String(tag) + " " + String((int)(used + 0.5f)) + "%  " + resetText(window.resetsIn), WHITE_TXT, 2);
  gfx->fillRoundRect(20, y + 20, 328, 12, 5, CARD);
  int16_t fill = (int16_t)(328 * used / 100);
  if (used > 0.5f) gfx->fillRoundRect(20, y + 20, fill < 10 ? 10 : fill, 12, 5, color);
}

void drawLimitRow(int16_t y, const char *name, const Limit &limit) {
  label(20, y, name, BLUE, 2);
  drawLimitBar(y + 20, "5h", limit.five);
  drawLimitBar(y + 56, "7d", limit.week);
}

void drawLimits() {
  gfx->fillRect(0, 100, LCD_WIDTH, 286, BG);
  struct Row { const char *name; const Limit *limit; };
  const Row rows[3] = {{"Claude Code", &shown.claude}, {"Codex", &shown.codex}, {"Antigravity", &shown.antigravity}};
  int16_t y = 104;
  bool any = false;
  for (const Row &row : rows) {
    if (shown.code != 200 || !row.limit->valid) continue;
    drawLimitRow(y, row.name, *row.limit);
    y += 94;
    any = true;
  }
  if (!any) label(30, 142, "No limit data yet", MUTED, 2);
}

void drawScreen(uint32_t now) {
  static Overlay previousOverlay = Overlay::None;
  static Screen previousScreen = Screen::Home;
  // Overlays and the Limits screen paint outside the bands the normal screens clear.
  if ((previousOverlay != Overlay::None && overlay == Overlay::None) ||
      (previousScreen == Screen::Limits && screen != Screen::Limits)) gfx->fillScreen(BG);
  previousOverlay = overlay;
  previousScreen = screen;
  if (overlay != Overlay::None) {
    drawOverlay(now);
    return;
  }
  if (screen == Screen::Limits) gfx->fillRect(0, 299, LCD_WIDTH, 73, BG);
  gfx->fillRect(0, 0, LCD_WIDTH, 299, BG);
  gfx->fillRect(0, 372, LCD_WIDTH, LCD_HEIGHT - 372, BG);
  label(18, 17, "AI Watch", WHITE_TXT, 4);
  if (screen == Screen::Home) button(248, 12, 104, "Light");
  label(20, 58, shown.wifi ? "Wi-Fi online" : "Wi-Fi offline", shown.wifi ? GREEN : AMBER, 2);
  label(20, 80, shown.code == 200 ? "Backend online" :
        (shown.code == 401 ? "Backend: check token" : "Backend offline"),
        shown.code == 200 ? GREEN : AMBER, 2);

  if (meeting()) label(250, 58, "Meeting", pendingAttention() ? AMBER : MUTED, 2);

  if (screen == Screen::Limits) {
    drawLimits();
    button(15, 389, 105, "Home");
  }
  if (screen == Screen::Home || screen == Screen::Detail) {
    gfx->fillRoundRect(16, 111, 336, 179, 14, CARD);
    if (!shown.count || shown.code != 200) {
      label(30, 142, shown.code == 200 ? "No sessions" : "Disconnected", MUTED, 3);
    } else {
      const Session &s = shown.sessions[screen == Screen::Home ? 0 : selected];
      const bool seen = isAcknowledged(s);
      label(30, 131, stateLabel(s.status), stateColor(s.status), 3);
      label(30, 181, clipped(s.project, 25), WHITE_TXT, 2);
      label(30, 207, s.agent, BLUE, 2);
      label(30, 233, clipped(s.detail, 50), MUTED, 1);
      drawDuration(s);
      if (seen) label(30, 274, "Acknowledged", GREEN, 1);
    }
  }

  if (screen == Screen::Sessions) {
    gfx->fillRect(0, 299, LCD_WIDTH, 73, BG);
    label(20, 112, "Sessions  " + String(shown.total), WHITE_TXT, 2);
    if (shown.code == 200) {
      for (int i = page; i < shown.count && i < page + 4; ++i) {
        const Session &s = shown.sessions[i];
        int y = 146 + (i - page) * 48;
        gfx->fillRoundRect(16, y, 336, 43, 8, CARD);
        label(26, y + 6, clipped(s.project, 26), WHITE_TXT, 2);
        label(26, y + 27, String(s.agent) + " / " + stateLabel(s.status),
              stateColor(s.status), 1);
      }
    }
    button(15, 389, 105, "Home");
    button(130, 389, 105, "Next");
    button(245, 389, 108, "Refresh");
  } else if (screen == Screen::Home) {
    button(15, 389, 112, "Sessions");
    button(135, 389, 100, "Focus");
    button(245, 389, 108, "Limits");
  } else if (screen == Screen::Detail) {
    button(15, 389, 112, "Back");
    button(135, 389, 218, "Acknowledge");
  } else if (screen == Screen::Focus) {
    label(20, 128, "25-minute focus timer", WHITE_TXT, 2);
    button(16, 160, 336, String("Meeting: ") + (meeting() ? "on" : "off"));
    button(15, 389, 105, "Home");
    button(130, 389, 105, focus.active() ? "Pause" : (focus.seconds(now) < 1500 && focus.seconds(now) ? "Resume" : "Start"));
    button(245, 389, 108, "Reset");
  }
  if (screen == Screen::Home || screen == Screen::Focus) drawTimer(now);
  else if (screen == Screen::Detail) gfx->fillRect(0, 299, LCD_WIDTH, 73, BG);
}

static void parseLimit(JsonObjectConst row, Limit &out) {
  JsonObjectConst five = row["five_hour"], week = row["weekly"];
  if (five.isNull() || week.isNull()) return;
  out.five = {five["used_pct"] | 0.0f, five["resets_in"] | 0U};
  out.week = {week["used_pct"] | 0.0f, week["resets_in"] | 0U};
  out.valid = true;
}

int fetch(Snapshot &next) {
  HTTPClient http;
  http.setConnectTimeout(1200);
  http.setTimeout(1500);
  if (!http.begin(BRIDGE_URL)) return -1;
  http.addHeader("X-Monitor-Key", MONITOR_TOKEN);
  int code = http.GET();
  if (code == 200) {
    JsonDocument doc;
    int length = http.getSize();
    if (length < 1 || length > 12288 ||
        deserializeJson(doc, http.getStream(), DeserializationOption::NestingLimit(6))) {
      code = -2;
    } else {
      JsonArrayConst sessions = doc["sessions"].as<JsonArrayConst>();
      if (sessions.isNull() || sessions.size() > MAX_SESSIONS) code = -2;
      for (JsonObjectConst row : sessions) {
        if (code != 200) break;
        const char *id = row["session_id"] | "";
        const char *agent = row["agent"] | "";
        if (!*id || !*agent || !row["status"].is<const char *>()) {
          code = -2;
          break;
        }
        Session &s = next.sessions[next.count++];
        strlcpy(s.id, id, sizeof(s.id));
        strlcpy(s.agent, agent, sizeof(s.agent));
        strlcpy(s.project, row["project"] | "Project", sizeof(s.project));
        strlcpy(s.status, row["status"] | "idle", sizeof(s.status));
        strlcpy(s.detail, row["detail"] | "", sizeof(s.detail));
        s.duration = row["duration"] | 0UL;
        s.updated = row["updated"] | 0.0;
        s.finishedLong = row["finished_long"] | false;
        s.waiting = row["waiting"] | 0UL;
      }
      parseLimit(doc["limits"]["Claude Code"], next.claude);
      parseLimit(doc["limits"]["Codex"], next.codex);
      parseLimit(doc["limits"]["Antigravity"], next.antigravity);
      next.total = doc["total_count"] | next.count;
    }
  }
  http.end();
  if (code != 200) { next.count = next.total = 0; next.claude.valid = next.codex.valid = next.antigravity.valid = false; }
  return code;
}

void monitorNetwork(void *) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int previous = -99;
  uint32_t lastRetry = millis();
  for (;;) {
    Snapshot next{};
    next.wifi = WiFi.status() == WL_CONNECTED;
    if (!next.wifi && millis() - lastRetry >= 10000) {
      WiFi.reconnect();
      lastRetry = millis();
    }
    next.code = next.wifi ? fetch(next) : 0;
    if (next.code != previous) {
      Serial.printf("Backend status: %d (0=Wi-Fi offline, -2=invalid JSON)\n", next.code);
      previous = next.code;
    }
    xQueueOverwrite(inbox, &next);
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(POLL_INTERVAL_MS));
  }
}

void pollTouch(uint32_t now) {
  if (!touchAvailable) return;
  using TouchValue = Arduino_IIC_Touch::Value_Information;
  if (touch->IIC_Read_Device_Value(TouchValue::TOUCH_FINGER_NUMBER) <= 0) {
    touchPressed = false;
    return;
  }
  if (touchPressed) return;
  touchPressed = true;
  int x = touch->IIC_Read_Device_Value(TouchValue::TOUCH_COORDINATE_X);
  int y = touch->IIC_Read_Device_Value(TouchValue::TOUCH_COORDINATE_Y);
  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return;
  lastActivity = now;
  if (overlay != Overlay::None) {  // first touch only wakes/dismisses; it never presses a hidden button
    if (overlay == Overlay::Alert) alert.dismiss();
    else if (overlay == Overlay::Celebrate) celebrateUntil = 0;
    Serial.printf("[%lu] Overlay dismissed by touch\n", now);
    repaint = true;
    return;
  }
  if (screen == Screen::Home && y >= 12 && y < 58 && x >= 248) {
    brightness = brightness >= 215 ? 65 : brightness + 50;  // applied by loop()
    return;
  }
  if (screen == Screen::Sessions && y >= 146 && y < 338) {
    int row = page + (y - 146) / 48;
    if (row < shown.count) {
      selected = row;
      screen = Screen::Detail;
    }
  } else if (screen == Screen::Focus && y >= 160 && y < 206) {
    meetingManual = !meetingManual;
  } else if (y >= 389 && y < 435) {
    switch (screen) {
      case Screen::Home:
        if (x < 130) { screen = Screen::Sessions; page = 0; }
        else if (x < 240) screen = Screen::Focus;
        else screen = Screen::Limits;
        break;
      case Screen::Sessions:
        if (x < 125) screen = Screen::Home;
        else if (x < 240) page = page + 4 < shown.count ? page + 4 : 0;
        else if (networkWorker) xTaskNotifyGive(networkWorker);
        break;
      case Screen::Detail:
        if (x < 130) screen = Screen::Sessions;
        else if (selected < shown.count) {
          const Session &session = shown.sessions[selected];
          auto &entry = acknowledged[nextAcknowledgement];
          strlcpy(entry.id, session.id, sizeof(entry.id));
          strlcpy(entry.agent, session.agent, sizeof(entry.agent));
          entry.updated = session.updated;
          nextAcknowledgement = (nextAcknowledgement + 1) % MAX_SESSIONS;
        }
        break;
      case Screen::Focus:
        if (x < 125) screen = Screen::Home;
        else if (x < 240) focus.toggle(now);
        else focus.reset();
        break;
      case Screen::Limits:
        if (x < 125) screen = Screen::Home;
        break;
    }
  }
  repaint = true;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("AI Watch 1.1.0 | Waveshare 1.8 V2");

  Wire.begin(IIC_SDA, IIC_SCL);
  Wire.setTimeOut(25);
  if (!expander.begin(0x20)) {
    Serial.println("XCA9554 expander not found; check V2 board and I2C");
    while (true) delay(1000);
  }

  // Match Waveshare's V2 02_Drawing_board power/IO reset sequence.
  for (uint8_t pin = 0; pin <= 2; ++pin) expander.pinMode(pin, OUTPUT);
  for (uint8_t pin = 0; pin <= 2; ++pin) expander.digitalWrite(pin, LOW);
  delay(20);
  for (uint8_t pin = 0; pin <= 2; ++pin) expander.digitalWrite(pin, HIGH);

  for (int attempt = 1; attempt <= 3 && !touchAvailable; ++attempt) {
    touchAvailable = touch->begin();
    if (!touchAvailable) {
      Serial.printf("Touch initialization attempt %d failed\n", attempt);
      if (attempt < 3) delay(2000);
    }
  }
  if (touchAvailable) {
    touch->IIC_Write_Device_State(
        touch->Arduino_IIC_Touch::Device::TOUCH_DEVICE_INTERRUPT_MODE,
        touch->Arduino_IIC_Touch::Device_Mode::TOUCH_DEVICE_INTERRUPT_PERIODIC);
    Serial.println("CST820-compatible touch ready");
  } else {
    Serial.println("Touch unavailable; display/network will still work");
  }

  if (!gfx->begin()) {
    Serial.println("CO5300 initialization failed. Verify board revision/libraries.");
    while (true) delay(1000);
  }
  Serial.println("CO5300 display initialized");
  gfx->setBrightness(165); // lower than maximum to reduce AMOLED wear
  gfx->setTextWrap(false);
  lastActivity = millis();
#ifdef AIWATCH_OFFLINE_PREVIEW
  Serial.println("Offline preview: create config.h to enable Wi-Fi");
#else
  inbox = xQueueCreate(1, sizeof(Snapshot));
  if (!inbox || xTaskCreate(monitorNetwork, "monitor", 8192, nullptr, 1, &networkWorker) != pdPASS) {
    Serial.println("Network worker unavailable");
  }
#endif
}

static bool limitChanged(const Limit &a, const Limit &b) {
  return a.valid != b.valid || (a.valid && (fabsf(a.five.used - b.five.used) >= 0.5f ||
         fabsf(a.week.used - b.week.used) >= 0.5f || a.five.resetsIn / 60 != b.five.resetsIn / 60 ||
         a.week.resetsIn / 60 != b.week.resetsIn / 60));
}

void loop() {
  uint32_t now = millis();
  const bool wasRunning = focus.active();
  focus.tick(now);
  if (wasRunning != focus.active()) repaint = true;  // meeting badge/brightness follow the timer
  pollTouch(now);
  Snapshot next{};
  if (inbox && xQueueReceive(inbox, &next, 0) == pdTRUE) {
    bool changed = next.code != shown.code || next.wifi != shown.wifi ||
                   next.count != shown.count || next.total != shown.total;
    for (int i = 0; i < next.count && !changed; ++i) {
      const Session &before = shown.sessions[i];
      const Session &after = next.sessions[i];
      changed = before.updated != after.updated || strcmp(before.id, after.id) ||
                strcmp(before.agent, after.agent);
    }
    const bool sessionsChanged = changed;  // limit countdowns must not count as user activity
    if (limitChanged(shown.claude, next.claude) || limitChanged(shown.codex, next.codex) ||
        limitChanged(shown.antigravity, next.antigravity)) {
      if (screen == Screen::Limits) changed = true;
      Serial.printf("[%lu] Limits: claude=%d codex=%d antigravity=%d\n", now, next.claude.valid, next.codex.valid,
                    next.antigravity.valid);
    }
    for (int i = 0; i < next.count; ++i) {
      const Session &after = next.sessions[i];
      for (int j = 0; j < shown.count; ++j) {
        const Session &before = shown.sessions[j];
        if (strcmp(before.id, after.id) || strcmp(before.agent, after.agent)) continue;
        if (!strcmp(before.status, "working") && !strcmp(after.status, "done") &&
            after.finishedLong && !meeting()) {
          celebrateUntil = now + CELEBRATE_MS;
          snprintf(celebrateText, sizeof(celebrateText), "%.18s  %lum", after.project, after.duration / 60);
          Serial.printf("[%lu] Celebrate: %s %lum\n", now, after.project, after.duration / 60);
        }
        break;
      }
    }
    if (sessionsChanged) lastActivity = now;
    if (changed && screen == Screen::Detail && selected < shown.count) {
      const Session &old = shown.sessions[selected];
      int match = 0;
      while (match < next.count && (strcmp(old.id, next.sessions[match].id) ||
                                    strcmp(old.agent, next.sessions[match].agent))) ++match;
      if (match == next.count) screen = Screen::Sessions;
      else selected = match;
    }
    const int visible = screen == Screen::Home ? 0 : selected;
    if (!changed && next.code == 200 && overlay == Overlay::None &&
        (screen == Screen::Home || screen == Screen::Detail) &&
        visible < next.count && shown.sessions[visible].duration != next.sessions[visible].duration) {
      drawDuration(next.sessions[visible]);
    }
    shown = next;
    if (page >= shown.count) page = 0;
    repaint = repaint || changed;
  }
  const Session *pending = pendingAttention();
  if (alert.poll(pending && !meeting(), now)) {
    lastActivity = now;
    Serial.printf("[%lu] Alert: show %s / %s\n", now, pending->agent, pending->project);
  }
  Overlay want = Overlay::None;
  if (celebrateUntil && (int32_t)(celebrateUntil - now) > 0) want = Overlay::Celebrate;
  else if (alert.visible() && !meeting()) want = Overlay::Alert;
  else if (now - lastActivity >= IDLE_MS && !pending) want = Overlay::Idle;
  if (celebrateUntil && want != Overlay::Celebrate) celebrateUntil = 0;
  const uint32_t phase = want == Overlay::Alert ? now / 500 : want == Overlay::Celebrate ? now / 250 :
                         want == Overlay::Idle ? now / 600 : 0;
  if (want != overlay || phase != overlayPhase) {
    if (want != overlay) Serial.printf("[%lu] Overlay: %d -> %d (1=alert 2=celebrate 3=idle)\n", now, (int)overlay, (int)want);
    overlay = want;
    overlayPhase = phase;
    repaint = true;
  }
  static bool wasMeeting = false;
  if (meeting() != wasMeeting) {
    wasMeeting = meeting();
    Serial.printf("[%lu] Meeting: %s\n", now, wasMeeting ? "on" : "off");
  }
  const uint8_t level = wasMeeting ? MEETING_BRIGHTNESS : brightness;
  if (level != appliedBrightness) {
    gfx->setBrightness(level);
    appliedBrightness = level;
  }
  static Screen loggedScreen = Screen::Home;
  if (screen != loggedScreen) {
    Serial.printf("[%lu] Screen: %d (0=home 1=sessions 2=detail 3=focus 4=limits)\n", now, (int)screen);
    loggedScreen = screen;
  }
  static uint32_t previous = UINT32_MAX;
  uint32_t seconds = focus.seconds(now);
  if (repaint) {
    drawScreen(now);
    repaint = false;
  } else if (seconds != previous && overlay == Overlay::None &&
             (screen == Screen::Home || screen == Screen::Focus)) {
    drawTimer(now);
  }
  previous = seconds;
  delay(25);
}
