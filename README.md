# Embedded Water Tank Controller

ESP32-S3 기반의 수위 자동 제어 시스템 프로젝트입니다.

PLC 기반 `Water Tank Auto Control System`의 제어 사고를 임베디드 C/C++ 구조로 재구현하며, GPIO 입력/출력, 상태머신, `millis()` 기반 타이머, Pump Delay, Emergency Stop 로직을 학습하고 구현하는 것을 목표로 합니다.

---

## 1. Project Overview

### Main Flow

```text
Start
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
→ Start
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
| Current Version | FULL_WATER_TANK_004 |

---

## 3. Hardware

- ESP32-S3 개발보드
- 브레드보드
- Dupont 점퍼 케이블
- 택트 스위치 5개
- LED 4개
- 10kΩ 저항
- USB-C 데이터 케이블

> 현재 LED에는 10kΩ 저항 사용.  
> LED 밝기는 약하게 표시됨.  
> 추후 330Ω 또는 470Ω 저항 사용 예정.

---

## 4. Pin Assignment

### Input Buttons

입력 방식: `INPUT_PULLUP`

```text
Released = HIGH
Pressed  = LOW
```

| 색상 | 역할 | GPIO | 연결 |
|---|---|---:|---|
| Green | START | GPIO4 | GPIO4 → Button → GND |
| Red | STOP | GPIO6 | GPIO6 → Button → GND |
| Yellow | EMERGENCY | GPIO5 | GPIO5 → Button → GND |
| Blue | LOW Sensor | GPIO7 | GPIO7 → Button → GND |
| White | HIGH Sensor | GPIO8 | GPIO8 → Button → GND |

---

### Output LEDs

LED 배열:

```text
[Blue LOW] → [Red PUMP] → [White FULL] → [Yellow ALARM]
```

| 색상 | 역할 | GPIO | ON 조건 |
|---|---|---:|---|
| Blue | LOW / Pump Delay | GPIO12 | PUMP_DELAY |
| Red | Pump / Filling | GPIO10 | FILLING |
| White | Full | GPIO13 | FULL |
| Yellow | Emergency / Alarm | GPIO11 | EMERGENCY |

---

## 5. State Machine

| 상태 | 의미 | 출력 |
|---|---|---|
| IDLE | 대기 | All OFF |
| RUN | 운전 / 센서 감시 | All OFF |
| PUMP_DELAY | LOW 감지 후 Pump ON 대기 | Blue LED ON |
| FILLING | Pump ON / 급수 중 | Red LED ON |
| FULL | 만수 | White LED ON |
| EMERGENCY | 비상 정지 | Yellow LED ON / Pump OFF |

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

---

## 7. Current Status

- [x] PlatformIO 프로젝트 생성
- [x] ESP32-S3 보드 설정
- [x] Build 성공
- [x] Upload 성공
- [x] Serial Monitor 출력 확인
- [x] USB CDC Serial 동작 확인
- [x] GitHub 저장소 연결
- [x] External LED Blink
- [x] Button Input Test
- [x] Button to LED Control
- [x] millis() Timer Test
- [x] Basic State Machine
- [x] Pump Delay Logic
- [x] Emergency Stop Logic
- [x] Full Water Tank Controller

---

## 8. Development Roadmap

- [x] Serial Monitor Test
- [x] External LED Blink
- [x] GPIO Output Test
- [x] Button Input Test
- [x] Button to LED Control
- [x] millis() Timer Test
- [x] Basic State Machine
- [x] Pump Delay Logic
- [x] Emergency Stop Logic
- [x] Full Water Tank Controller
- [ ] Wiring Diagram
- [ ] Demonstration Video
- [ ] README_JP 작성
- [ ] Notion Portfolio 정리

---

## 9. Project Structure

```text
embedded-water-tank-controller/
 ├─ README.md
 ├─ platformio.ini
 ├─ src/
 │   └─ main.cpp
 ├─ include/
 ├─ lib/
 ├─ test/
 └─ docs/
```

Planned structure:

```text
embedded-water-tank-controller/
 ├─ README.md
 ├─ README_JP.md
 ├─ platformio.ini
 ├─ src/
 │   └─ main.cpp
 ├─ docs/
 │   ├─ 01_serial_monitor_test.md
 │   ├─ 02_external_led_blink.md
 │   ├─ 03_button_input_test.md
 │   ├─ 04_button_to_led_control.md
 │   ├─ 05_millis_timer.md
 │   ├─ 06_basic_state_machine.md
 │   ├─ 07_pump_delay_logic.md
 │   ├─ 08_emergency_stop_logic.md
 │   └─ 09_full_water_tank_controller.md
 └─ images/
     ├─ wiring_full_controller.png
     ├─ serial_monitor_result.png
     └─ demo_screenshot.png
