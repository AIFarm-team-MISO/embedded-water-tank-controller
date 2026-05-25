#include <Arduino.h>

/*
  ==================================================
  Project   : Embedded Water Tank Controller
  Step      : 9. Full Water Tank Controller
  Version   : FULL_WATER_TANK_005
  Board     : ESP32-S3 DevKitC-1
  Framework : Arduino / PlatformIO
  ==================================================

  목적:
  - Start / Stop / LOW / HIGH / Emergency 입력을 통합한다.
  - LOW Sensor 감지 후 Pump Delay를 거쳐 Pump ON 한다.
  - HIGH Sensor 감지 시 Pump를 정지하고 FULL 상태로 전환한다.
  - Emergency 입력은 어떤 상태에서도 최우선 처리한다.
  - Emergency 버튼을 다시 누르면 IDLE 상태로 Reset한다.
  - Emergency Reset 후 자동 재기동되지 않고, Start 버튼을 눌러야 다시 RUN 상태가 된다.
  - 영상 촬영용으로 Serial Monitor 로그를 간결하게 출력한다.

  --------------------------------------------------
  버튼 입력:
  --------------------------------------------------
  - START     : GPIO4  -> 버튼 -> GND
  - STOP      : GPIO6  -> 버튼 -> GND
  - EMERGENCY : GPIO5  -> 버튼 -> GND
  - LOW       : GPIO7  -> 버튼 -> GND
  - HIGH      : GPIO8  -> 버튼 -> GND

  모든 입력은 INPUT_PULLUP 방식이다.

  INPUT_PULLUP:
  - 버튼 안 누름 = HIGH
  - 버튼 누름   = LOW

  --------------------------------------------------
  LED 출력:
  --------------------------------------------------
  - LOW Sensor LED  : GPIO12 -> 저항 -> 파란 LED -> GND
  - Pump LED        : GPIO10 -> 저항 -> 빨간 LED -> GND
  - HIGH / FULL LED : GPIO13 -> 저항 -> 흰색 LED -> GND
  - Alarm LED       : GPIO11 -> 저항 -> 노란 LED -> GND

  --------------------------------------------------
  LED 의미:
  --------------------------------------------------
  - 파란 LED  : PUMP_DELAY 상태 표시
  - 빨간 LED  : FILLING / Pump ON 상태 표시
  - 흰색 LED  : FULL / 만수 상태 표시
  - 노란 LED  : EMERGENCY / Alarm 상태 표시

  --------------------------------------------------
  상태 흐름:
  --------------------------------------------------
  IDLE
    -> START
  RUN
    -> LOW Sensor
  PUMP_DELAY
    -> 3초 경과
  FILLING
    -> HIGH Sensor
  FULL

  어떤 상태에서든:
    -> EMERGENCY 버튼
  EMERGENCY

  EMERGENCY 상태에서:
    -> EMERGENCY 버튼 다시 누름
  IDLE

  이후:
    -> START 버튼
  RUN
*/

// ==================================================
// Debug Settings
// ==================================================

// 영상 촬영용 기본값: false
// 버튼 눌림/뗌 로그까지 확인하고 싶으면 true로 변경
const bool DEBUG_INPUT_LOG = false;

// ==================================================
// Pin Definition
// ==================================================

// 입력 핀
const int START_BUTTON_PIN     = 4;   // 녹색 버튼
const int STOP_BUTTON_PIN      = 6;   // 빨간 버튼
const int EMERGENCY_BUTTON_PIN = 5;   // 노란 버튼
const int LOW_SENSOR_PIN       = 7;   // 파란 버튼
const int HIGH_SENSOR_PIN      = 8;   // 흰색 버튼

// 출력 핀
const int LOW_SENSOR_LED_PIN   = 12;  // 파란 LED: Pump Delay 표시
const int PUMP_LED_PIN         = 10;  // 빨간 LED: Pump ON / Filling 표시
const int HIGH_SENSOR_LED_PIN  = 13;  // 흰색 LED: Full 표시
const int ALARM_LED_PIN        = 11;  // 노란 LED: Emergency / Alarm 표시

// ==================================================
// Timing Settings
// ==================================================

// 버튼 디바운스 시간
const unsigned long DEBOUNCE_DELAY = 50;

