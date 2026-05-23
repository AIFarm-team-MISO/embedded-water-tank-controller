#include <Arduino.h>

/*
  ==================================================
  Project : Embedded Water Tank Controller
  Step    : 6. Basic State Machine
  Version : STATE_MACHINE_001
  Board   : ESP32-S3 DevKitC-1
  Framework : Arduino / PlatformIO
  ==================================================

  목적:
  - 버튼 입력으로 시스템 상태를 변경한다.
  - 현재 상태에 따라 LED 출력을 제어한다.
  - 입력 → 상태 변경 → 출력 제어 구조를 이해한다.

  회로:
  - Button: GPIO4 ─ 버튼 ─ GND
  - LED   : GPIO10 ─ 10kΩ 저항 ─ LED ─ GND

  상태:
  - IDLE : 대기 상태, LED OFF
  - RUN  : 운전 상태, LED ON

  INPUT_PULLUP 방식:
  - 버튼 안 누름 = HIGH
  - 버튼 누름   = LOW
*/

// 버튼 입력 핀
const int BUTTON_PIN = 4;

// LED 출력 핀
const int LED_PIN = 10;

// 버튼 디바운스 시간
const unsigned long DEBOUNCE_DELAY = 50;

// 시스템 상태 정의
enum SystemState {
  STATE_IDLE,
  STATE_RUN
};

// 현재 시스템 상태
SystemState currentState = STATE_IDLE;

// 버튼 입력 안정화를 위한 변수들
int lastRawButtonState = HIGH;      // 가장 최근에 읽은 원시 버튼 값
int stableButtonState = HIGH;       // 디바운스 후 확정된 버튼 상태
unsigned long lastDebounceTime = 0; // 버튼 값이 마지막으로 변한 시간

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 버튼은 내부 풀업 저항 사용
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // LED는 출력으로 사용
  pinMode(LED_PIN, OUTPUT);

  // 초기 상태는 IDLE이므로 LED OFF
  digitalWrite(LED_PIN, LOW);

  Serial.println("=================================");
  Serial.println("ESP32-S3 Basic State Machine");
  Serial.println("VERSION: STATE_MACHINE_001");
  Serial.println("BUTTON_PIN = GPIO4");
  Serial.println("LED_PIN    = GPIO10");
  Serial.println("=================================");
  Serial.println("Initial State: IDLE");
  Serial.println("Press button to toggle IDLE <-> RUN");
}

/*
  버튼 눌림 이벤트를 감지하는 함수

  단순히 버튼이 LOW인지 계속 확인하는 것이 아니라,
  버튼이 '새로 눌린 순간'만 true로 반환한다.

  즉,
  - 버튼을 계속 누르고 있어도 한 번만 true
  - 버튼을 떼었다가 다시 누르면 다시 true
*/
bool isButtonPressedEvent() {
  int rawButtonState = digitalRead(BUTTON_PIN);

  // 원시 버튼 값이 바뀌면 디바운스 타이머를 갱신한다.
  if (rawButtonState != lastRawButtonState) {
    lastDebounceTime = millis();
    lastRawButtonState = rawButtonState;
  }

  // 일정 시간 동안 값이 안정적으로 유지되었는지 확인한다.
  if (millis() - lastDebounceTime >= DEBOUNCE_DELAY) {

    // 안정화된 버튼 상태가 이전과 다를 때만 상태 변경으로 인정한다.
    if (rawButtonState != stableButtonState) {
      stableButtonState = rawButtonState;

      // INPUT_PULLUP에서는 버튼을 누르면 LOW가 된다.
      if (stableButtonState == LOW) {
        return true;
      }
    }
  }

  return false;
}

/*
  상태 전환 함수

  버튼이 눌릴 때마다
  IDLE이면 RUN으로,
  RUN이면 IDLE로 변경한다.
*/
void toggleState() {
  if (currentState == STATE_IDLE) {
    currentState = STATE_RUN;
    Serial.println("STATE CHANGE: IDLE -> RUN");
  } else {
    currentState = STATE_IDLE;
    Serial.println("STATE CHANGE: RUN -> IDLE");
  }
}

/*
  현재 상태에 따라 출력을 제어하는 함수

  여기서는 LED 하나만 제어하지만,
  나중에는 Pump, Alarm, Relay 출력도 이 함수 구조로 확장할 수 있다.
*/
void updateOutputs() {
  if (currentState == STATE_RUN) {
    digitalWrite(LED_PIN, HIGH);  // RUN 상태에서는 LED ON
  } else {
    digitalWrite(LED_PIN, LOW);   // IDLE 상태에서는 LED OFF
  }
}

void loop() {
  // 버튼이 새로 눌린 순간에만 상태를 변경한다.
  if (isButtonPressedEvent()) {
    toggleState();
  }

  // 현재 상태에 따라 LED 출력을 계속 갱신한다.
  updateOutputs();

  /*
    delay()를 사용하지 않기 때문에 loop()는 계속 빠르게 반복된다.

    이후 이 구조 안에 다음 기능을 추가할 수 있다.
    - Start / Stop 버튼
    - LOW / HIGH 수위 센서
    - Emergency Stop
    - Pump Delay Timer
    - Alarm Output
  */
}