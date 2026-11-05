#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SdFat.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP3XX.h>

// Lab pin map — docs/hardware.md
static const int PIN_SDA = 8;
static const int PIN_SCL_I2C = 18;
static const uint8_t BMP390_ADDR = 0x77;

static const int TFT_SCK = 12;
static const int TFT_MOSI = 11;
static const int TFT_CS = 10;
static const int TFT_DC = 9;
static const int TFT_RST = 7;

static const int SD_MISO = 14;
static const int SD_CS = 13;

// Phase 9 controls — tact to GND (pull-up), LEDs via resistor to GND
static const int PIN_BTN_VIEW = 5;
static const int PIN_LED_OK = 4;
static const int PIN_LED_FAULT = 6;

static const uint32_t UI_PERIOD_MS = 1000;
static const uint32_t LOG_PERIOD_MS = 60000;
static const uint32_t BTN_DEBOUNCE_MS = 40;
static const uint32_t LED_OK_MS = 500;
static const char *LOG_PATH = "/climate.csv";

static const uint16_t COL_BG = ST77XX_BLACK;
static const uint16_t COL_LABEL = 0x8410;
static const uint16_t COL_TEMP = 0xFD20;
static const uint16_t COL_HUM = 0x07FF;
static const uint16_t COL_PRES = 0x07E0;
static const uint16_t COL_TITLE = ST77XX_WHITE;
static const uint16_t COL_WARN = 0xF800;

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SHT4x sht4;
Adafruit_BMP3XX bmp;
SdFat sd;

enum View : uint8_t { VIEW_NOW = 0, VIEW_SESSION = 1, VIEW_SYSTEM = 2, VIEW_COUNT = 3 };

static View view = VIEW_NOW;
static bool view_dirty = true;
static bool sd_ok = false;
static bool sensor_fault = false;
static uint32_t last_ui_ms = 0;
static uint32_t last_log_ms = 0;
static uint32_t last_append_ms = 0;
static uint32_t led_ok_until = 0;

static bool btn_stable = true;  // pull-up idle high
static bool btn_last_raw = true;
static uint32_t btn_edge_ms = 0;

struct Sample {
  float temp_c;
  float humidity;
  float pressure_hpa;
};

static Sample live = {NAN, NAN, NAN};
static Sample sess_min = {NAN, NAN, NAN};
static Sample sess_max = {NAN, NAN, NAN};
static bool sess_init = false;

// Last painted strings — skip SPI when unchanged
static char painted_now_t[20] = "";
static char painted_now_h[20] = "";
static char painted_now_p[20] = "";
static char painted_sess_t[40] = "";
static char painted_sess_h[40] = "";
static char painted_sess_p[40] = "";
static char painted_sys_sd[16] = "";
static char painted_sys_sens[16] = "";
static char painted_sys_age[24] = "";
static char painted_sys_up[28] = "";

static void setFaultLed(bool on) { digitalWrite(PIN_LED_FAULT, on ? HIGH : LOW); }
static void pulseOkLed() {
  digitalWrite(PIN_LED_OK, HIGH);
  led_ok_until = millis() + LED_OK_MS;
}

static void updateSession(const Sample &s) {
  if (!sess_init) {
    sess_min = sess_max = s;
    sess_init = true;
    return;
  }
  if (s.temp_c < sess_min.temp_c) sess_min.temp_c = s.temp_c;
  if (s.temp_c > sess_max.temp_c) sess_max.temp_c = s.temp_c;
  if (s.humidity < sess_min.humidity) sess_min.humidity = s.humidity;
  if (s.humidity > sess_max.humidity) sess_max.humidity = s.humidity;
  if (s.pressure_hpa < sess_min.pressure_hpa) sess_min.pressure_hpa = s.pressure_hpa;
  if (s.pressure_hpa > sess_max.pressure_hpa) sess_max.pressure_hpa = s.pressure_hpa;
}

