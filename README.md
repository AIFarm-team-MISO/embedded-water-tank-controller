# Embedded Water Tank Controller

ESP32-S3 기반으로 구현한 수위 자동 제어 시스템입니다.

이 프로젝트는 **GX Works2로 설계했던 PLC 래더 기반 Water Tank Auto Control System**의 제어 흐름을 임베디드 C/C++ 구조로 재구성한 것입니다.  
버튼 입력, 수위 센서 시뮬레이션, Pump Delay, State Machine, Emergency Stop 로직을 하나의 시스템으로 구현하여, PLC 제어 사고를 ESP32-S3 임베디드 환경에서 검증했습니다.

---

## Demo & Portfolio

| 구분 | 링크 |
|---|---|
| Demo Video | https://drive.google.com/file/d/1-pfB4ODrYTeBe4RWIjsHS0X9Hq-xsizD/view |
| Notion Portfolio | https://www.notion.so/Full-Water-Tank-Controller-368097d0083980acb97bc15f0b612f40?source=copy_link |

---

## 1. Project Overview

### 프로젝트 목적

```text
GX Works2 PLC 래더 기반 수위 자동 제어 로직
→ ESP32-S3 기반 임베디드 C/C++ 구조로 재구현
→ 실제 버튼 / LED 회로로 동작 검증
```

### 핵심 제어 흐름

```text
입력
→ 상태 판단
→ 시간 조건
→ 출력 제어
→ 안전 차단
```

### Main Flow

```text
START
→ RUN
→ LOW Sensor
→ PUMP_DELAY
→ FILLING
→ HIGH Sensor
→ FULL
```

### Emergency Flow

```text
Any State
→ EMERGENCY
→ Pump OFF
→ Alarm ON
→ Emergency Reset
→ IDLE
→ START
→ RUN
```

---

## 2. Development Environment

| 항목 | 내용 |
|---|---|
| Board | ESP32-S3-WROOM-1-N16R8 |
| IDE | Visual Studio Code |
| Platform | PlatformIO |
| Framework | Arduino |
| Language | C / C++ |
| Upload | USB-C / Native USB |
| Serial Monitor | USB CDC Serial |
| Current Version | FULL_WATER_TANK_005 |

---

## 3. Hardware

### 사용 부품

```text
ESP32-S3 development board
Breadboard
Tact switch x 5
LED x 4
10kΩ resistor
Dupont jumper wires
USB-C data cable
```

### 부품 구성 메모

```text
현재 LED 저항: 10kΩ
LED 밝기: 약하게 표시됨
추후 개선: 330Ω 또는 470Ω 저항 사용 예정
```

---

## 4. Pin Assignment

### Input Buttons

입력 방식: `INPUT_PULLUP`

```text
Released = HIGH
Pressed  = LOW
```

| Button | Color | Role | GPIO | Connection |
|---|---|---|---:|---|
| START | Green | 시스템 시작 | GPIO4 | GPIO4 → Button → GND |
| STOP | Red | 일반 정지 | GPIO6 | GPIO6 → Button → GND |
| EMERGENCY | Yellow | 비상 정지 / Reset | GPIO5 | GPIO5 → Button → GND |
| LOW Sensor | Blue | 수위 낮음 감지 | GPIO7 | GPIO7 → Button → GND |
| HIGH Sensor | White | 수위 높음 / 만수 감지 | GPIO8 | GPIO8 → Button → GND |

---

### Output LEDs

LED 역할: 버튼 입력 표시가 아니라 **현재 시스템 상태 표시**

| LED | Role | GPIO | ON State |
|---|---|---:|---|
| LOW Lamp | Pump Delay 표시 | GPIO12 | PUMP_DELAY |
| Pump Lamp | Pump ON / Filling 표시 | GPIO10 | FILLING |
| Full Lamp | 만수 표시 | GPIO13 | FULL |
| Alarm Lamp | Emergency / Alarm 표시 | GPIO11 | EMERGENCY |

LED 배열:

```text
[LOW Lamp] → [Pump Lamp] → [Full Lamp] → [Alarm Lamp]
```

---

## 5. State Machine

### State Definition

```cpp
enum SystemState {
  STATE_IDLE,
  STATE_RUN,
  STATE_PUMP_DELAY,
  STATE_FILLING,
  STATE_FULL,
  STATE_EMERGENCY
};
```

### State Description