```

---

## 10. Key Concepts

### GPIO Input

```cpp
pinMode(START_BUTTON_PIN, INPUT_PULLUP);
```

```text
Released = HIGH
Pressed  = LOW
```

---

### GPIO Output

```cpp
digitalWrite(PUMP_LED_PIN, HIGH);
digitalWrite(PUMP_LED_PIN, LOW);
```

---

### millis() Timer

```cpp
if (currentMillis - pumpDelayStartTime >= PUMP_DELAY_TIME) {
  changeState(STATE_FILLING);
}
```

용도:

- Pump Delay
- Non-blocking timer
- Emergency 우선 처리 가능

---

### State Machine

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

구조:

```text
Input
→ State
→ Output
```

---

## 11. PLC Concept Mapping

| PLC 개념 | ESP32 구현 |
|---|---|
| X 입력 | GPIO Input |
| Y 출력 | GPIO Output |
| M 내부 릴레이 | `currentState` |
| Timer | `millis()` 시간 비교 |
| 자기유지 / 운전 상태 | State Machine |
| Emergency Stop | 최우선 상태 전환 |

Example:

```text
PLC:
LOW Sensor X 입력
→ Timer 동작
→ Pump Y 출력 ON

ESP32:
LOW_SENSOR GPIO 입력
→ STATE_PUMP_DELAY
→ millis() 3초 경과
→ STATE_FILLING
→ PUMP_LED GPIO HIGH
```

---

## 12. Test Scenario

### Normal Flow

```text
1. START
   IDLE → RUN

2. LOW Sensor
   RUN → PUMP_DELAY
   Blue LED ON

3. 3초 경과
   PUMP_DELAY → FILLING
   Blue LED OFF
   Red LED ON

4. HIGH Sensor
   FILLING → FULL
   Red LED OFF
   White LED ON
```

---

### Refill Flow

```text
1. FULL 상태

2. LOW Sensor
   FULL → PUMP_DELAY
   Blue LED ON

3. 3초 경과
   PUMP_DELAY → FILLING
   Red LED ON

4. HIGH Sensor
   FILLING → FULL
   White LED ON
```

---

### Stop Flow

```text
RUN / PUMP_DELAY / FILLING / FULL
  ↓ STOP
IDLE
```

---

### Emergency Flow

```text
1. EMERGENCY
   Any State → EMERGENCY
   Pump OFF
   Yellow LED ON

2. EMERGENCY again
   EMERGENCY → IDLE
   Yellow LED OFF

3. START
   IDLE → RUN
```

---

## 13. Troubleshooting Notes

### COM Port 인식

증상:

```text
USB JTAG/serial debug unit
COM 포트 미표시
```

처리:

- 장치 관리자 확인
- USB Serial Device 드라이버 확인
- PlatformIO Upload / Monitor 재시도

---

### COM Port 번호 변경

처리:

- `upload_port`, `monitor_port` 고정 설정 제거
- 필요 시에만 임시 지정

---

### LED 밝기 부족

원인:

```text
10kΩ 저항 사용
```

처리:

- 위쪽에서 LED 점등 확인
- 추후 330Ω 또는 470Ω 사용 예정

---

### GPIO 번호 혼동

원인:

```text
도면 표시와 실제 보드 실크 인쇄 차이
```

처리:

- 실제 보드의 GPIO 번호 기준으로 연결
- Serial Monitor 입력 로그로 검증

---

### GND 부족

처리:

```text
ESP32 GND
→ 브레드보드 공통 GND 레일
→ 모든 버튼 / LED GND 공유
```

---

## 14. Learning Purpose

학습 항목:

- GPIO 입력 처리
- GPIO 출력 제어
- 버튼 디바운스
- INPUT_PULLUP 구조
- 공통 GND 구성
- millis() 기반 non-blocking timer
- State Machine 설계
- Pump Delay Logic
- Emergency Stop 우선 처리
- 자동 재기동 방지
- PLC 제어 사고와 임베디드 코드의 대응

---

## 15. Portfolio Direction

핵심 방향:

```text
PLC 제어 사고
+ 전기 입출력 이해
+ 임베디드 C/C++ 구현
+ 상태머신 설계
+ GitHub 기반 기록
```

목표:

```text
단순 예제 구현
→ 제어 시스템 구조 이해
→ 입력 / 상태 / 출력 기반 제어 로직 구현
→ 포트폴리오화
```

---

## 16. Repository

```text
https://github.com/AIFarm-team-MISO/embedded-water-tank-controller
```

---

## 17. Author

AIFarm-team-MISO

Embedded / PLC / Automation learning project.