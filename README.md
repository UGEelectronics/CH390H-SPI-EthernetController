# CH390H SPI to Ethernet Controller Module

Compact **10/100 Mbps Ethernet** expansion for microcontrollers over **SPI**. This repository contains the module documentation, schematic, PCB artwork, interactive BOM, an ESP32-S3 ESP-IDF example, and an Arduino-ESP32 example.

<p align="center">
  <img src="IMGs/CH390H%20Module2.jpeg" alt="CH390H SPI to Ethernet final assembled module" width="520">
</p>

Available as a **fully assembled module** or as an **empty PCB** for DIY soldering. It is intended for electronics enthusiasts, embedded developers, and students who need a small SPI Ethernet interface.

## Buy options

- Full assembled module: [Buy here](https://uge-one.com/?post_type=product&p=169848&preview=true)
- Empty PCB for DIY soldering: [Buy here](https://uge-one.com/product/pcb-for-ch390h-spi-ethernet-controller-module/)

## Compact size advantage

Compared with common SPI Ethernet modules such as **CH392F** and **W5500**, the **CH390H module is the smallest** — ideal when board space is limited.

<p align="center">
  <img src="IMGs/Ethernet%20Modules.jpeg" alt="Size comparison: CH390H (smallest), CH392F, and W5500 Ethernet modules" width="720">
</p>

<p align="center"><em>Left to right: CH390H (smallest), CH392F, W5500</em></p>

## Key features

- WCH **CH390H** industrial Ethernet controller (MAC + PHY)
- **Smallest footprint** among typical CH390H / CH392F / W5500 SPI Ethernet modules
- **SPI** host interface to an external MCU
- **10/100 Mbps** Ethernet, IEEE 802.3u class PHY
- Standard **RJ45** with integrated magnetics
- **25 MHz** crystal oscillator
- Dedicated **INT** interrupt output
- Onboard Ethernet link/activity LEDs
- **3.3 V** logic, compact PCB, labeled 7-pin SPI header
- Works with **Arduino, STM32, ESP32**, and other SPI-capable MCUs
- No external Ethernet PHY is required when the module is fully assembled

## Technical specifications

| Specification | Detail |
| --- | --- |
| Ethernet controller | CH390H (QFN) |
| Host interface | SPI + interrupt |
| Ethernet speed | 10/100 Mbps |
| Supply / logic | 3.3 V |
| Crystal | 25 MHz |
| Ethernet connector | RJ45 with integrated magnetics |
| MCU header | 7-pin, 2.54 mm |
| Status indicators | Ethernet link/activity LEDs |
| Application | MCU Ethernet expansion |

Electrical limits, SPI timing, and supported Ethernet modes must be taken from the [CH390H datasheet](https://www.wch.cn/downloads/CH390DS1_PDF.html) for the chip revision you use.

## SPI interface

The module uses a 7-pin header. **There is no RST pin** on this PCB. That is normal: the driver resets the CH390 in software. In sketches use `PIN_RST -1`.

| Pin | Function | Description |
| --- | --- | --- |
| CS | Chip Select | SPI chip-select input |
| SCK | SPI Clock | SPI clock |
| MOSI | SPI MOSI | MCU → CH390H data |
| MISO | SPI MISO | CH390H → MCU data |
| INT | Interrupt | Ethernet interrupt (optional) |
| 3V3 | Power | 3.3 V only — not 5 V |
| GND | Ground | Common ground with the MCU |

### Connect to ESP32

**3.3 V only.** Common GND. Do not connect 5 V to 3V3.

| CH390H module | ESP32 (DevKit / WROOM) | ESP32-S3 |
| --- | --- | --- |
| CS | **GPIO 5** | **GPIO 10** |
| SCK | **GPIO 18** (default SCK) | **GPIO 12** (default SCK) |
| MOSI | **GPIO 23** (default MOSI) | **GPIO 11** (default MOSI) |
| MISO | **GPIO 19** (default MISO) | **GPIO 13** (default MISO) |
| INT | **GPIO 4** | **GPIO 14** |
| 3V3 | 3.3 V | 3.3 V |
| GND | GND | GND |
| RST | *not on module* → `PIN_RST -1` | `PIN_RST -1` |

Arduino examples (ESP32):

```cpp
#define PIN_CS   5
#define PIN_INT  4
#define PIN_RST  -1   // this module has no reset pin

CH390.init(PIN_CS, PIN_INT, PIN_RST);
```

Arduino examples (ESP32-S3):

```cpp
#define PIN_CS   10
#define PIN_INT  14
#define PIN_RST  -1

CH390.init(PIN_CS, PIN_INT, PIN_RST);
```

SCK / MOSI / MISO are the board default SPI pins, so you do not pass them to `init()` unless you rewired SPI. INT is optional: set `PIN_INT` to `-1` to poll if you leave INT unconnected.

### Typical MCU connection

| CH390H module | MCU |
| --- | --- |
| CS | SPI CS |
| SCK | SPI SCK |
| MOSI | SPI MOSI |
| MISO | SPI MISO |
| INT | GPIO / interrupt input |
| 3V3 | 3.3 V |
| GND | GND |

CS, SCK, MOSI, and MISO form the SPI bus. INT can go to an MCU GPIO for interrupt-driven Ethernet events. INT is optional depending on software, but using it is recommended.

```
MCU                         CH390H module                 Network
┌──────────────┐            ┌──────────────┐              ┌────────┐
│          SCK ├───────────►│ SCK          │              │        │
│         MOSI ├───────────►│ MOSI         │   Ethernet   │  RJ45  │
│         MISO │◄───────────┤ MISO         ├─────────────►│        │
│           CS ├───────────►│ CS           │              └────────┘
│          INT │◄───────────┤ INT          │
│          3V3 ├───────────►│ 3V3          │
│          GND ├────────────┤ GND          │
└──────────────┘            └──────────────┘
```

## Ethernet interface

The Ethernet section includes:

- RJ45 connector with integrated transformer/magnetics
- Ethernet differential-pair routing
- Link/activity LEDs
- 75 Ω termination network
- CH390H Ethernet MAC/PHY

Use a standard Ethernet cable to connect the MCU system to a switch, router, PC, or other Ethernet device.

## Schematic and BOM

<p align="center">
  <img src="Module_Schematic.png" alt="CH390H module schematic" width="720">
</p>

- Schematic: [`Module_Schematic.png`](Module_Schematic.png)
- Interactive BOM (opens in the browser): [https://ugeelectronics.github.io/CH390H-SPI-EthernetController/bom.html](https://ugeelectronics.github.io/CH390H-SPI-EthernetController/bom.html)
- Interactive BOM source file: [`InteractiveBOM_CH390H Module.html`](InteractiveBOM_CH390H%20Module.html) (too large for GitHub’s file viewer; use the link above)
- Full write-up: [`Module Specs.odt`](Module%20Specs.odt)

## Onboard components

- CH390H Ethernet controller
- 25 MHz crystal oscillator
- RJ45 with integrated transformer
- Decoupling capacitors and Ethernet termination
- SPI interface header
- Ethernet status LEDs
- 3.3 V power connections

## ESP32 example firmware

`CH390H_ESP32/s3-ch390h` is an ESP-IDF example that brings up the CH390H over SPI, assigns a static Ethernet IP, and includes a TCP server path.

Default SPI pin mapping in `sdkconfig` (ESP32-S3):

| Signal | GPIO |
| --- | --- |
| SCLK | 12 |
| MOSI | 11 |
| MISO | 13 |
| CS | 10 |
| INT | 14 |
| SPI clock | 72 MHz |

Example Ethernet address used by the firmware: **192.168.1.10 / 255.255.255.0**, gateway **192.168.1.1**.

Build with [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html) from `CH390H_ESP32/s3-ch390h`:

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```

Driver usage details are in [`CH390H_ESP32/s3-ch390h/components/ch390/README.md`](CH390H_ESP32/s3-ch390h/components/ch390/README.md). Downloaded IDF components in `managed_components/` are omitted from this repo and will be restored by the IDF Component Manager from `idf_component.yml`.

## Arduino IDE (ESP32, no ESP-IDF project)

CH390H is a SPI **MAC + PHY**. It is **not** a W5500, so the built-in Arduino-ESP32 `ETH.begin(ETH_PHY_W5500, ...)` path and the classic Arduino `Ethernet.h` (W5100/W5500) library will not work.

Use the Arduino-ESP32 core plus the [ESP32-CH390H-lib](https://github.com/UGEelectronics/ESP32-CH390H-lib) library. That library is an Arduino port of the same CH390 driver and plugs into the ESP32 network stack, so `WebServer`, `WiFiClient`, and `HTTPClient` work over Ethernet the same way they do over Wi-Fi.

### Arduino IDE setup

1. Install **esp32 by Espressif** from Boards Manager (Arduino-ESP32 2.x or 3.x).
2. Install the CH390 library from the standalone repo: [UGEelectronics/ESP32-CH390H-lib](https://github.com/UGEelectronics/ESP32-CH390H-lib). Use **Sketch → Include Library → Add .ZIP Library…** with the ZIP from that repo, or clone it into `Documents/Arduino/libraries/ESP32-CH390`.
3. Open **File → Examples → ESP32-CH390** and pick a sketch. Every example starts with:

```cpp
#define PIN_CS   5     // ESP32-S3: 10
#define PIN_INT  4     // ESP32-S3: 14
#define PIN_RST  -1    // module has no RST pin

CH390.init(PIN_CS, PIN_INT, PIN_RST);
if (!CH390.begin()) {
  Serial.println("CH390 not detected on SPI");
  while (1) {}
}
```

Use **GPIO 5 / 4** on ESP32 and **GPIO 10 / 14** on ESP32-S3, with `PIN_RST -1`. MOSI/MISO/SCK are the default SPI pins (see **Connect to ESP32** above).

| Example | Feature (same as W5500 / typical SPI Ethernet chips) |
| --- | --- |
| `Basic` / `StaticIP` | SPI hardware test, then DHCP or fixed IP |
| `WebServer` | HTTP page, JSON status, GPIO |
| `TcpEchoServer` / `TcpClient` | TCP server and client |
| `UdpNtpClient` / `UdpSendReceive` | NTP and UDP packets |
| `MdnsWebServer` | `http://ch390h.local/` |
| `HttpsClient` | TLS client (ESP32 stack) |
| `ArduinoOTA` | Firmware update over Ethernet |
| `SerialBridge` | UART ↔ TCP (telnet port 23) |
| `LinkMonitor` | Link/PHY registers; poll mode if INT is unused |
| `MqttClient` | MQTT (install PubSubClient) |
| `Advanced` | Custom SPI pins, MAC, PHY dump, HTTP GET |

There is also a standalone sketch at [`CH390H_Arduino/CH390H_ESP32_Ethernet/CH390H_ESP32_Ethernet.ino`](CH390H_Arduino/CH390H_ESP32_Ethernet/CH390H_ESP32_Ethernet.ino).

### Wiring notes

The 7-pin header has **no RST**. Leave `PIN_RST` at `-1`. INT is optional; set `PIN_INT` to `-1` to poll. Keep SPI around **20 MHz** in Arduino (max about 33 MHz). No MDC/MDIO: the PHY is inside the CH390H.

### Sketch outline

```cpp
#define PIN_CS   5
#define PIN_INT  4
#define PIN_RST  -1

#include "ESP32_CH390.h"

void setup() {
  Serial.begin(115200);
  CH390.init(PIN_CS, PIN_INT, PIN_RST);
  if (!CH390.begin()) {
    Serial.println("CH390 not detected on SPI");
    while (1) {}
  }
}
```

If `CH390.begin()` fails, check 3.3 V, common GND, and that CS/SCK/MOSI/MISO are not swapped. The RJ45 link LED should light when the cable is in a live switch or router.

## Applications

- STM32 / ESP32 / Arduino Ethernet expansion
- Industrial controllers, PLC, and automation
- Embedded web servers and remote monitoring
- IoT gateways and network-connected sensors
- Data acquisition and robotics
- Custom boards that need Ethernet without an on-chip MAC

SPI Ethernet is useful when the MCU has no convenient Ethernet MAC: the host only needs SPI plus one optional interrupt GPIO.

## Package contents

- 1 × CH390H SPI to Ethernet Controller PCB

Ethernet cable and MCU are not included unless specifically stated.

## Notes

- The module requires a **3.3 V** supply. Connect module GND to MCU GND.
- Wire SPI according to the MCU peripheral you use.
- Firmware/library support is required to talk to the CH390H.
- This is an Ethernet **controller module**, not a standalone Ethernet-to-USB adapter.

## Repository layout

```
.
├── README.md
├── Module Specs.odt
├── Module_Schematic.png
├── PCB For CH390H Ethernet Module.jpg
├── InteractiveBOM_CH390H Module.html
├── IMGs/                       Final module and size-comparison photos
├── site/                       GitHub Pages landing page
├── CH390H_Arduino/             Standalone Arduino-ESP32 sketch
├── (external) ESP32-CH390H-lib Arduino library + examples at github.com/UGEelectronics/ESP32-CH390H-lib
└── CH390H_ESP32/s3-ch390h/     ESP-IDF example + CH390 driver
```