| State | Meaning | Output |
|---|---|---|
| IDLE | 대기 상태 | All OFF |
| RUN | 운전 상태 / 센서 감시 | All OFF |
| PUMP_DELAY | LOW 감지 후 Pump ON 전 대기 | LOW Lamp ON |
| FILLING | Pump ON / 급수 중 | Pump Lamp ON |
| FULL | 만수 상태 | Full Lamp ON |
| EMERGENCY | 비상 정지 상태 | Alarm Lamp ON / Pump OFF |

---

## 6. Control Flow

### Normal Flow

```text
IDLE
  ↓ START
RUN
  ↓ LOW Sensor
PUMP_DELAY
  ↓ 3 seconds
FILLING
  ↓ HIGH Sensor
FULL
```

### Refill Flow

```text
FULL
  ↓ LOW Sensor
PUMP_DELAY
  ↓ 3 seconds
FILLING
  ↓ HIGH Sensor
FULL
```

### Stop Flow

```text
RUN / PUMP_DELAY / FILLING / FULL
  ↓ STOP
IDLE
```

### Emergency Flow

```text
Any State
  ↓ EMERGENCY
EMERGENCY
  ↓ EMERGENCY again
IDLE
  ↓ START
RUN
```

### Emergency 설계 기준

```text
Emergency 상태 진입 시 Pump 즉시 OFF
Emergency 상태에서는 Alarm Lamp ON
Emergency Reset 후 IDLE로만 복귀
자동 재기동 없음
START 입력 후 RUN 재진입
```

---

## 7. Software Architecture

### 전체 소스 구조

현재 코드는 하나의 `src/main.cpp` 파일 안에 작성되어 있으나, 기능별 역할을 함수 단위로 분리.

```text
Pin Definition
→ State Definition
→ Input Structure
→ Input Update
→ Emergency Logic
→ Stop Logic
→ Start Logic
→ Level Sensor Logic
→ Pump Delay Logic
→ Output Logic
→ Main Loop
```

### Main Modules

| Module | Function | Role |
|---|---|---|
| Pin Definition | GPIO 상수 정의 | 입력 / 출력 핀 번호 관리 |
| State Definition | `SystemState` enum | 시스템 상태 정의 |
| Input Structure | `InputState` struct | 버튼 / 센서 입력 상태 관리 |
| Input Update | `updateInput()` / `updateAllInputs()` | 디바운스 및 입력 이벤트 처리 |
| Emergency Logic | `handleEmergency()` | 비상정지 및 Reset 처리 |
| Stop Logic | `handleStop()` | 일반 정지 처리 |
| Start Logic | `handleStart()` | 시스템 시작 처리 |
| Level Sensor Logic | `handleLevelSensors()` | LOW / HIGH 센서 조건 처리 |
| Pump Delay Logic | `handlePumpDelay()` | `millis()` 기반 Pump Delay 처리 |
| Output Logic | `updateOutputs()` | 현재 상태에 따른 LED 출력 제어 |
| Main Loop | `loop()` | 전체 제어 순서 실행 |

---

## 8. Input Structure

### InputState 구조체

버튼과 센서 입력을 동일한 구조로 관리.

```cpp
struct InputState {
  int pin;
  const char* name;
  int lastRawState;
  int stableState;
  unsigned long lastDebounceTime;
  bool pressedEvent;
  bool releasedEvent;
};
```

### 사용 목적

```text
버튼 입력과 센서 입력을 같은 방식으로 처리
디바운스 로직 재사용
pressed / released 이벤트 분리
입력 추가 시 구조 확장 용이
```

### 입력 처리 흐름

```text
digitalRead()
→ raw input 확인
→ debounce time 확인
→ stable state 확정
→ pressedEvent / releasedEvent 생성
```

---

## 9. Event-Based Button Handling

버튼을 계속 누르고 있는 동안 같은 동작이 반복되지 않도록 이벤트 기반 처리.

```text
Button Released
→ Button Pressed
→ pressedEvent = true
→ Logic executed once
```

### 장점

```text
버튼 길게 누름으로 인한 중복 실행 방지
Start / Stop 중복 처리 방지
Emergency Reset 중복 처리 방지
입력 이벤트와 상태 전환 분리
```

---

## 10. Button Debounce

택트 스위치 입력의 흔들림을 줄이기 위해 `millis()` 기반 디바운스 사용.