// Pump ON 전 대기 시간
const unsigned long PUMP_DELAY_TIME = 3000;  // 3000ms = 3초

// ==================================================
// State Definition
// ==================================================

enum SystemState {
  STATE_IDLE,        // 대기 상태
  STATE_RUN,         // 운전 상태, 센서 감시
  STATE_PUMP_DELAY,  // LOW 감지 후 Pump ON 전 대기
  STATE_FILLING,     // Pump ON, 물 채우는 상태
  STATE_FULL,        // HIGH 감지, 만수 상태
  STATE_EMERGENCY    // 비상 정지 상태
};

// 현재 시스템 상태
SystemState currentState = STATE_IDLE;

// Pump Delay 시작 시간
unsigned long pumpDelayStartTime = 0;

// ==================================================
// Input Structure
// ==================================================

/*
  버튼과 센서 입력을 같은 구조로 관리하기 위한 구조체

  pin:
  - 입력이 연결된 GPIO 번호

  name:
  - 디버그 로그에 출력할 입력 이름

  lastRawState:
  - 마지막으로 읽은 원시 입력값

  stableState:
  - 디바운스 후 확정된 입력값

  lastDebounceTime:
  - 입력값이 마지막으로 바뀐 시간

  pressedEvent:
  - 새로 눌린 순간 true

  releasedEvent:
  - 새로 떼어진 순간 true
*/
struct InputState {
  int pin;
  const char* name;
  int lastRawState;
  int stableState;
  unsigned long lastDebounceTime;
  bool pressedEvent;
  bool releasedEvent;
};

// 입력 객체 생성
InputState startButton = {
  START_BUTTON_PIN,
  "START",
  HIGH,
  HIGH,
  0,
  false,
  false
};

InputState stopButton = {
  STOP_BUTTON_PIN,
  "STOP",
  HIGH,
  HIGH,
  0,
  false,
  false
};

InputState emergencyButton = {
  EMERGENCY_BUTTON_PIN,
  "EMERGENCY",
  HIGH,
  HIGH,
  0,
  false,
  false
};

InputState lowSensor = {
  LOW_SENSOR_PIN,
  "LOW_SENSOR",
  HIGH,
  HIGH,
  0,
  false,
  false
};

InputState highSensor = {
  HIGH_SENSOR_PIN,
  "HIGH_SENSOR",
  HIGH,
  HIGH,
  0,
  false,
  false
};

// ==================================================
// Utility Functions
// ==================================================

/*
  현재 상태를 문자열로 변환한다.
  Serial Monitor에서 상태 변화를 확인하기 쉽게 하기 위한 함수이다.
*/
const char* getStateName(SystemState state) {
  switch (state) {
    case STATE_IDLE:
      return "IDLE";
    case STATE_RUN:
      return "RUN";
    case STATE_PUMP_DELAY:
      return "PUMP_DELAY";
    case STATE_FILLING:
      return "FILLING";
    case STATE_FULL:
      return "FULL";
    case STATE_EMERGENCY:
      return "EMERGENCY";
    default:
      return "UNKNOWN";
  }
}

/*
  영상 촬영용 상태 변경 로그 함수

  출력 형식:
  [EVENT] OLD_STATE -> NEW_STATE
    - Message

  예:
  [LOW] RUN -> PUMP_DELAY
    - Pump delay started: 3000ms.
*/
void changeState(SystemState newState, const char* eventName, const char* message) {
  if (currentState != newState) {
    Serial.println();

    Serial.print("[");
    Serial.print(eventName);
    Serial.print("] ");

    Serial.print(getStateName(currentState));
    Serial.print(" -> ");
    Serial.println(getStateName(newState));

    if (message != nullptr) {
      Serial.print("  - ");
      Serial.println(message);
    }

    currentState = newState;
  }
}

/*
  현재 입력이 눌린 상태인지 확인한다.

  INPUT_PULLUP 방식에서는:
  - 눌림 = LOW
  - 안 눌림 = HIGH
*/
bool isPressed(InputState &input) {
  return input.stableState == LOW;
}

// ==================================================
// Input Update
// ==================================================

