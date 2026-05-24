#include <Arduino.h>

/*
  ==================================================
  Project : Embedded Water Tank Controller
  Step    : 7. Pump Delay Logic
  Version : PUMP_DELAY_001
  Board   : ESP32-S3 DevKitC-1
  Framework : Arduino / PlatformIO
  ==================================================

  목적:
  - 버튼 입력으로 펌프 동작 요청을 만든다.
  - 요청 즉시 Pump ON 하지 않고, 일정 시간 대기 후 Pump ON 한다.
  - millis() 기반 non-blocking timer를 State Machine에 적용한다.

  회로:
  - Button: GPIO4 ─ 버튼 ─ GND
  - Pump LED: GPIO10 ─ 10kΩ 저항 ─ LED ─ GND

  현재 테스트에서는 실제 펌프 대신 LED를 Pump 출력으로 사용한다.

  상태:
  - IDLE        : 대기 상태, Pump OFF
  - PUMP_DELAY  : Pump ON 전 대기 상태, Pump OFF
  - PUMP_ON     : Pump ON 상태, LED ON

  버튼 동작:
  - IDLE에서 버튼 누름       → PUMP_DELAY
  - PUMP_DELAY에서 버튼 누름 → IDLE, 타이머 취소
  - PUMP_ON에서 버튼 누름    → IDLE, Pump OFF
*/

// 버튼 입력 핀
const int BUTTON_PIN = 4;

// 펌프 출력 대신 사용할 LED 핀
const int PUMP_LED_PIN = 10;

// 버튼 디바운스 시간
const unsigned long DEBOUNCE_DELAY = 50;

// Pump ON 전 대기 시간
const unsigned long PUMP_DELAY_TIME = 3000;  // 3000ms = 3초

// 시스템 상태 정의
enum SystemState {
  STATE_IDLE,
  STATE_PUMP_DELAY,
  STATE_PUMP_ON
};

// 현재 시스템 상태
SystemState currentState = STATE_IDLE;

// Pump Delay 시작 시간
unsigned long pumpDelayStartTime = 0;

// 버튼 디바운스용 변수
int lastRawButtonState = HIGH;
int stableButtonState = HIGH;
unsigned long lastDebounceTime = 0;

/*
  현재 상태 이름을 문자열로 반환한다.
  Serial Monitor 로그를 보기 쉽게 하기 위한 함수이다.
*/
const char* getStateName(SystemState state) {
  switch (state) {
    case STATE_IDLE:
      return "IDLE";
    case STATE_PUMP_DELAY:
      return "PUMP_DELAY";
    case STATE_PUMP_ON:
      return "PUMP_ON";
    default:
      return "UNKNOWN";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 버튼은 내부 풀업 저항 사용
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Pump LED는 출력으로 사용
  pinMode(PUMP_LED_PIN, OUTPUT);

  // 초기 상태는 Pump OFF
  digitalWrite(PUMP_LED_PIN, LOW);

  Serial.println("=================================");
  Serial.println("ESP32-S3 Pump Delay Logic Test");
  Serial.println("VERSION: PUMP_DELAY_001");
  Serial.println("BUTTON_PIN   = GPIO4");
  Serial.println("PUMP_LED_PIN = GPIO10");
  Serial.println("PUMP_DELAY   = 3000ms");
  Serial.println("=================================");
  Serial.println("Initial State: IDLE");
  Serial.println("Press button: IDLE -> PUMP_DELAY -> PUMP_ON");
  Serial.println("Press again : Stop and return to IDLE");
}

/*
  버튼이 새로 눌린 순간만 감지하는 함수

  INPUT_PULLUP 방식:
  - 버튼 안 누름 = HIGH
  - 버튼 누름   = LOW

  버튼을 계속 누르고 있어도 한 번만 true를 반환한다.
*/
bool isButtonPressedEvent() {
  int rawButtonState = digitalRead(BUTTON_PIN);

  // 버튼 원시 값이 바뀌면 디바운스 타이머 갱신
  if (rawButtonState != lastRawButtonState) {
    lastDebounceTime = millis();
    lastRawButtonState = rawButtonState;
  }

  // 값이 일정 시간 동안 안정되었는지 확인
  if (millis() - lastDebounceTime >= DEBOUNCE_DELAY) {
    if (rawButtonState != stableButtonState) {
      stableButtonState = rawButtonState;

      // 버튼이 눌린 순간만 이벤트로 처리
      if (stableButtonState == LOW) {
        return true;
      }
    }
  }

  return false;
}

/*
  상태를 변경하는 함수

  상태가 바뀔 때마다 로그를 출력한다.
*/
void changeState(SystemState newState) {
  if (currentState != newState) {
    Serial.print("STATE CHANGE: ");
    Serial.print(getStateName(currentState));
    Serial.print(" -> ");
    Serial.println(getStateName(newState));

    currentState = newState;
  }
}

/*
  버튼 입력에 따른 상태 전환 처리
*/
void handleButtonEvent() {
  if (isButtonPressedEvent()) {

    if (currentState == STATE_IDLE) {
      // IDLE에서 버튼을 누르면 Pump Delay 시작
      pumpDelayStartTime = millis();
      changeState(STATE_PUMP_DELAY);
      Serial.println("Pump delay timer started.");
    }
    else {
      // PUMP_DELAY 또는 PUMP_ON 상태에서 버튼을 누르면 정지
      changeState(STATE_IDLE);
      Serial.println("Pump stopped by button.");
    }
  }
}

/*
  Pump Delay 타이머 처리

  PUMP_DELAY 상태에서 3초가 지나면 PUMP_ON 상태로 변경한다.
  delay()를 사용하지 않기 때문에 loop()는 멈추지 않는다.
*/
void handlePumpDelay() {
  if (currentState == STATE_PUMP_DELAY) {
    unsigned long currentMillis = millis();

    if (currentMillis - pumpDelayStartTime >= PUMP_DELAY_TIME) {
      changeState(STATE_PUMP_ON);
      Serial.println("Pump delay completed. Pump ON.");
    }
  }
}

/*
  현재 상태에 따라 Pump LED 출력을 제어한다.

  STATE_PUMP_ON 상태에서만 LED ON
  나머지 상태에서는 LED OFF
*/
void updateOutputs() {
  if (currentState == STATE_PUMP_ON) {
    digitalWrite(PUMP_LED_PIN, HIGH);  // Pump ON
  } else {
    digitalWrite(PUMP_LED_PIN, LOW);   // Pump OFF
  }
}

void loop() {
  // 1. 버튼 입력 확인
  handleButtonEvent();

  // 2. Pump Delay 타이머 확인
  handlePumpDelay();

  // 3. 현재 상태에 맞게 출력 갱신
  updateOutputs();

  /*
    이 구조에서는 delay()를 사용하지 않는다.

    따라서 PUMP_DELAY 상태로 3초를 기다리는 동안에도
    loop()는 계속 반복된다.

    이후 Emergency Stop 입력을 추가하면,
    Pump Delay 진행 중에도 즉시 정지할 수 있다.
  */
}