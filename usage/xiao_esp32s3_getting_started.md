# Getting Started with Seeed Studio XIAO ESP32-S3 Series

URL: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/

| Seeed Studio XIAO ESP32-S3 | Seeed Studio XIAO ESP32-S3 Sense | Seeed Studio XIAO ESP32-S3 Plus |
| :---: | :---: | :---: |
| ![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/xiaoesp32s3.jpg) | ![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/xiaoesp32s3sense.jpg) | ![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/xiaoesp32s3plus.png) |
| [**Get One Now**](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html) | [**Get One Now**](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) | [**Get One Now**](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Plus-p-6361.html) |

---

## Introduction

Seeed Studio XIAO Series are diminutive development boards, sharing a similar hardware structure, where the size is literally thumb-sized. The code name "XIAO" here represents its half feature "Tiny", and the other half will be "Puissant".

Seeed Studio XIAO ESP32-S3 Sense integrates camera sensor, digital microphone and SD card supporting. Combining embedded ML computing power and photography capability, this development board can be your great tool to get started with intelligent voice and vision AI.

> [!TIP]
> The **OV2640 camera** has been discontinued, and the subsequent XIAO ESP32-S3 Sense uses the **OV3660 camera** model. However, the Wiki example code for the camera still applies.

---

### Specification