/*
  입력 상태 업데이트 함수

  역할:
  - GPIO 입력값을 읽는다.
  - 디바운스를 처리한다.
  - 새로 눌린 순간 pressedEvent를 true로 만든다.
  - 새로 떼어진 순간 releasedEvent를 true로 만든다.

  영상용 기본 모드에서는 입력 로그를 출력하지 않는다.
  DEBUG_INPUT_LOG를 true로 바꾸면 PRESSED / RELEASED 로그를 확인할 수 있다.
*/
void updateInput(InputState &input) {
  input.pressedEvent = false;
  input.releasedEvent = false;

  int rawState = digitalRead(input.pin);

  // 원시 입력값이 바뀌면 디바운스 타이머 갱신
  if (rawState != input.lastRawState) {
    input.lastDebounceTime = millis();
    input.lastRawState = rawState;
  }

  // 일정 시간 동안 값이 안정적으로 유지되었을 때만 실제 상태로 인정
  if (millis() - input.lastDebounceTime >= DEBOUNCE_DELAY) {
    if (rawState != input.stableState) {
      input.stableState = rawState;

      if (input.stableState == LOW) {
        input.pressedEvent = true;

        if (DEBUG_INPUT_LOG) {
          Serial.print("DEBUG INPUT: ");
          Serial.print(input.name);
          Serial.println(" PRESSED");
        }
      } else {
        input.releasedEvent = true;

        if (DEBUG_INPUT_LOG) {
          Serial.print("DEBUG INPUT: ");
          Serial.print(input.name);
          Serial.println(" RELEASED");
        }
      }
    }
  }
}

/*
  모든 입력을 한 번씩 업데이트한다.
*/
void updateAllInputs() {
  updateInput(startButton);
  updateInput(stopButton);
  updateInput(emergencyButton);
  updateInput(lowSensor);
  updateInput(highSensor);
}

/*
  setup()에서 입력의 실제 초기 상태를 코드 내부 상태와 동기화한다.

  보드가 시작될 때 버튼이 눌려 있거나,
  배선 상태가 HIGH/LOW 중 하나로 고정되어 있을 수 있으므로,
  실제 값을 한 번 읽어서 stableState에 반영한다.
*/
void initializeInput(InputState &input) {
  input.lastRawState = digitalRead(input.pin);
  input.stableState = input.lastRawState;
  input.lastDebounceTime = millis();
  input.pressedEvent = false;
  input.releasedEvent = false;
}

// ==================================================
// Emergency Logic
// ==================================================

/*
  Emergency 처리 함수

  동작:
  1. 일반 상태에서 Emergency 버튼을 누르면 EMERGENCY 상태로 전환
  2. EMERGENCY 상태에서 Emergency 버튼을 다시 누르면 IDLE 상태로 Reset

  중요한 점:
  - Emergency Reset은 RUN으로 복귀하지 않는다.
  - Reset 후에는 반드시 START 버튼을 눌러야 다시 운전한다.
*/
void handleEmergency() {
  if (!emergencyButton.pressedEvent) {
    return;
  }

  // 일반 상태에서 Emergency 버튼을 누르면 비상 정지
  if (currentState != STATE_EMERGENCY) {
    changeState(
      STATE_EMERGENCY,
      "EMERGENCY",
      "Pump OFF / Alarm ON"
    );
    return;
  }

  // EMERGENCY 상태에서 Emergency 버튼을 다시 누르면 IDLE로 Reset
  changeState(
    STATE_IDLE,
    "RESET",
    "Emergency reset. Press START to run system."
  );
}

// ==================================================
// Stop Logic
// ==================================================

/*
  Stop 처리 함수

  Emergency가 아닌 상태에서 Stop 버튼을 누르면 IDLE로 복귀한다.
  Stop은 일반 정지 명령이다.
*/
void handleStop() {
  if (stopButton.pressedEvent && currentState != STATE_EMERGENCY) {
    changeState(
      STATE_IDLE,
      "STOP",
      "System stopped. All outputs OFF."
    );
  }
}

// ==================================================
// Start Logic
// ==================================================

