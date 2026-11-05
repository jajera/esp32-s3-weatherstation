#include <Arduino.h>
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  WiFi.mode(WIFI_STA);  // so macAddress() returns the eFuse STA MAC
  delay(50);
  Serial.println();
  Serial.println("weather hello — phase 1");
  Serial.printf("MAC=%s\n", WiFi.macAddress().c_str());
  Serial.printf("chip=ESP32-S3 flash=%uMB psram=%u bytes\n",
                ESP.getFlashChipSize() / (1024u * 1024u),
                ESP.getPsramSize());
}

void loop() {
  Serial.printf("weather hello MAC=%s uptime_ms=%lu\n",
                WiFi.macAddress().c_str(),
                (unsigned long)millis());
  delay(1000);
}