| Product | [XIAO ESP32-S3](https://www.seeedstudio.com/XIAO-ESP32S3-p-5627.html) | [XIAO ESP32-S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) | [XIAO ESP32-S3 Plus](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32S3-Plus-p-6361.html) |
| :--- | :--- | :--- | :--- |
| **Processor** | ESP32-S3R8<br>Xtensa LX7 dual-core, 32-bit processor that operates at up to 240 MHz | ESP32-S3R8<br>Xtensa LX7 dual-core, 32-bit processor that operates at up to 240 MHz | ESP32-S3R8<br>Xtensa LX7 dual-core, 32-bit processor that operates at up to 240 MHz |
| **Wireless** | Complete 2.4GHz Wi-Fi subsystem<br>Bluetooth Low Energy 5.0 / Bluetooth Mesh | Complete 2.4GHz Wi-Fi subsystem<br>Bluetooth Low Energy 5.0 / Bluetooth Mesh | Complete 2.4GHz Wi-Fi subsystem<br>Bluetooth Low Energy 5.0 / Bluetooth Mesh |
| **Built-in Sensors** | / | 1x OV3660 camera sensor<br>1x Digital Microphone | / |
| **Memory** | On-chip 8MB PSRAM & 8MB Flash | On-chip 8MB PSRAM & 8MB Flash<br>Onboard SD Card Slot, supporting 32GB FAT | On-chip 8MB PSRAM & 16MB Flash |
| **Interface** | 1x UART<br>1x IIC<br>1x SPI<br>11x GPIO(PWM)<br>9x ADC<br>1x User LED<br>1x Charge LED<br>1x Reset button<br>1x Boot button | 1x UART<br>1x IIC<br>1x IIS<br>1x SPI<br>11x GPIOs (PWM)<br>9x ADC<br>1x User LED<br>1x Charge LED<br>1x B2B Connector (with 2 additional GPIOs)<br>1x Reset button<br>1x Boot button | 2x UART<br>1x IIC<br>1x IIS<br>2x SPI<br>18x GPIOs (PWM)<br>9x ADC<br>1x User LED<br>1x Charge LED<br>1x B2B Connector<br>1x Reset button<br>1x Boot button |
| **Dimensions** | 21 x 17.8mm | 21 x 17.8 x 15mm (with expansion board) | 21 x 17.8mm |
| **Power (Typ.)** | Input voltage (Type-C): 5V<br>Input voltage (BAT): 3.7V | Input voltage (Type-C): 5V<br>Input voltage (BAT): 3.7V | Input voltage (Type-C): 5V<br>Input voltage (BAT): 3.7V |
| **Power Consumption** | Circuit operating Voltage:<br>- Type-C: 5V@19mA<br>- BAT: 3.8V@22mA | Circuit operating Voltage:<br>- Type-C: 5V@38.3mA<br>- BAT: 3.8V@43.2mA (with expansion board) | Circuit operating Voltage:<br>- Type-C: 5V@28mA<br>- BAT: 3.8V@35mA |
| **Webcam App Power** | / | Webcam Web application:<br>- Type-C:<br>-- Average power: 5V/~140mA<br>-- Peak power (Capture): 5V/~347mA<br>- Battery:<br>-- Average power: 3.8V/~155mA<br>-- Peak power (Capture): 3.8V/~366mA | / |
| **Mic & SD App Power** | / | Mic recording & SD card writing:<br>- Type-C:<br>-- Average power: 5V/54.58mA<br>-- Peak power: 5V/86.7mA<br>- Battery:<br>-- Average power: 3.8V/64.5mA<br>-- Peak power: 3.8V/109.3mA | / |
| **Charging Current** | 50mA(Fast) / 3.8mA(Trickle) | / | 100mA(Fast) / 0.9mA(Trickle) |
| **Low Power Modes** | Modem-sleep: 27mA<br>Light-sleep: 2mA<br>Deep Sleep: 14µA | Modem-sleep: 44mA<br>Light-sleep: 5mA<br>Deep Sleep: 3mA | Modem-sleep: 3.8V/31.6mA<br>Light-sleep: 3.8V/2.45mA<br>Deep Sleep: 3.8V/33.51µA |
| **Wi-Fi Power** | Active Mode: 100 mA | Active Mode: 110 mA (with expansion board) | Active Mode: 81 mA |
| **BLE Power** | Active Mode: 85 mA | Active Mode: 102 mA (with expansion board) | Active Mode: 101 mA |
| **Working Temp** | -20°C ~ 65°C | -20°C ~ 65°C | -20°C ~ 65°C |

---

### Features

- **Powerful MCU Board**: Incorporate the ESP32S3 32-bit, dual-core, Xtensa processor chip operating up to 240 MHz, mounted multiple development ports, Arduino / MicroPython supported
- **Advanced Functionality (for Sense)**: Detachable **OV2640 camera sensor for 1600x1200** resolution and **OV3660 camera sensor for 2048x1536** compatible with OV5640 camera sensor, integrating additional digital microphone
- **Elaborate Power Design**: Lithium battery charge management capability, offer 4 power consumption models which allows for deep sleep mode with power consumption as low as 14µA
- **Great Memory for more Possibilities**: Offer 8MB PSRAM and 8MB FLASH (16MB in **Plus** version), supporting SD card slot for external 32GB FAT memory (only for XIAO ESP32-S3 Sense)
- **Outstanding RF performance**: Support 2.4GHz Wi-Fi and BLE dual wireless communication, support 100m+ remote communication when connected with U.FL antenna
- **Thumb-sized Compact Design**: 21 x 17.8mm, adopting the classic form factor of XIAO, suitable for space limited projects like wearable devices

| Item | OV3660 Camera | OV2640 Camera |
| :--- | :--- | :--- |
| **MAX Power Consumption on (640*480)** | Active Model: **~ 0.6A** | Active Model: **~ 0.65A** |
| **AVG Power Consumption on (640*480)** | Active Model: **~ 0.12A** | Active Model: **~ 0.24A** |
| **MIN Power Consumption on (640*480)** | Active Model: **~ 0.12A** | Active Model: **~ 0.15A** |

---

## Hardware Overview

### XIAO ESP32-S3 Front & Back

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/XIAO_ESP32-S3_front_pinout.png)
![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/XIAO_ESP32-S3_back_pinout.png)

#### Pin Map (XIAO ESP32-S3)