/*
  Start 처리 함수

  동작:
  - IDLE 상태에서 Start 버튼을 누르면 RUN 상태로 전환
  - FULL 상태에서 Start 버튼을 누르면 RUN 상태로 재개
  - EMERGENCY 상태에서는 Start 버튼으로 시작할 수 없음

  Emergency 이후에는:
  1. Emergency 버튼을 다시 눌러 IDLE로 Reset
  2. 그 다음 Start 버튼을 눌러 RUN 시작
*/
void handleStart() {
  if (!startButton.pressedEvent) {
    return;
  }

  // Emergency 상태에서는 Start 금지
  if (currentState == STATE_EMERGENCY) {
    Serial.println();
    Serial.println("[START] Blocked");
    Serial.println("  - System is in EMERGENCY state.");
    Serial.println("  - Press EMERGENCY again to reset.");
    return;
  }

  // IDLE에서 Start
  if (currentState == STATE_IDLE) {
    changeState(
      STATE_RUN,
      "START",
      "Monitoring level sensors."
    );
    return;
  }

  // FULL 상태에서 다시 운전 재개
  if (currentState == STATE_FULL) {
    changeState(
      STATE_RUN,
      "START",
      "Restarted from FULL. Monitoring level sensors."
    );
    return;
  }
}

// ==================================================
// Level Sensor Logic
// ==================================================

/*
  수위 센서 처리 함수

  HIGH Sensor:
  - 물이 가득 찼다는 의미
  - Pump를 꺼야 하므로 우선 처리한다.

  LOW Sensor:
  - 물이 부족하다는 의미
  - RUN 또는 FULL 상태에서 감지되면 PUMP_DELAY로 전환한다.
*/
void handleLevelSensors() {
  bool lowDetected = isPressed(lowSensor);
  bool highDetected = isPressed(highSensor);

  // HIGH Sensor는 Pump OFF 조건이므로 우선 처리
  if (highDetected) {
    if (currentState == STATE_RUN ||
        currentState == STATE_PUMP_DELAY ||
        currentState == STATE_FILLING) {
      changeState(
        STATE_FULL,
        "HIGH",
        "Tank FULL. Pump OFF."
      );
      return;
    }
  }

  // LOW Sensor 감지 시 Pump Delay 시작
  if (lowDetected) {
    if (currentState == STATE_RUN || currentState == STATE_FULL) {
      pumpDelayStartTime = millis();

      changeState(
        STATE_PUMP_DELAY,
        "LOW",
        "Pump delay started: 3000ms."
      );
      return;
    }
  }
}

// ==================================================
// Pump Delay Logic
// ==================================================

/*
  Pump Delay 처리 함수

  PUMP_DELAY 상태에서 3초가 지나면 FILLING 상태로 전환한다.

  delay()를 사용하지 않고 millis()로 시간을 비교하므로,
  Pump Delay 중에도 Emergency나 Stop 입력을 즉시 처리할 수 있다.
*/
void handlePumpDelay() {
  if (currentState == STATE_PUMP_DELAY) {
    unsigned long currentMillis = millis();

    if (currentMillis - pumpDelayStartTime >= PUMP_DELAY_TIME) {
      changeState(
        STATE_FILLING,
        "TIMER",
        "Pump ON. Filling started."
      );
    }
  }
}

// ==================================================
// Output Logic
// ==================================================

/*
  상태에 따른 LED 출력 제어

  LED는 버튼 입력 자체보다 시스템 상태를 표시하는 용도로 사용한다.

  - IDLE        : 모든 LED OFF
  - RUN         : 모든 LED OFF
  - PUMP_DELAY  : 파란 LED ON
  - FILLING     : 빨간 LED ON
  - FULL        : 흰색 LED ON
  - EMERGENCY   : 노란 LED ON
*/
void updateOutputs() {
  // 파란 LED: PUMP_DELAY 상태 표시
  digitalWrite(
    LOW_SENSOR_LED_PIN,
    currentState == STATE_PUMP_DELAY ? HIGH : LOW
  );

  // 빨간 LED: Pump ON / FILLING 상태 표시
  digitalWrite(
    PUMP_LED_PIN,
    currentState == STATE_FILLING ? HIGH : LOW
  );

  // 흰색 LED: FULL 상태 표시
  digitalWrite(
    HIGH_SENSOR_LED_PIN,
    currentState == STATE_FULL ? HIGH : LOW
  );

  // 노란 LED: EMERGENCY / Alarm 상태 표시
  digitalWrite(
    ALARM_LED_PIN,
    currentState == STATE_EMERGENCY ? HIGH : LOW
  );
}