```text
Input Change
→ Debounce Timer Update
→ 50ms Stable Check
→ Stable Input Accepted
```

### Debounce 설정

```cpp
const unsigned long DEBOUNCE_DELAY = 50;
```

---

## 11. Non-Blocking Timer

Pump Delay는 `delay()`가 아니라 `millis()` 기반으로 구현.

```cpp
if (currentMillis - pumpDelayStartTime >= PUMP_DELAY_TIME) {
  changeState(
    STATE_FILLING,
    "TIMER",
    "Pump ON. Filling started."
  );
}
```

### 설계 이유

```text
delay() 사용 시 시스템 전체 대기 발생
Pump Delay 중 Emergency 입력 처리 불가
Stop 입력 반응 지연 가능
```

### millis() 사용 결과

```text
PUMP_DELAY 상태에서 3초 대기
대기 중에도 Emergency 입력 가능
대기 중에도 Stop 입력 가능
시스템 non-blocking 구조 유지
```

---

## 12. Control Priority

`loop()` 내부 처리 우선순위:

```cpp
void loop() {
  updateAllInputs();

  handleEmergency();

  if (currentState == STATE_EMERGENCY) {
    updateOutputs();
    return;
  }

  handleStop();
  handleStart();
  handleLevelSensors();
  handlePumpDelay();
  updateOutputs();
}
```

### 처리 순서

```text
1. 입력 상태 업데이트
2. Emergency 처리
3. Emergency 상태 유지 또는 Reset
4. Stop 처리
5. Start 처리
6. Level Sensor 처리
7. Pump Delay 처리
8. Output 처리
```

### 핵심 기준

```text
Emergency 최우선
Emergency 상태에서는 일반 제어 로직 중단
Emergency Reset 후 IDLE로만 복귀
재운전은 START 입력 필요
```

---

## 13. Output by State

출력은 버튼 입력이 아니라 현재 상태 기준으로 결정.

```cpp
digitalWrite(
  PUMP_LED_PIN,
  currentState == STATE_FILLING ? HIGH : LOW
);
```

### 상태별 출력

| State | LOW Lamp | Pump Lamp | Full Lamp | Alarm Lamp |
|---|---|---|---|---|
| IDLE | OFF | OFF | OFF | OFF |
| RUN | OFF | OFF | OFF | OFF |
| PUMP_DELAY | ON | OFF | OFF | OFF |
| FILLING | OFF | ON | OFF | OFF |
| FULL | OFF | OFF | ON | OFF |
| EMERGENCY | OFF | OFF | OFF | ON |

### 설계 방향

```text
Input
→ Event
→ State Change
→ Output
```

---

## 14. Emergency Stop Logic

### Emergency 진입

```text
일반 상태에서 EMERGENCY 버튼 입력
→ EMERGENCY 상태 전환
→ Pump OFF
→ Alarm Lamp ON
```

### Emergency Reset

```text
EMERGENCY 상태에서 EMERGENCY 버튼 다시 입력
→ IDLE 상태 복귀
→ 모든 출력 OFF
→ 자동 재기동 없음
```

### 재시작

```text
IDLE 상태에서 START 버튼 입력
→ RUN 상태 진입
```

---

## 15. Serial Monitor Log Example

영상 촬영용으로 상태 전환 중심 로그 출력.

```text
[START] IDLE -> RUN
  - Monitoring level sensors.

[LOW] RUN -> PUMP_DELAY
  - Pump delay started: 3000ms.

[TIMER] PUMP_DELAY -> FILLING
  - Pump ON. Filling started.

[HIGH] FILLING -> FULL
  - Tank FULL. Pump OFF.

[STOP] FULL -> IDLE
  - System stopped. All outputs OFF.

[EMERGENCY] FILLING -> EMERGENCY
  - Pump OFF / Alarm ON

[RESET] EMERGENCY -> IDLE
  - Emergency reset. Press START to run system.
```

### 로그 설계 기준

```text
입력 Pressed / Released 로그 기본 비활성화
상태 전환 중심 로그 출력
영상 시연 시 가독성 확보
필요 시 DEBUG_INPUT_LOG = true 로 입력 로그 활성화 가능
```

---

## 16. PLC Concept Mapping

GX Works2에서 설계한 PLC 수위 제어 구조를 ESP32 코드 구조로 대응.

