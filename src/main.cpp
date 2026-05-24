#include <Arduino.h>

/*
  ==================================================
  Project : Embedded Water Tank Controller
  Step    : 8. Emergency Stop Logic
  Version : EMERGENCY_STOP_001
  Board   : ESP32-S3 DevKitC-1
  Framework : Arduino / PlatformIO
  ==================================================

  목적:
  - 어떤 상태에서든 Emergency 버튼이 눌리면 즉시 Pump를 정지한다.
  - Emergency 상태에서는 Pump OFF, Alarm ON을 유지한다.
  - Emergency 해제 후 자동 재기동되지 않도록 한다.
  - 사용자가 Start 버튼을 다시 눌러야 IDLE 상태로 복귀한다.

  회로:
  - Start/Stop Button : GPIO4 ─ 버튼 ─ GND
  - Emergency Button  : GPIO5 ─ 버튼 ─ GND
  - Pump LED          : GPIO10 ─ 10kΩ 저항 ─ LED ─ GND
  - Alarm LED         : GPIO11 ─ 10kΩ 저항 ─ LED ─ GND

  INPUT_PULLUP 방식:
  - 버튼 안 누름 = HIGH
  - 버튼 누름   = LOW
*/

// 입력 핀
const int START_BUTTON_PIN = 4;
const int EMERGENCY_BUTTON_PIN = 5;

// 출력 핀
const int PUMP_LED_PIN = 10;
const int ALARM_LED_PIN = 11;

// 버튼 디바운스 시간
const unsigned long DEBOUNCE_DELAY = 50;

// Pump ON 전 대기 시간
const unsigned long PUMP_DELAY_TIME = 3000;  // 3초

// 시스템 상태 정의
enum SystemState {
  STATE_IDLE,
  STATE_PUMP_DELAY,
  STATE_PUMP_ON,
  STATE_EMERGENCY
};

// 현재 시스템 상태
SystemState currentState = STATE_IDLE;

// Pump Delay 시작 시간
unsigned long pumpDelayStartTime = 0;

// 버튼 디바운스 처리를 위한 구조체
struct ButtonState {
  int pin;
  int lastRawState;
  int stableState;
  unsigned long lastDebounceTime;
};

// Start 버튼 상태
ButtonState startButton = {
  START_BUTTON_PIN,
  HIGH,
  HIGH,
  0
};

// Emergency 버튼 상태
ButtonState emergencyButton = {
  EMERGENCY_BUTTON_PIN,
  HIGH,
  HIGH,
  0
};

/*
  현재 상태 이름을 문자열로 반환한다.
  Serial Monitor에서 상태 변화를 보기 쉽게 하기 위한 함수이다.
*/
const char* getStateName(SystemState state) {
  switch (state) {
    case STATE_IDLE:
      return "IDLE";
    case STATE_PUMP_DELAY:
      return "PUMP_DELAY";
    case STATE_PUMP_ON:
      return "PUMP_ON";
    case STATE_EMERGENCY:
      return "EMERGENCY";
    default:
      return "UNKNOWN";
  }
}