// Opaque text: paint glyph + background so we never fillScreen on refresh.
static void paintField(int x, int y, uint8_t size, uint16_t fg, const char *text,
                       int width_chars, char *cache, size_t cache_len) {
  char buf[48];
  if (width_chars >= (int)sizeof(buf)) {
    width_chars = (int)sizeof(buf) - 1;
  }
  snprintf(buf, sizeof(buf), "%-*s", width_chars, text);
  if (cache != nullptr && cache_len > 0 && strncmp(cache, buf, cache_len) == 0) {
    return;
  }
  tft.setTextWrap(false);
  tft.setTextSize(size);
  tft.setTextColor(fg, COL_BG);
  tft.setCursor(x, y);
  tft.print(buf);
  if (cache != nullptr && cache_len > 0) {
    strncpy(cache, buf, cache_len - 1);
    cache[cache_len - 1] = '\0';
  }
}

static void clearPaintCache() {
  painted_now_t[0] = painted_now_h[0] = painted_now_p[0] = '\0';
  painted_sess_t[0] = painted_sess_h[0] = painted_sess_p[0] = '\0';
  painted_sys_sd[0] = painted_sys_sens[0] = '\0';
  painted_sys_age[0] = painted_sys_up[0] = '\0';
}

static void drawTitleChrome(const char *title) {
  tft.fillScreen(COL_BG);
  clearPaintCache();
  tft.setTextWrap(false);
  tft.setTextColor(COL_TITLE, COL_BG);
  tft.setTextSize(2);
  tft.setCursor(16, 20);
  tft.print(title);
  tft.drawFastHLine(12, 44, 216, COL_LABEL);
}

static void drawNowChrome() {
  drawTitleChrome("Now");
  tft.setTextSize(1);
  tft.setTextColor(COL_LABEL, COL_BG);
  tft.setCursor(16, 64);
  tft.print("TEMPERATURE");
  tft.setCursor(16, 144);
  tft.print("HUMIDITY");
  tft.setCursor(16, 224);
  tft.print("PRESSURE");
}

static void drawNowValues(const Sample &s) {
  if (isnan(s.temp_c)) {
    return;
  }
  char buf[20];
  snprintf(buf, sizeof(buf), "%.1f C", s.temp_c);
  paintField(16, 92, 3, COL_TEMP, buf, 12, painted_now_t, sizeof(painted_now_t));

  snprintf(buf, sizeof(buf), "%.1f %%", s.humidity);
  paintField(16, 172, 3, COL_HUM, buf, 12, painted_now_h, sizeof(painted_now_h));

  snprintf(buf, sizeof(buf), "%.1f hPa", s.pressure_hpa);
  paintField(16, 252, 3, COL_PRES, buf, 14, painted_now_p, sizeof(painted_now_p));
}

static void drawSessionChrome() {
  drawTitleChrome("Session");
  tft.setTextSize(1);
  tft.setTextColor(COL_LABEL, COL_BG);
  tft.setCursor(16, 60);
  tft.print("since boot  min / now / max");
  tft.setCursor(16, 80);
  tft.print("TEMPERATURE C");
  tft.setCursor(16, 150);
  tft.print("HUMIDITY %");
  tft.setCursor(16, 220);
  tft.print("PRESSURE hPa");
}

static void drawSessionValues(const Sample &now) {
  char buf[40];
  if (!sess_init) {
    paintField(16, 94, 2, COL_TEMP, "--", 22, painted_sess_t, sizeof(painted_sess_t));
    paintField(16, 164, 2, COL_HUM, "--", 22, painted_sess_h, sizeof(painted_sess_h));
    paintField(16, 234, 2, COL_PRES, "--", 22, painted_sess_p, sizeof(painted_sess_p));
    return;
  }
  snprintf(buf, sizeof(buf), "%.1f  %.1f  %.1f", sess_min.temp_c, now.temp_c,
           sess_max.temp_c);
  paintField(16, 94, 2, COL_TEMP, buf, 22, painted_sess_t, sizeof(painted_sess_t));

  snprintf(buf, sizeof(buf), "%.1f  %.1f  %.1f", sess_min.humidity, now.humidity,
           sess_max.humidity);
  paintField(16, 164, 2, COL_HUM, buf, 22, painted_sess_h, sizeof(painted_sess_h));

  snprintf(buf, sizeof(buf), "%.1f  %.1f  %.1f", sess_min.pressure_hpa,
           now.pressure_hpa, sess_max.pressure_hpa);
  paintField(16, 234, 2, COL_PRES, buf, 22, painted_sess_p, sizeof(painted_sess_p));
}