| PLC Concept | ESP32 Implementation |
|---|---|
| X 입력 | GPIO Input |
| Y 출력 | GPIO Output |
| M 내부 릴레이 | `currentState` |
| Timer | `millis()` time comparison |
| 자기유지 / 운전 상태 | State Machine |
| Emergency Stop | Highest-priority state transition |

### Example

```text
PLC:
LOW Sensor X input
→ Timer
→ Pump Y output ON

ESP32:
LOW_SENSOR GPIO input
→ STATE_PUMP_DELAY
→ millis() 3 seconds
→ STATE_FILLING
→ PUMP_LED GPIO HIGH
```

### 핵심 변환 구조

```text
PLC Ladder
→ 입력 / 내부 상태 / 타이머 / 출력

Embedded C/C++
→ GPIO Input / State Machine / millis() / GPIO Output
```

---

## 17. Current Status

- [O] PlatformIO project setup
- [O] ESP32-S3 board setting
- [O] Build / Upload success
- [O] Serial Monitor output
- [O] External LED Blink
- [O] Button Input Test
- [O] Button to LED Control
- [O] millis() Timer Test
- [O] Basic State Machine
- [O] Pump Delay Logic
- [O] Emergency Stop Logic
- [O] Full Water Tank Controller
- [O] Demo Video
- [O] Notion Portfolio
- [O] GitHub upload

---

## 18. Troubleshooting Notes

### COM Port Recognition

Issue:

```text
ESP32-S3 board was not recognized as a normal COM port.
USB JTAG/serial debug unit displayed.
```

Action:

```text
Device Manager 확인
USB Serial Device 드라이버 확인
COM port 확인
PlatformIO upload / monitor 재시도
```

---

### COM Port Number Change

Issue:

```text
보드 재접속후 COM port가 변경됨
```

Action:

```text
platformio.ini 에서 업로드 포트, 모니터포트 설정 삭제

```

### GPIO Number Mismatch

Issue:

```text

도면의 GPIO 라벨과 실물의 핀넘버가 틀림

```

Action:

```text

실물의 번호로 다시 체크하여 설정

```

---

### Common GND

Issue:

```text

램프와 버튼이 증가함에 따라 GND를 묶을 필요성이 생김

```

Action:

```text

ESP32 GND를 브레드보드 GND 레일에 연결 후
모든 버튼과 LED를 공통 GND 레일에 연결

```


---

## 19. Project Structure

```text
embedded-water-tank-controller/
 ├─ README.md
 ├─ platformio.ini
 ├─ src/
 │   └─ main.cpp
 ├─ include/
 ├─ lib/
 └─ test/
```

### Main Source

```text
src/main.cpp
```

주요 내용:

```text
Pin Definition
InputState struct
State Machine
Input Debounce
Event Handling
Emergency Logic
Pump Delay Logic
Output Logic
```

---

## 20. Learning Points

```text
GPIO Input
GPIO Output
INPUT_PULLUP
Common GND
Button Debounce
Pressed / Released Event
millis() based non-blocking timer
State Machine
Pump Delay Logic
Emergency Stop
Emergency Reset
Prevention of automatic restart
PLC control concept mapping to embedded C/C++
```

---

## 21. Portfolio Meaning

이 프로젝트의 포트폴리오 의미:

```text
PLC control logic
→ ESP32-S3 hardware I/O
→ C/C++ State Machine
→ Demo Video
→ GitHub / Notion documentation
```

### 핵심 포인트

```text
단순 예제 실행이 아니라 PLC 제어 개념을 임베디드 구조로 재구현
실제 버튼과 LED로 제어 흐름 검증
Serial Monitor 로그와 실제 회로 동작 동시 시연
입력 → 상태 → 타이머 → 출력 → 안전 차단 구조 확인
```

---

## 22. Next Step

```text
실제 수위 센서 적용
릴레이 모듈 기반 실제 Pump 제어
부저 또는 경광등 추가
OLED / LCD 상태 표시
Wi-Fi 기반 상태 모니터링
Japanese README 작성
Japanese portfolio page 작성
```

---

## 23. Summary

```text
GX Works2 PLC 래더 기반 Water Tank Auto Control System
→ ESP32-S3 기반 Embedded Water Tank Controller로 재구현

Input
→ Event
→ State Machine
→ Timer
→ Output
→ Emergency Stop
```

---

## Author

AIFarm-team-MISO

Embedded / PLC / Automation learning project.