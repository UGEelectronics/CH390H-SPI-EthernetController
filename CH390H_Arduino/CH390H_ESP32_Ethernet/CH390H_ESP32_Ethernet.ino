/*
 * CH390H SPI Ethernet — Arduino-ESP32 example
 *
 * CH390H is a SPI MAC+PHY. It is not a W5500, so do not use
 * ETH.begin(ETH_PHY_W5500, ...) or the classic Arduino Ethernet.h library.
 *
 * Required:
 *   - Arduino-ESP32 board package (esp32 by Espressif)
 *   - ESP32-CH390 library:
 *     https://github.com/meshtastic/ESP32-CH390
 *
 * Module is 3.3 V only. Common GND. INT is optional (set to -1 to poll).
 */

#include "ESP32_CH390.h"
#include "WiFi.h"
#include <WebServer.h>

// Set to 1 for a fixed address, 0 for DHCP
#define USE_STATIC_IP 0

#if CONFIG_IDF_TARGET_ESP32S3
// Matches the ESP-IDF example in this repository (ESP32-S3)
static const int PIN_CS   = 10;
static const int PIN_SCK  = 12;
static const int PIN_MOSI = 11;
static const int PIN_MISO = 13;
static const int PIN_INT  = 14;
static const int SPI_HOST = 1;  // SPI2 / HSPI
#elif CONFIG_IDF_TARGET_ESP32
static const int PIN_CS   = 5;
static const int PIN_SCK  = 18;
static const int PIN_MOSI = 23;
static const int PIN_MISO = 19;
static const int PIN_INT  = 4;
static const int SPI_HOST = 2;  // VSPI
#else
// ESP32-C3 / other: change these to match your wiring
static const int PIN_CS   = 7;
static const int PIN_SCK  = 4;
static const int PIN_MOSI = 6;
static const int PIN_MISO = 5;
static const int PIN_INT  = 10;
static const int SPI_HOST = 1;
#endif

WebServer server(80);

void onNetEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH started");
      CH390.setHostname("ch390h");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("ETH link up");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("ETH IP: ");
      Serial.println(CH390.localIP());
      Serial.print("Gateway: ");
      Serial.println(CH390.gatewayIP());
      Serial.print("Speed: ");
      Serial.print(CH390.linkSpeed());
      Serial.println(" Mbps");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH link down");
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH stopped");
      break;
    default:
      break;
  }
}

void handleRoot() {
  String body = "CH390H Ethernet OK\nIP: " + CH390.localIP().toString() + "\n";
  server.send(200, "text/plain", body);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("CH390H Arduino-ESP32 example");

  WiFi.onEvent(onNetEvent);

  ch390_config_t cfg = CH390_DEFAULT_CONFIG();
  cfg.spi_cs_gpio   = PIN_CS;
  cfg.spi_sck_gpio  = PIN_SCK;
  cfg.spi_mosi_gpio = PIN_MOSI;
  cfg.spi_miso_gpio = PIN_MISO;
  cfg.spi_clock_mhz = 20;     // keep <= 33 MHz with this library
  cfg.spi_host      = SPI_HOST;
  cfg.int_gpio      = PIN_INT;  // use -1 to poll instead of IRQ

  if (!CH390.begin(cfg)) {
    Serial.println("CH390 begin failed — check 3.3 V, GND, and SPI wiring");
    return;
  }

#if USE_STATIC_IP
  CH390.config(
      IPAddress(192, 168, 1, 10),
      IPAddress(192, 168, 1, 1),
      IPAddress(255, 255, 255, 0),
      IPAddress(8, 8, 8, 8));
#endif

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server on port 80");
}

void loop() {
  server.handleClient();
}
