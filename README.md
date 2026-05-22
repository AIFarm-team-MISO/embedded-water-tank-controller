# Embedded Water Tank Controller

ESP32-S3 기반의 수위 자동 제어 시스템 프로젝트입니다.

기존에 PLC로 구현했던 `Water Tank Auto Control System`을  
임베디드 C/C++ 구조로 다시 구현하면서, GPIO 입출력, 상태머신, 타이머, 비상정지 로직을 학습하고 정리하는 것을 목표로 합니다.

---

## 1. Project Overview

이 프로젝트는 물탱크 자동 제어 시스템을 임베디드 방식으로 구현하는 학습형 포트폴리오입니다.

기본 제어 흐름은 다음과 같습니다.

```text
입력
→ 상태 판단
→ 타이머 처리
→ 출력 제어
→ 비상정지 및 안전 처리
```

PLC에서 사용했던 제어 사고를 ESP32-S3 기반 임베디드 구조로 변환하면서,  
전기·PLC·임베디드 제어의 연결 구조를 이해하는 것을 목표로 합니다.

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

---

## 3. Hardware

현재 사용 중인 주요 부품은 다음과 같습니다.

- ESP32-S3 개발보드
- USB-C 데이터 케이블
- 브레드보드
- Dupont 점퍼 케이블
- 5mm LED
- 330Ω 저항
- 10kΩ 저항
- 택트 스위치
- 단자대 보드

추후 프로젝트 진행에 따라 펌프, 릴레이, 수위 센서 등을 가정하거나 추가할 예정입니다.

---

## 4. Current Status

현재 완료된 내용입니다.

- [x] PlatformIO 프로젝트 생성
- [x] ESP32-S3 보드 설정
- [x] Build 성공
- [x] Upload 성공
- [x] Serial Monitor 출력 확인
- [x] USB CDC Serial 동작 확인
- [x] GitHub 저장소 연결
- [x] 첫 번째 push 완료

현재 테스트 코드에서는 `setup()`에서 초기 로그를 출력하고,  
`loop()`에서 1초마다 실행 상태를 Serial Monitor에 출력합니다.

예상 출력 예시:

```text
=================================
ESP32-S3 project started
Embedded Water Tank Controller
VERSION: SERIAL_TEST_003
=================================
Running SERIAL_TEST_003...
```

---

## 5. Project Goals

이 프로젝트의 최종 목표는 다음과 같습니다.

- GPIO Output 이해
- GPIO Input 이해
- 버튼 입력 처리
- LED 출력 제어
- millis() 기반 non-blocking timer 구현
- State Machine 구조 설계
- Pump Delay Logic 구현
- Emergency Stop 우선 처리
- PLC 제어 구조와 임베디드 제어 구조 비교
- GitHub / Notion 기반 포트폴리오 정리

---

## 6. Control Concept

기본 제어 개념은 다음과 같습니다.

### Input

| 입력 | 설명 |
|---|---|
| Start Button | 시스템 운전 시작 |
| Stop Button | 시스템 정지 |
| LOW Sensor | 수위 낮음 감지 |
| HIGH Sensor | 수위 높음 감지 |
| Emergency Button | 비상 정지 |

### Output

| 출력 | 설명 |
|---|---|
| Pump | 펌프 동작 |
| Alarm | 비상 또는 이상 상태 표시 |
| LED | 상태 표시용 출력 |

### State

| 상태 | 설명 |
|---|---|
| IDLE | 대기 상태 |
| RUN | 시스템 운전 상태 |
| FILLING | 펌프 동작 상태 |
| FULL | 만수 상태 |
| EMERGENCY | 비상 정지 상태 |

---

## 7. Development Roadmap

앞으로의 진행 계획입니다.

- [x] Serial Monitor Test
- [ ] External LED Blink
- [ ] GPIO Output Test
- [ ] Button Input Test
- [ ] Button to LED Control
- [ ] millis() Timer Test
- [ ] Basic State Machine
- [ ] Pump Delay Logic
- [ ] Emergency Stop Logic
- [ ] Full Water Tank Controller
- [ ] Wiring Diagram
- [ ] Demonstration Video
- [ ] README Update
- [ ] README_JP 작성

---

## 8. Project Structure

현재 프로젝트 구조는 다음과 같습니다.

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

추후 문서와 이미지가 추가되면 다음과 같이 확장할 예정입니다.

```text
embedded-water-tank-controller/
 ├─ README.md
 ├─ README_JP.md
 ├─ platformio.ini
 ├─ src/
 │   └─ main.cpp
 ├─ docs/
 │   ├─ 01_serial_monitor_test.md
 │   ├─ 02_gpio_output_led.md
 │   ├─ 03_button_input.md
 │   ├─ 04_state_machine.md
 │   └─ troubleshooting.md
 └─ images/
     ├─ serial_monitor_result.png
     ├─ wiring_led_test.png
     └─ control_flow_diagram.png
```

---

## 9. Troubleshooting Notes

초기 개발환경 구축 과정에서 다음 문제를 확인했습니다.

### 1. COM Port 인식 문제

일부 USB 포트에서 업로드가 실패하거나 COM 포트가 인식되지 않는 문제가 있었습니다.

해결:

- 다른 USB-C 포트 사용
- PC 재부팅
- 장치 관리자에서 COM 포트 확인

### 2. setup() 출력 누락 문제

Serial Monitor에서 `loop()` 출력만 보이고 `setup()` 출력이 보이지 않는 문제가 있었습니다.

원인:

- Monitor 연결 시 자동 리셋이 발생하지 않아, `setup()` 실행 시점이 이미 지나간 상태였음

해결:

- `platformio.ini`에서 RTS / DTR 관련 설정 제거
- Serial Monitor 연결 시 초기 출력 정상 확인

### 3. Native USB 구조 이해

ESP32-S3는 Native USB 구조를 사용할 수 있으므로,  
일반 UART 기반 보드와 업로드 및 Serial Monitor 동작 방식이 다를 수 있습니다.

---

## 10. Learning Purpose

이 프로젝트는 단순한 예제 구현이 아니라,  
PLC 제어 구조와 임베디드 제어 구조를 비교하며 학습하기 위한 프로젝트입니다.

특히 다음 관점을 중점적으로 학습합니다.

- PLC의 자기유지 회로를 임베디드 상태변수로 표현
- PLC 타이머를 millis() 기반 타이머로 변환
- 센서 입력을 GPIO Input으로 처리
- 출력 릴레이를 GPIO Output으로 추상화
- 비상정지 조건을 최우선 로직으로 설계
- 순차 제어를 State Machine으로 구조화

---

## 11. Portfolio Direction

이 프로젝트는 전기·PLC·임베디드 제어를 연결하는 포트폴리오로 정리합니다.

핵심 방향은 다음과 같습니다.

```text
PLC 제어 사고
+ 전기 입출력 이해
+ 임베디드 C/C++ 구현
+ 상태머신 설계
+ GitHub 기반 기록
```

최종적으로는 단순 코드 예제가 아니라,  
제어 시스템을 구조적으로 이해하고 구현할 수 있음을 보여주는 프로젝트로 정리하는 것을 목표로 합니다.

---

## 12. Repository

GitHub Repository:

```text
https://github.com/AIFarm-team-MISO/embedded-water-tank-controller
```

---

## 13. Author

AIFarm-team-MISO

Embedded / PLC / Automation learning project.