/*
  버튼이 새로 눌린 순간만 true로 반환한다.

  INPUT_PULLUP 방식:
  - 버튼 안 누름 = HIGH
  - 버튼 누름   = LOW

  이 함수는 버튼을 계속 누르고 있는 동안 반복 true를 반환하지 않고,
  HIGH -> LOW로 안정적으로 바뀐 순간에만 true를 반환한다.
*/
bool isButtonPressedEvent(ButtonState &button) {
  int rawState = digitalRead(button.pin);

  // 버튼 원시 값이 바뀌면 디바운스 타이머를 갱신한다.
  if (rawState != button.lastRawState) {
    button.lastDebounceTime = millis();
    button.lastRawState = rawState;
  }

  // 일정 시간 동안 입력이 안정되었는지 확인한다.
  if (millis() - button.lastDebounceTime >= DEBOUNCE_DELAY) {
    if (rawState != button.stableState) {
      button.stableState = rawState;

      // INPUT_PULLUP에서는 LOW가 버튼 눌림이다.
      if (button.stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

/*
  현재 버튼이 눌린 상태인지 확인한다.
  EMERGENCY 상태에서 Emergency 버튼이 아직 눌려 있는지 확인할 때 사용한다.
*/
bool isButtonCurrentlyPressed(ButtonState &button) {
  return button.stableState == LOW;
}

/*
  상태 변경 함수
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
  Emergency Stop 처리

  이 함수는 loop()에서 가장 먼저 처리한다.
  이유:
  - Emergency는 어떤 상태보다 우선순위가 높아야 하기 때문이다.
*/
void handleEmergencyEvent(bool emergencyPressedEvent) {
  if (emergencyPressedEvent && currentState != STATE_EMERGENCY) {
    changeState(STATE_EMERGENCY);
    Serial.println("!!! EMERGENCY STOP TRIGGERED !!!");
    Serial.println("Pump OFF, Alarm ON");
  }
}

/*
  Start / Stop 버튼 처리

  상태별 버튼 동작:
  - IDLE        → PUMP_DELAY
  - PUMP_DELAY  → IDLE
  - PUMP_ON     → IDLE
  - EMERGENCY   → 조건 만족 시 IDLE 복귀

  Emergency 상태에서는 Emergency 버튼이 해제된 뒤에만 IDLE로 복귀할 수 있다.
*/
void handleStartButtonEvent(bool startPressedEvent) {
  if (!startPressedEvent) {
    return;
  }

  if (currentState == STATE_EMERGENCY) {
    // Emergency 버튼이 아직 눌린 상태라면 복귀 금지
    if (isButtonCurrentlyPressed(emergencyButton)) {
      Serial.println("Cannot reset: Emergency button is still pressed.");
      return;
    }

    // Emergency 해제 후 Start 버튼을 눌러야 IDLE로 복귀
    changeState(STATE_IDLE);
    Serial.println("Emergency reset. System returned to IDLE.");
    Serial.println("Press Start again to begin pump delay.");
    return;
  }

  if (currentState == STATE_IDLE) {
    // IDLE에서 Start 버튼을 누르면 Pump Delay 시작
    pumpDelayStartTime = millis();
    changeState(STATE_PUMP_DELAY);
    Serial.println("Pump delay timer started.");
  } 
  else {
    // PUMP_DELAY 또는 PUMP_ON 상태에서 Start 버튼을 다시 누르면 정지
    changeState(STATE_IDLE);
    Serial.println("Pump stopped by Start/Stop button.");
  }
}

/*
  Pump Delay 타이머 처리

  PUMP_DELAY 상태에서 3초가 지나면 PUMP_ON 상태로 변경한다.
  EMERGENCY 상태에서는 이 로직이 실행되지 않도록 loop()에서 상태를 분리한다.
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
  현재 상태에 따라 출력을 제어한다.

  가장 중요한 안전 조건:
  - EMERGENCY 상태에서는 무조건 Pump OFF
  - EMERGENCY 상태에서는 Alarm ON
*/
void updateOutputs() {
  switch (currentState) {
    case STATE_IDLE:
      digitalWrite(PUMP_LED_PIN, LOW);    // Pump OFF
      digitalWrite(ALARM_LED_PIN, LOW);   // Alarm OFF
      break;

    case STATE_PUMP_DELAY:
      digitalWrite(PUMP_LED_PIN, LOW);    // 아직 Pump OFF
      digitalWrite(ALARM_LED_PIN, LOW);   // Alarm OFF
      break;

    case STATE_PUMP_ON:
      digitalWrite(PUMP_LED_PIN, HIGH);   // Pump ON
      digitalWrite(ALARM_LED_PIN, LOW);   // Alarm OFF
      break;

    case STATE_EMERGENCY:
      digitalWrite(PUMP_LED_PIN, LOW);    // Emergency에서는 무조건 Pump OFF
      digitalWrite(ALARM_LED_PIN, HIGH);  // Alarm ON
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 입력 핀 설정
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(EMERGENCY_BUTTON_PIN, INPUT_PULLUP);

  // 출력 핀 설정
  pinMode(PUMP_LED_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);

  // 초기 출력 상태
  digitalWrite(PUMP_LED_PIN, LOW);
  digitalWrite(ALARM_LED_PIN, LOW);

  Serial.println("=================================");
  Serial.println("ESP32-S3 Emergency Stop Logic Test");
  Serial.println("VERSION: EMERGENCY_STOP_001");
  Serial.println("START_BUTTON_PIN     = GPIO4");
  Serial.println("EMERGENCY_BUTTON_PIN = GPIO5");
  Serial.println("PUMP_LED_PIN         = GPIO10");
  Serial.println("ALARM_LED_PIN        = GPIO11");
  Serial.println("PUMP_DELAY           = 3000ms");
  Serial.println("=================================");
  Serial.println("Initial State: IDLE");
  Serial.println("Start button: IDLE -> PUMP_DELAY -> PUMP_ON");
  Serial.println("Emergency button: Any state -> EMERGENCY");
  Serial.println("Emergency reset: release Emergency, then press Start");
}

void loop() {
  // 버튼 이벤트는 매 loop마다 먼저 읽어둔다.
  bool emergencyPressedEvent = isButtonPressedEvent(emergencyButton);
  bool startPressedEvent = isButtonPressedEvent(startButton);

  // 1. Emergency는 항상 최우선 처리
  handleEmergencyEvent(emergencyPressedEvent);

  // 2. Start / Stop / Emergency Reset 처리
  handleStartButtonEvent(startPressedEvent);

  // 3. Emergency 상태가 아닐 때만 Pump Delay 진행
  if (currentState != STATE_EMERGENCY) {
    handlePumpDelay();
  }

  // 4. 현재 상태에 따라 출력 갱신
  updateOutputs();

  /*
    이 구조에서 Emergency는 가장 높은 우선순위를 가진다.

    PUMP_DELAY 중 Emergency 발생:
    → 즉시 STATE_EMERGENCY
    → Pump OFF
    → Alarm ON

    PUMP_ON 중 Emergency 발생:
    → 즉시 STATE_EMERGENCY
    → Pump OFF
    → Alarm ON

    Emergency 해제 후 자동 재기동되지 않으며,
    Start 버튼을 다시 눌러 IDLE로 복귀해야 한다.
  */
}