| XIAO Pin | Function | Chip Pin | Alternate Functions | Description |
| :--- | :--- | :--- | :--- | :--- |
| **5V** | VBUS | | | Power Input/Output |
| **GND** | | | | Ground |
| **3V3** | 3V3_OUT | | | Power Output |
| **D0** | Analog | GPIO1 | TOUCH1 | GPIO, ADC |
| **D1** | Analog | GPIO2 | TOUCH2 | GPIO, ADC |
| **D2** | Analog | GPIO3 | TOUCH3 | GPIO, ADC |
| **D3** | Analog | GPIO4 | TOUCH4 | GPIO, ADC |
| **D4** | Analog, SDA | GPIO5 | TOUCH5 | GPIO, I2C Data, ADC |
| **D5** | Analog, SCL | GPIO6 | TOUCH6 | GPIO, I2C Clock, ADC |
| **D6** | TX | GPIO43 | | GPIO, UART Transmit |
| **D7** | RX | GPIO44 | | GPIO, UART Receive |
| **D8** | Analog, SCK | GPIO7 | TOUCH7 | GPIO, SPI Clock, ADC |
| **D9** | Analog, MISO | GPIO8 | TOUCH8 | GPIO, SPI Data, ADC |
| **D10** | Analog, MOSI| GPIO9 | TOUCH9 | GPIO, SPI Data, ADC |
| **D11** | Analog | GPIO42 | TOUCH12 | GPIO, ADC |
| **D12** | Analog | GPIO41 | TOUCH13 | GPIO, ADC |
| **MTDO**| | GPIO40 | | JTAG |
| **MTDI**| | GPIO41 | | JTAG, ADC |
| **MTCK**| | GPIO39 | | JTAG, ADC |
| **MTMS**| | GPIO42 | | JTAG, ADC |
| **Reset**| | CHIP_PU | | Reset Pin |
| **Boot** | | GPIO0 | | Enter Boot Mode |
| **U.FL** | | LNA_IN | | UFL antenna |
| **CHARGE_LED** | | | | CHG-LED |
| **USER_LED** | | GPIO21 | | User Light |

---

### XIAO ESP32-S3 Sense Front & Back

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/XIAO_ESP32-S3_Sense_front_pinout.png)
![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/XIAO_ESP32-S3_Sense_back_pinout.png)

#### Pin Map (XIAO ESP32-S3 Sense)

| XIAO Pin | Function | Chip Pin | Alternate Functions | Description |
| :--- | :--- | :--- | :--- | :--- |
| **5V** | VBUS | | | Power Input/Output |
| **GND** | | | | Ground |
| **3V3** | 3V3_OUT | | | Power Output |
| **D0** | Analog | GPIO1 | TOUCH1 | GPIO, ADC |
| **D1** | Analog | GPIO2 | TOUCH2 | GPIO, ADC |
| **D2** | Analog | GPIO3 | TOUCH3 | GPIO, ADC |
| **D3** | Analog | GPIO4 | TOUCH4 | GPIO, ADC |
| **D4** | Analog, SDA | GPIO5 | TOUCH5 | GPIO, I2C Data, ADC |
| **D5** | Analog, SCL | GPIO6 | TOUCH6 | GPIO, I2C Clock, ADC |
| **D6** | TX | GPIO43 | | GPIO, UART Transmit |
| **D7** | RX | GPIO44 | | GPIO, UART Receive |
| **D8** | Analog, SCK | GPIO7 | TOUCH7 | GPIO, SPI Clock, ADC |
| **D9** | Analog, MISO | GPIO8 | TOUCH8 | GPIO, SPI Data, ADC |
| **D10** | Analog, MOSI| GPIO9 | TOUCH9 | GPIO, SPI Data, ADC |
| **D11** | Analog | GPIO42 | TOUCH12 | GPIO, ADC |
| **D12** | Analog | GPIO41 | TOUCH13 | GPIO, ADC |
| **Digital MIC CLK** | | GPIO42 | | PDM clock pin for MIC |
| **Digital MIC DATA**| | GPIO41 | | PDM data pin for MIC |
| **SD Card CS** | | GPIO3 | | SD card chip select pin |
| **SD Card SCK**| | GPIO7 | | SD card clock pin |
| **SD Card MISO**| | GPIO8 | | SD card data input pin |
| **SD Card MOSI**| | GPIO9 | | SD card data output pin |

