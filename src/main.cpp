#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("=================================");
  Serial.println("ESP32-S3 project started");
  Serial.println("Embedded Water Tank Controller");
  Serial.println("VERSION: SERIAL_TEST_003");
  Serial.println("=================================");
}

void loop() {
  Serial.println("Running SERIAL_TEST_003...");
  delay(1000);
}