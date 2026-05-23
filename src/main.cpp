#include <Arduino.h>

/*
  ==================================================
  Project : Embedded Water Tank Controller
  Step    : 5. millis() Timer Test
  Version : MILLIS_TIMER_001
  Board   : ESP32-S3 DevKitC-1
  Framework : Arduino / PlatformIO
  ==================================================

  목적:
  - delay() 없이 LED를 2초 간격으로 깜빡인다.
  - millis()를 사용하여 non-blocking timer 구조를 이해한다.
  - 이후 Pump Delay Logic, Emergency Stop Logic의 기초로 사용한다.
*/

// LED 출력 핀
const int LED_PIN = 10;

// LED 상태 저장 변수
// false = OFF, true = ON
bool ledState = false;

// 마지막으로 LED 상태가 바뀐 시간
unsigned long previousMillis = 0;

// LED 상태를 바꿀 시간 간격
// 2000ms = 2초
const unsigned long interval = 2000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);

  // 초기 LED 상태는 OFF
  digitalWrite(LED_PIN, LOW);

  Serial.println("=================================");
  Serial.println("ESP32-S3 millis() Timer Test");
  Serial.println("VERSION: MILLIS_TIMER_001");
  Serial.println("LED_PIN = GPIO10");
  Serial.println("INTERVAL = 2000ms");
  Serial.println("=================================");
  Serial.println("delay() 없이 millis()로 LED를 제어합니다.");
}

void loop() {
  // 현재 보드가 실행된 시간을 가져온다.
  unsigned long currentMillis = millis();

  // 현재 시간 - 마지막 변경 시간이 interval 이상이면 실행
  if (currentMillis - previousMillis >= interval) {
    // 마지막 변경 시간을 현재 시간으로 갱신
    previousMillis = currentMillis;

    // LED 상태 반전
    ledState = !ledState;

    // 변경된 상태를 실제 GPIO에 출력
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    // 상태 확인용 로그 출력
    if (ledState) {
      Serial.println("LED ON  - millis timer");
    } else {
      Serial.println("LED OFF - millis timer");
    }
  }

  /*
    여기에는 다른 작업을 계속 추가할 수 있다.

    예:
    - 버튼 입력 확인
    - Emergency 입력 확인
    - 센서 상태 확인
    - 상태머신 처리

    delay()를 사용하지 않기 때문에 loop()가 멈추지 않는다.
  */
}