static void drawSystemChrome() {
  drawTitleChrome("System");
  tft.setTextSize(1);
  tft.setTextColor(COL_LABEL, COL_BG);
  tft.setCursor(16, 160);
  tft.print("LAST APPEND");
  tft.setCursor(16, 230);
  tft.print("UPTIME / LOG INTERVAL");
}

static void drawSystemValues(uint32_t now_ms) {
  char buf[48];
  const char *sd_txt = sd_ok ? "SD ok" : "SD FAIL";
  paintField(16, 70, 2, sd_ok ? COL_PRES : COL_WARN, sd_txt, 12, painted_sys_sd,
             sizeof(painted_sys_sd));

  const char *sens_txt = sensor_fault ? "SENSOR FAIL" : "sensors ok";
  paintField(16, 110, 2, sensor_fault ? COL_WARN : COL_TITLE, sens_txt, 14,
             painted_sys_sens, sizeof(painted_sys_sens));

  if (last_append_ms == 0) {
    snprintf(buf, sizeof(buf), "none yet");
  } else {
    const uint32_t age_s = (now_ms - last_append_ms) / 1000UL;
    snprintf(buf, sizeof(buf), "%lu s ago", (unsigned long)age_s);
  }
  paintField(16, 178, 2, COL_TITLE, buf, 16, painted_sys_age, sizeof(painted_sys_age));

  snprintf(buf, sizeof(buf), "%lu s / %lu s", (unsigned long)(now_ms / 1000UL),
           (unsigned long)(LOG_PERIOD_MS / 1000UL));
  paintField(16, 248, 2, COL_TITLE, buf, 18, painted_sys_up, sizeof(painted_sys_up));
}

static void showView(uint32_t now_ms) {
  switch (view) {
    case VIEW_NOW:
      drawNowChrome();
      drawNowValues(live);
      break;
    case VIEW_SESSION:
      drawSessionChrome();
      drawSessionValues(live);
      break;
    case VIEW_SYSTEM:
      drawSystemChrome();
      drawSystemValues(now_ms);
      break;
    default:
      break;
  }
  view_dirty = false;
}

static void refreshView(uint32_t now_ms) {
  // Values only — never fillScreen here
  switch (view) {
    case VIEW_NOW:
      drawNowValues(live);
      break;
    case VIEW_SESSION:
      drawSessionValues(live);
      break;
    case VIEW_SYSTEM:
      drawSystemValues(now_ms);
      break;
    default:
      break;
  }
}

static void pollButton(uint32_t now_ms) {
  const bool raw = digitalRead(PIN_BTN_VIEW) == HIGH;
  if (raw != btn_last_raw) {
    btn_last_raw = raw;
    btn_edge_ms = now_ms;
  }
  if ((now_ms - btn_edge_ms) < BTN_DEBOUNCE_MS) {
    return;
  }
  if (raw == btn_stable) {
    return;
  }
  btn_stable = raw;
  // falling edge: released was high, now low (pressed to GND)
  if (!btn_stable) {
    view = static_cast<View>((static_cast<uint8_t>(view) + 1) % VIEW_COUNT);
    view_dirty = true;
    Serial.printf("view=%u\n", static_cast<unsigned>(view));
  }
}

static bool ensureLogHeader() {
  if (sd.exists(LOG_PATH)) {
    return true;
  }
  FsFile f = sd.open(LOG_PATH, O_RDWR | O_CREAT | O_TRUNC);
  if (!f) {
    return false;
  }
  f.println("uptime_ms,temp_c,humidity,pressure_hpa");
  f.close();
  return true;
}

