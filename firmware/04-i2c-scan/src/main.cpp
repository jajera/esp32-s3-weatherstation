#include <Arduino.h>
#include <Wire.h>

// Lab pin map — docs/hardware.md
static const int PIN_SDA = 8;
static const int PIN_SCL = 18;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("weather i2c-scan — phase 4");
  Serial.printf("SDA=GPIO%d SCL=GPIO%d expect=SHT40~0x44 BMP390~0x76/0x77\n",
                PIN_SDA, PIN_SCL);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
}

void loop() {
  uint8_t found = 0;
  Serial.println("--- i2c scan ---");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("found 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("found none");
  } else {
    Serial.printf("devices=%u\n", found);
  }
  delay(2000);
}
