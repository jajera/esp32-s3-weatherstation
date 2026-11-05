#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP3XX.h>

// Lab pin map — docs/hardware.md
static const int PIN_SDA = 8;
static const int PIN_SCL = 18;
static const uint8_t BMP390_ADDR = 0x77;  // this bench (Phase 4 scan)

Adafruit_SHT4x sht4;
Adafruit_BMP3XX bmp;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("weather climate — phase 5");
  Serial.printf("SDA=GPIO%d SCL=GPIO%d SHT40~0x44 BMP390=0x%02X\n",
                PIN_SDA, PIN_SCL, BMP390_ADDR);

  Wire.begin(PIN_SDA, PIN_SCL);
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

  Serial.println("sensors ok — printing ~1 Hz");
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
  Serial.printf("temp_c=%.2f  humidity=%.1f  pressure_hpa=%.2f\n",
                temp.temperature, humidity.relative_humidity, pressure_hpa);
  delay(1000);
}
