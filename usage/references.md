# Component Reference Links & Notes

프로젝트 부품 관련 데이터시트 및 주요 스펙 요약 목록입니다.

## 📌 부품 목록 및 데이터시트

### 1. Main Controller: ESP32-WROOM-32
- **데이터시트**: [`esp32-wroom-32_datasheet_en.pdf`](file:///C:/art_tech/usage/esp32-wroom-32_datasheet_en.pdf)
- **용도**: 메인 메인보드 / 마이크로컨트롤러 (Wi-Fi & Bluetooth 지원, Dual Core Xtensa 32-bit LX6)
- **주요 핀 인터페이스**:
  - I2C (SDA / SCL): PCA9685 드라이버 통신에 사용 (기본 GPIO21: SDA, GPIO22: SCL)
  - PWM / GPIO outputs

### 2. Servo Driver: PCA9685 16-Channel PWM/Servo Driver
- **데이터시트**: [`16-channel-pwm-servo-driver.pdf`](file:///C:/art_tech/usage/16-channel-pwm-servo-driver.pdf)
- **용도**: 최대 16개의 서보 모터를 I2C 통신(2핀)만으로 제어할 수 있는 PWM 드라이버 모듈
- **주요 스펙 및 특징**:
  - 통신 방식: I2C (기본 주소 `0x40`)
  - 분해능: 12-bit PWM (4096 단계)
  - 주파수: 24Hz ~ 1526Hz (서보 모터 제어 표준 50Hz 사용)
  - 전원: VCC (논리 전원 3.3V/5V), V+ (서보 모터 전원 입력 5V~6V 권장)

### 3. Servo Motor: MG996R
- **데이터시트**: [`MG996R.pdf`](file:///C:/art_tech/usage/MG996R.pdf)
- **용도**: 고토크 메탈 기어 서보 모터 (관절, 관절 구동 및 메카닉 액추에이터용)
- **주요 스펙 및 특징**:
  - 작동 전압: 4.8V ~ 7.2V (권장 5V~6V 외부 전원)
  - 토크: 약 9.4 kg·cm (4.8V) / 11 kg·cm (6.0V)
  - 제어 신호: PWM (50Hz / 20ms 주기, 펄스 폭 약 0.5ms~2.5ms 또는 1ms~2ms)
  - 핀 구조: Brown(GND), Red(VCC +5V~6V), Orange/Yellow(PWM Signal)

---

*위 부품 스펙을 바탕으로 ESP32 ↔ PCA9685 ↔ MG996R 연결 및 제어 코드를 최적화하여 작성합니다.*