#### Camera Pin Map

| Chip Pin | Description |
| :--- | :--- |
| **GPIO10** | Camera-related clock pin |
| **GPIO11** | Camera video data pin (Y8) |
| **GPIO12** | Camera video data pin (Y7) |
| **GPIO13** | Camera pixel clock pin |
| **GPIO14** | Camera video data pin (Y6) |
| **GPIO15** | Camera video data pin (Y2) |
| **GPIO16** | Camera video data pin (Y5) |
| **GPIO17** | Camera video data pin (Y3) |
| **GPIO18** | Camera video data pin (Y4) |
| **GPIO40** | I2C data pin for Camera |
| **GPIO39** | I2C clock pin for Camera |
| **GPIO38** | Camera vertical sync pin |
| **GPIO47** | Camera horizontal sync pin |
| **GPIO48** | Camera video data pin (Y9) |

---

### XIAO ESP32-S3 Plus Front & Back

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/XIAO_ESP32-S3_Plus_front_pinout.png)
![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/XIAO_ESP32-S3_Plus_back_pinout.png)

#### Pin Map (XIAO ESP32-S3 Plus)

| XIAO Pin | Function | Chip Pin | Alternate Functions | Description |
| :--- | :--- | :--- | :--- | :--- |
| **5V** | VBUS | | | Power Input/Output |
| **GND** | | | | Ground |
| **3V3** | 3V3_OUT | | | Power Output |
| **D0** ~ **D10** | | GPIO1 ~ GPIO9 | | GPIO, ADC, I2C, SPI, UART |
| **D11** | | GPIO38 | | GPIO, ADC |
| **D12** | | GPIO39 | | GPIO, ADC |
| **D13** | | GPIO40 | | GPIO |
| **D14** | | GPIO41 | | GPIO |
| **D15** | | GPIO42 | | GPIO |
| **D16** | | GPIO10 | | GPIO |
| **D17** | | GPIO13 | | GPIO |
| **D18** | | GPIO12 | | GPIO |
| **D19** | | GPIO11 | | GPIO |
| **ADC_BAT** | | GPIO10 | | Read BAT voltage value |
| **USER_LED**| | GPIO21 | | User Light |

> [!CAUTION]
> - Although the XIAO ESP32-S3 assigns GPIO41 and GPIO42 to pins A11 and A12, due to the nature of the ESP32-S3 chip, pins A11 and A12 do not support ADC functionality. Please be sure to distinguish and differentiate between them.
> - The B2B connector of XIAO ESP32-S3 Plus is compatible with [Wio-SX1262 extension board](https://www.seeedstudio.com/Wio-SX1262-with-XIAO-ESP32S3-p-5982.html) but not with Plug-in camera sensor board.

---

### Power Pins

- **5V** - This is 5V out from the USB port. You can also use this as a voltage input but you must have some sort of diode (schottky, signal, power) between your external power source and this pin with anode to battery, cathode to 5V pin.
- **3V3** - This is the regulated output from the onboard regulator. You can draw up to 700mA.
- **GND** - Power/data/signal ground.

---

### Strapping Pins

At each startup or reset, a chip requires some initial configuration parameters, such as in which boot mode to load the chip, voltage of flash memory, etc. These parameters are passed over via the strapping pins. After reset, the strapping pins operate as regular IO pins.

The parameters controlled by the given strapping pins at chip reset are as follows:
- **Chip boot mode**: GPIO0 and GPIO46
- **VDD_SPI voltage**: GPIO45
- **ROM messages printing**: GPIO46
- **JTAG signal source**: GPIO3

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/110.png)
![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/111.png)
![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/112.png)

---

## Getting Started