static bool appendSample(uint32_t uptime_ms, const Sample &s) {
  FsFile f = sd.open(LOG_PATH, O_RDWR | O_CREAT | O_AT_END);
  if (!f) {
    return false;
  }
  f.print(uptime_ms);
  f.print(',');
  f.print(s.temp_c, 2);
  f.print(',');
  f.print(s.humidity, 1);
  f.print(',');
  f.println(s.pressure_hpa, 2);
  f.close();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("weather views — phase 9");
  Serial.printf("BTN=%d LED_OK=%d LED_FAULT=%d\n", PIN_BTN_VIEW, PIN_LED_OK,
                PIN_LED_FAULT);
  Serial.printf("I2C SDA=%d SCL=%d  TFT CS=%d  SD CS=%d MISO=%d\n", PIN_SDA,
                PIN_SCL_I2C, TFT_CS, SD_CS, SD_MISO);

  pinMode(PIN_BTN_VIEW, INPUT_PULLUP);
  pinMode(PIN_LED_OK, OUTPUT);
  pinMode(PIN_LED_FAULT, OUTPUT);
  digitalWrite(PIN_LED_OK, LOW);
  digitalWrite(PIN_LED_FAULT, LOW);

  // Boot self-test — both LEDs should light if wiring/polarity is right
  Serial.println("LED self-test: green then red");
  digitalWrite(PIN_LED_OK, HIGH);
  delay(800);
  digitalWrite(PIN_LED_OK, LOW);
  digitalWrite(PIN_LED_FAULT, HIGH);
  delay(800);
  digitalWrite(PIN_LED_FAULT, LOW);

  Wire.begin(PIN_SDA, PIN_SCL_I2C);
  Wire.setClock(100000);

  if (!sht4.begin(&Wire)) {
    Serial.println("SHT40 begin failed — halt");
    setFaultLed(true);
    while (true) {
      delay(1000);
    }
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  if (!bmp.begin_I2C(BMP390_ADDR, &Wire)) {
    Serial.println("BMP390 begin failed — halt");
    setFaultLed(true);
    while (true) {
      delay(1000);
    }
  }
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  SPI.begin(TFT_SCK, SD_MISO, TFT_MOSI, -1);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  tft.init(240, 320);
  tft.setRotation(0);
  tft.setSPISpeed(40000000);
  digitalWrite(TFT_CS, HIGH);

  sd_ok = sd.begin(SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(4), &SPI));
  if (!sd_ok) {
    Serial.printf("SD begin failed err=0x%02X data=0x%02X\n", sd.sdErrorCode(),
                  sd.sdErrorData());
    setFaultLed(true);
  } else if (!ensureLogHeader()) {
    Serial.println("SD header write failed");
    sd_ok = false;
    setFaultLed(true);
  } else {
    Serial.println("SD ready");
  }

  view = VIEW_NOW;
  view_dirty = true;
  showView(millis());
  Serial.println("UI live — BTN cycles Now/Session/System (partial redraw)");
}

void loop() {
  const uint32_t now = millis();
  pollButton(now);

  if (led_ok_until && now >= led_ok_until) {
    digitalWrite(PIN_LED_OK, LOW);
    led_ok_until = 0;
  }

  if (view_dirty) {
    showView(now);
  }

  if (now - last_ui_ms < UI_PERIOD_MS) {
    return;
  }
  last_ui_ms = now;

  sensors_event_t humidity, temp;
  if (!sht4.getEvent(&humidity, &temp) || !bmp.performReading()) {
    Serial.println("sensor read failed");
    sensor_fault = true;
    setFaultLed(true);
    if (view == VIEW_SYSTEM) {
      refreshView(now);
    }
    return;
  }
  sensor_fault = false;
  if (sd_ok) {
    setFaultLed(false);
  }

  live.temp_c = temp.temperature;
  live.humidity = humidity.relative_humidity;
  live.pressure_hpa = bmp.pressure / 100.0f;
  updateSession(live);

  refreshView(now);
  Serial.printf("view=%u temp_c=%.2f humidity=%.1f pressure_hpa=%.2f\n",
                static_cast<unsigned>(view), live.temp_c, live.humidity,
                live.pressure_hpa);

  if (sd_ok && (last_log_ms == 0 || now - last_log_ms >= LOG_PERIOD_MS)) {
    if (appendSample(now, live)) {
      Serial.printf("sd append %s uptime_ms=%lu\n", LOG_PATH,
                    (unsigned long)now);
      last_log_ms = now;
      last_append_ms = now;
      pulseOkLed();
      if (view == VIEW_SYSTEM) {
        refreshView(now);
      }
    } else {
      Serial.println("sd append failed");
      sd_ok = false;
      setFaultLed(true);
      if (view == VIEW_SYSTEM) {
        refreshView(now);
      }
    }
  }
}
