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

// Shared SPI bus; dedicated SD CS + MISO (TFT has no MISO wire).
// Module silk this bench: 3V3 · CS · MOSI · CLK · MISO · GND (CLK = SCK).
static const int SD_MISO = 14;
static const int SD_CS = 13;

static const uint32_t UI_PERIOD_MS = 1000;
static const uint32_t LOG_PERIOD_MS = 60000;  // 60 s — raise toward 300000 for 5 min
static const char *LOG_PATH = "/climate.csv";

static const uint16_t COL_BG = ST77XX_BLACK;
static const uint16_t COL_LABEL = 0x8410;
static const uint16_t COL_TEMP = 0xFD20;
static const uint16_t COL_HUM = 0x07FF;
static const uint16_t COL_PRES = 0x07E0;
static const uint16_t COL_TITLE = ST77XX_WHITE;

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SHT4x sht4;
Adafruit_BMP3XX bmp;

static const int VALUE_SIZE = 3;
static const int VALUE_H = 8 * VALUE_SIZE;
static const int VALUE_PAD = 4;
static const int VALUE_X = 16;
static const int TEMP_Y = 88;
static const int HUM_Y = 168;
static const int PRES_Y = 248;

static uint32_t last_ui_ms = 0;
static uint32_t last_log_ms = 0;
static bool sd_ok = false;
SdFat sd;

static void drawValue(int y, uint16_t color, const char *text) {
  tft.fillRect(12, y - VALUE_PAD, 216, VALUE_H + 2 * VALUE_PAD, COL_BG);
  tft.setTextSize(VALUE_SIZE);
  tft.setTextColor(color);
  tft.setCursor(VALUE_X, y);
  tft.print(text);
}

static void drawChrome() {
  tft.fillScreen(COL_BG);
  tft.setTextWrap(false);

  tft.setTextColor(COL_TITLE);
  tft.setTextSize(2);
  tft.setCursor(16, 20);
  tft.print("Climate");

  tft.drawFastHLine(12, 44, 216, COL_LABEL);

  tft.setTextSize(1);
  tft.setTextColor(COL_LABEL);
  tft.setCursor(16, 64);
  tft.print("TEMPERATURE");
  tft.setCursor(16, 144);
  tft.print("HUMIDITY");
  tft.setCursor(16, 224);
  tft.print("PRESSURE");
}

static void drawValues(float temp_c, float humidity, float pressure_hpa) {
  char buf[24];
  snprintf(buf, sizeof(buf), "%.1f C", temp_c);
  drawValue(TEMP_Y, COL_TEMP, buf);
  snprintf(buf, sizeof(buf), "%.1f %%", humidity);
  drawValue(HUM_Y, COL_HUM, buf);
  snprintf(buf, sizeof(buf), "%.1f hPa", pressure_hpa);
  drawValue(PRES_Y, COL_PRES, buf);
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

static bool appendSample(uint32_t uptime_ms, float temp_c, float humidity,
                         float pressure_hpa) {
  FsFile f = sd.open(LOG_PATH, O_RDWR | O_CREAT | O_AT_END);
  if (!f) {
    return false;
  }
  f.print(uptime_ms);
  f.print(',');
  f.print(temp_c, 2);
  f.print(',');
  f.print(humidity, 1);
  f.print(',');
  f.println(pressure_hpa, 2);
  f.close();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("weather sd-log — phase 8");
  Serial.printf("I2C SDA=%d SCL=%d  TFT CS=%d  SD CS=%d MISO=%d\n", PIN_SDA,
                PIN_SCL_I2C, TFT_CS, SD_CS, SD_MISO);
  Serial.printf("UI ~1 Hz; CSV append every %lu s -> %s\n",
                (unsigned long)(LOG_PERIOD_MS / 1000UL), LOG_PATH);

  Wire.begin(PIN_SDA, PIN_SCL_I2C);
  Wire.setClock(100000);

  if (!sht4.begin(&Wire)) {
    Serial.println("SHT40 begin failed — halt");
    while (true) {
      delay(1000);
    }
  }
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  if (!bmp.begin_I2C(BMP390_ADDR, &Wire)) {
    Serial.println("BMP390 begin failed — halt");
    while (true) {
      delay(1000);
    }
  }
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);

  // Shared SPI: SCK/MOSI with TFT; MISO for SD only. SS left to device CS pins.
  SPI.begin(TFT_SCK, SD_MISO, TFT_MOSI, -1);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);  // deselect panel before card init
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  tft.init(240, 320);
  tft.setRotation(0);
  drawChrome();
  digitalWrite(TFT_CS, HIGH);

  // Cheap TF readers often need a slow first clock; raise later if stable.
  sd_ok = sd.begin(SdSpiConfig(SD_CS, SHARED_SPI, SD_SCK_MHZ(4), &SPI));
  if (!sd_ok) {
    Serial.printf("SD begin failed err=0x%02X data=0x%02X — check card/FAT/wiring\n",
                  sd.sdErrorCode(), sd.sdErrorData());
  } else if (!ensureLogHeader()) {
    Serial.println("SD header write failed");
    sd_ok = false;
  } else {
    Serial.println("SD ready");
  }

  Serial.println("UI live");
  last_ui_ms = 0;
  last_log_ms = 0;
}

void loop() {
  const uint32_t now = millis();

  if (now - last_ui_ms < UI_PERIOD_MS) {
    return;
  }
  last_ui_ms = now;

  sensors_event_t humidity, temp;
  if (!sht4.getEvent(&humidity, &temp)) {
    Serial.println("SHT40 read failed");
    return;
  }
  if (!bmp.performReading()) {
    Serial.println("BMP390 read failed");
    return;
  }

  const float pressure_hpa = bmp.pressure / 100.0f;
  drawValues(temp.temperature, humidity.relative_humidity, pressure_hpa);
  Serial.printf("temp_c=%.2f  humidity=%.1f  pressure_hpa=%.2f\n",
                temp.temperature, humidity.relative_humidity, pressure_hpa);

  if (sd_ok && (last_log_ms == 0 || now - last_log_ms >= LOG_PERIOD_MS)) {
    if (appendSample(now, temp.temperature, humidity.relative_humidity,
                     pressure_hpa)) {
      Serial.printf("sd append %s uptime_ms=%lu\n", LOG_PATH,
                    (unsigned long)now);
      last_log_ms = now;
    } else {
      Serial.println("sd append failed");
      sd_ok = false;
    }
  }
}