// ==================================================
// Setup
// ==================================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 입력 핀 설정
  pinMode(START_BUTTON_PIN, INPUT_PULLUP);
  pinMode(STOP_BUTTON_PIN, INPUT_PULLUP);
  pinMode(EMERGENCY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LOW_SENSOR_PIN, INPUT_PULLUP);
  pinMode(HIGH_SENSOR_PIN, INPUT_PULLUP);

  // 출력 핀 설정
  pinMode(LOW_SENSOR_LED_PIN, OUTPUT);
  pinMode(PUMP_LED_PIN, OUTPUT);
  pinMode(HIGH_SENSOR_LED_PIN, OUTPUT);
  pinMode(ALARM_LED_PIN, OUTPUT);

  // 출력 초기화
  digitalWrite(LOW_SENSOR_LED_PIN, LOW);
  digitalWrite(PUMP_LED_PIN, LOW);
  digitalWrite(HIGH_SENSOR_LED_PIN, LOW);
  digitalWrite(ALARM_LED_PIN, LOW);

  // 입력 초기 상태 동기화
  initializeInput(startButton);
  initializeInput(stopButton);
  initializeInput(emergencyButton);
  initializeInput(lowSensor);
  initializeInput(highSensor);

  // 시작 로그 출력
  Serial.println("=================================");
  Serial.println("ESP32-S3 Full Water Tank Controller");
  Serial.println("VERSION: FULL_WATER_TANK_005");
  Serial.println("=================================");

  Serial.println("[INPUT]");
  Serial.println("START     = GPIO4");
  Serial.println("STOP      = GPIO6");
  Serial.println("EMERGENCY = GPIO5");
  Serial.println("LOW       = GPIO7");
  Serial.println("HIGH      = GPIO8");

  Serial.println("---------------------------------");

  Serial.println("[OUTPUT]");
  Serial.println("LOW LED   = GPIO12");
  Serial.println("PUMP LED  = GPIO10");
  Serial.println("HIGH LED  = GPIO13");
  Serial.println("ALARM LED = GPIO11");

  Serial.println("---------------------------------");

  Serial.println("PUMP_DELAY = 3000ms");
  Serial.println("Initial State: IDLE");

  Serial.println("---------------------------------");

  Serial.println("[FLOW]");
  Serial.println("IDLE -> START -> RUN");
  Serial.println("RUN -> LOW -> PUMP_DELAY");
  Serial.println("PUMP_DELAY -> 3s -> FILLING");
  Serial.println("FILLING -> HIGH -> FULL");
  Serial.println("Any state -> EMERGENCY");
  Serial.println("EMERGENCY -> Emergency again -> IDLE");
  Serial.println("IDLE -> START -> RUN");

  Serial.println("=================================");
}

// ==================================================
// Main Loop
// ==================================================

void loop() {
  // 1. 모든 입력 상태 업데이트
  updateAllInputs();

  // 2. Emergency는 항상 최우선 처리
  handleEmergency();

  /*
    Emergency 상태에서는 일반 로직을 실행하지 않는다.

    단, handleEmergency()에서 Emergency 버튼이 다시 눌리면
    STATE_IDLE로 Reset될 수 있다.

    currentState가 여전히 EMERGENCY라면
    Pump OFF / Alarm ON 상태를 유지하고 loop를 종료한다.
  */
  if (currentState == STATE_EMERGENCY) {
    updateOutputs();
    return;
  }

  // 3. Stop 처리
  handleStop();

  // 4. Start 처리
  handleStart();

  // 5. 수위 센서 처리
  handleLevelSensors();

  // 6. Pump Delay 타이머 처리
  handlePumpDelay();

  // 7. 현재 상태에 따라 출력 갱신
  updateOutputs();

  /*
    전체 처리 우선순위:

    1. 입력 업데이트
    2. Emergency 처리
    3. Emergency 상태 유지 또는 Reset
    4. Stop 처리
    5. Start 처리
    6. Level Sensor 처리
    7. Pump Delay 처리
    8. Output 처리

    핵심 안전 조건:
    - Emergency 상태에서는 Pump가 항상 OFF
    - Emergency 상태에서는 Alarm이 항상 ON
    - Emergency Reset 후에는 IDLE로만 복귀
    - 재운전은 반드시 Start 버튼으로 수행
  */
}