#include <Arduino.h>

const int LED_PIN = 10;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);

  Serial.println("=================================");
  Serial.println("ESP32-S3 GPIO Output Test");
  Serial.println("VERSION: LED_BLINK_001");
  Serial.println("LED_PIN = GPIO10");
  Serial.println("=================================");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("GPIO10 HIGH - LED ON");
  delay(2000);

  digitalWrite(LED_PIN, LOW);
  Serial.println("GPIO10 LOW - LED OFF");
  delay(2000);
}