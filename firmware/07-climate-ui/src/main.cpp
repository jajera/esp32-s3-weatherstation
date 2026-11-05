#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP3XX.h>

// Lab pin map — docs/hardware.md
static const int PIN_SDA = 8;
static const int PIN_SCL_I2C = 18;
static const uint8_t BMP390_ADDR = 0x77;  // this bench (Phase 4)

static const int TFT_SCK = 12;
static const int TFT_MOSI = 11;
static const int TFT_CS = 10;
static const int TFT_DC = 9;
static const int TFT_RST = 7;

static const uint16_t COL_BG = ST77XX_BLACK;
static const uint16_t COL_LABEL = 0x8410;   // grey
static const uint16_t COL_TEMP = 0xFD20;    // orange
static const uint16_t COL_HUM = 0x07FF;     // cyan
static const uint16_t COL_PRES = 0x07E0;    // green
static const uint16_t COL_TITLE = ST77XX_WHITE;

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_SHT4x sht4;
Adafruit_BMP3XX bmp;

// Built-in GFX font: 8 px tall * text size. Keep clear pad above the glyph
// so redraw erase cannot shave the top of size-3 digits.
static const int VALUE_SIZE = 3;
static const int VALUE_H = 8 * VALUE_SIZE;
static const int VALUE_PAD = 4;
static const int VALUE_X = 16;
static const int TEMP_Y = 88;
static const int HUM_Y = 168;
static const int PRES_Y = 248;

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

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("weather climate-ui — phase 7");
  Serial.printf("SDA=%d SCL=%d  SPI SCK=%d MOSI=%d CS=%d DC=%d RST=%d\n",
                PIN_SDA, PIN_SCL_I2C, TFT_SCK, TFT_MOSI, TFT_CS, TFT_DC, TFT_RST);

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

  SPI.begin(TFT_SCK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(0);
  drawChrome();
  Serial.println("UI live — ~1 Hz");
}

void loop() {
  sensors_event_t humidity, temp;
  if (!sht4.getEvent(&humidity, &temp)) {
    Serial.println("SHT40 read failed");
    delay(1000);
    return;
  }
  if (!bmp.performReading()) {
    Serial.println("BMP390 read failed");
    delay(1000);
    return;
  }

  const float pressure_hpa = bmp.pressure / 100.0f;
  drawValues(temp.temperature, humidity.relative_humidity, pressure_hpa);
  Serial.printf("temp_c=%.2f  humidity=%.1f  pressure_hpa=%.2f\n",
                temp.temperature, humidity.relative_humidity, pressure_hpa);
  delay(1000);
}