### 1. XIAO ESP32-S3
The factory program preset in the regular version is the touch pin light-up program. When you power up the XIAO, touch some of its pins and the orange user indicator will light up.

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/105.jpg)

### 2. XIAO ESP32-S3 Sense
The XIAO ESP32-S3 Sense is shipped with the WebCam sample program pre-installed. You can use this program by giving the XIAO a good antenna installation and powering it up.

> [!NOTE]
> Starting from **June 2025**, the factory firmware of XIAO ESP32-S3 Sense enables a default AP Wi-Fi with the following credentials:
> - **SSID:** `XIAO_ESP32S3_Sense`
> - **Password:** `seeedstudio`

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/98.png)

---

### Hardware Preparation

#### 1. Solder Header
XIAO ESP32-S3 is shipped without pin headers by default. You need to prepare your own pin headers and solder them to the corresponding pins.

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/4.jpg)

#### 2. Installation of Antenna
On the bottom left of the front of XIAO ESP32-S3, there is a separate "WiFi/BT Antenna Connector". Put one side of the antenna connector into the connector block first, then press down a little on the other side.

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/5.gif)

#### 3. Installation of Expansion Board (for Sense)
Align the connector on the expansion board with the B2B connector on the XIAO ESP32-S3, press firmly until hearing a "click".

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/61.gif)

> [!WARNING]
> **Do not pry the boards apart vertically.** Always separate the boards by pushing or sliding from the **side**, keeping the motion parallel to the connector.

---

### Software Preparation (Arduino IDE)

1. **Download & Install Arduino IDE** ([Arduino Software Download](https://www.arduino.cc/en/software))
2. **Add ESP32 Board Package URL**:
   Navigate to **File > Preferences**, and fill **Additional Boards Manager URLs** with:
   ```text
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Install ESP32 Package**:
   Navigate to **Tools > Board > Boards Manager...**, search for `esp32`, select version **2.0.8 or above**, and install it.
4. **Select Board & Port**:
   - Board: **XIAO_ESP32S3**
   - Port: Select the corresponding COM port.

---

### BootLoader Mode

If uploading fails or the port is not recognized:
- **Step 1.** Press and hold the `BOOT` button on the XIAO ESP32-S3.
- **Step 2.** Keep `BOOT` pressed while connecting the USB cable to PC, then release `BOOT`.
- **Step 3.** Upload **File > Examples > 01.Basics > Blink**.

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/15.gif)

---

### Battery Usage

- Connect 3.7V rechargeable lithium battery.
- **Negative (-)**: Side closest to the USB port.
- **Positive (+)**: Side away from the USB port.

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/16.jpg)
![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/17.png)

> [!CAUTION]
> When using battery power, there is **no voltage output on the 5V pin**.

#### Battery Charge LED Indicator:
1. No battery connected + USB plugged in: Red light on, turns off after 30s.
2. Battery connected + Charging: Red light flashes.
3. Battery fully charged: Red light turns off.

![](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/img/104.jpg)

---

## Resources & Links

- **Datasheet**: [Espressif ESP32-S3 Datasheet (PDF)](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/res/esp32-s3_datasheet.pdf)
- **Schematic**: [XIAO ESP32-S3 Schematic (PDF)](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/new-res/202003751_XIAO%20ESP32S3_v1.4_SCH_260226.pdf.pdf)
- **Sense Schematic**: [XIAO ESP32-S3 Sense Schematic (PDF)](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/new-res/202003753_XIAO%20ESP32S3%20Sense_v1.5_SCH_260226.pdf.pdf)
- **Pinout Sheet**: [XIAO ESP32-S3 Pinout Excel](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/res/XIAO_ESP32S3_Sense_Pinout.xlsx)
- **Factory Firmware**: [XIAO ESP32-S3 Factory Firmware ZIP](https://files.seeedstudio.com/wiki/SeeedStudio-XIAO-ESP32S3/res/XIAO-ESP32S3-firmware-20240814.zip)
