#include <Arduino.h>

/*
  ==================================================
  Project : Embedded Water Tank Controller
  Step    : 4. Button to LED Control
  Version : BUTTON_LED_CONTROL_001
  Board   : ESP32-S3 DevKitC-1
  Framework : Arduino / PlatformIO
  ==================================================

  목적:
  - 버튼 입력을 읽어서 LED 출력을 제어한다.
  - GPIO 입력 → 조건 판단 → GPIO 출력 구조를 이해한다.

  회로:
  - Button: GPIO4 ─ 버튼 ─ GND
  - LED   : GPIO10 ─ 10kΩ 저항 ─ LED ─ GND

  INPUT_PULLUP 방식:
  - 버튼 안 누름 = HIGH
  - 버튼 누름   = LOW
*/

// 버튼 입력 핀
const int BUTTON_PIN = 4;

// LED 출력 핀
const int LED_PIN = 10;

// 이전 버튼 상태 저장용
int lastButtonState = HIGH;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 버튼은 내부 풀업 저항 사용
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // LED는 출력으로 사용
  pinMode(LED_PIN, OUTPUT);

  // 초기 LED OFF
  digitalWrite(LED_PIN, LOW);

  Serial.println("=================================");
  Serial.println("ESP32-S3 Button to LED Control");
  Serial.println("VERSION: BUTTON_LED_CONTROL_001");
  Serial.println("BUTTON_PIN = GPIO4");
  Serial.println("LED_PIN    = GPIO10");
  Serial.println("INPUT MODE = INPUT_PULLUP");
  Serial.println("=================================");
  Serial.println("Button released = LED OFF");
  Serial.println("Button pressed  = LED ON");
}

void loop() {
  // 현재 버튼 상태 읽기
  int buttonState = digitalRead(BUTTON_PIN);

  // 버튼이 눌린 상태: INPUT_PULLUP에서는 LOW
  if (buttonState == LOW) {
    digitalWrite(LED_PIN, HIGH);  // LED ON
  } else {
    digitalWrite(LED_PIN, LOW);   // LED OFF
  }

  // 상태가 바뀔 때만 로그 출력
  if (buttonState != lastButtonState) {
    delay(50);  // 간단한 디바운스

    buttonState = digitalRead(BUTTON_PIN);

    if (buttonState != lastButtonState) {
      lastButtonState = buttonState;

      if (buttonState == LOW) {
        Serial.println("BUTTON PRESSED  -> LED ON");
      } else {
        Serial.println("BUTTON RELEASED -> LED OFF");
      }
    }
  }
}