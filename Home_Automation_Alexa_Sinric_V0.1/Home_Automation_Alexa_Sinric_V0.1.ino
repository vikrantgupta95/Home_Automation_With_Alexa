/*
  Author: Vikrant_Gupta
  Email: vikrantgupta95@gmail.com
  LinkedIn: https://www.linkedin.com/in/vikrantgupta95/
  Instagram: https://www.instagram.com/crazyvikku/
  GitHub: https://github.com/vikrantgupta95
  Created For: Apeiro Energy 

  Description:
  *How to use up to N SinricPro Switch devices on one ESP module 
 *                       to control N relays and N flipSwitches for manually control:
 * - setup N SinricPro switch devices
 * - setup N relays
 * - setup N flipSwitches to control relays manually
 *   (flipSwitch can be a tactile button or a toggle switch and is setup in line #52)
 * 
 * - handle request using just one callback to switch relay
 * - handle flipSwitches to switch relays manually and send event to sinricPro server
 * 
 * - SinricPro deviceId and PIN configuration for relays and buttins is done in std::map<String, deviceConfig_t> devices
 * 
 * If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one

  Last updated: [11-04-2024]
*/

#ifdef ENABLE_DEBUG
#define DEBUG_ESP_PORT Serial
#define NODEBUG_WEBSOCKETS
#define NDEBUG
#endif

#include <Arduino.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
#include <WiFi.h>
#endif

#include "SinricPro.h"
#include "SinricProSwitch.h"
#include <map>

#define WIFI_SSID "Inlyf"
#define WIFI_PASS "Inlyf@2022"
#define APP_KEY "ed0d23fc-9227-49f9-9455-aa29133a0592"
#define APP_SECRET "0d96f066-26d6-409c-8dd9-f365ea6babed-5e038c7b-b94e-489d-9917-076598d238cf"

// comment the following line if you use a toggle switches instead of tactile buttons

#define BAUD_RATE 115200

#define DEBOUNCE_TIME 250

#define RELAYPIN_1 18  //Lights
#define RELAYPIN_2 19  //Lamp
#define RELAYPIN_3 25  //Fan
//#define RELAYPIN_4 26  // Socket

#define SWITCH_ID_1 "65f3423038f6f4a3cdc212b1"
#define SWITCH_ID_2 "65f3424638f6f4a3cdc21303"
#define SWITCH_ID_3 "65f3421538f6f4a3cdc21294"
//#define SWITCH_ID_4       "65f3421538f6f4a3cdc21294"



bool onPowerState1(const String &deviceId, bool &state) {
  Serial.printf("Device 1 turned %s", state ? "on" : "off");
  digitalWrite(RELAYPIN_1, state ? HIGH : LOW);
  Serial.println(" ");
  Serial.println("******************************");
  Serial.print("Relay PIN: ");
  Serial.print(RELAYPIN_1);
  Serial.print(" = is set to ");
  Serial.println(state);
  Serial.println("******************************");
  return true;  // request handled properly
}

bool onPowerState2(const String &deviceId, bool &state) {
  Serial.printf("Device 2 turned %s", state ? "on" : "off");
  digitalWrite(RELAYPIN_2, state ? HIGH : LOW);
  Serial.println(" ");
  Serial.println("******************************");
  Serial.print("Relay PIN: ");
  Serial.print(RELAYPIN_2);
  Serial.print(" = is set to ");
  Serial.println(state);
  Serial.println("******************************");
  return true;  // request handled properly
}

bool onPowerState3(const String &deviceId, bool &state) {
  Serial.printf("Device 3 turned %s", state ? "on" : "off");
  digitalWrite(RELAYPIN_3, state ? HIGH : LOW);
  Serial.println(" ");
  Serial.println("******************************");
  Serial.print("Relay PIN: ");
  Serial.print(RELAYPIN_3);
  Serial.print(" = is set to ");
  Serial.println(state);
  Serial.println("******************************");
  return true;  // request handled properly
}


void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");

#if defined(ESP8266)
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(true);
#elif defined(ESP32)
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
    digitalWrite(2, LOW);
  }

  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
  digitalWrite(2, HIGH);
}

// setup function for SinricPro
void setupSinricPro() {
  // add devices and callbacks to SinricPro
  pinMode(RELAYPIN_1, OUTPUT);
  pinMode(RELAYPIN_2, OUTPUT);
  pinMode(RELAYPIN_3, OUTPUT);

  SinricProSwitch &mySwitch1 = SinricPro[SWITCH_ID_1];
  mySwitch1.onPowerState(onPowerState1);
  SinricProSwitch &mySwitch2 = SinricPro[SWITCH_ID_2];
  mySwitch2.onPowerState(onPowerState2);
  SinricProSwitch &mySwitch3 = SinricPro[SWITCH_ID_3];
  mySwitch3.onPowerState(onPowerState3);


  // setup SinricPro
  SinricPro.onConnected([]() {
    Serial.printf("Connected to SinricPro\r\n");
  });
  SinricPro.onDisconnected([]() {
    Serial.printf("Disconnected from SinricPro\r\n");
  });
  SinricPro.restoreDeviceStates(true);  // Uncomment to restore the last known state from the server.

  SinricPro.begin(APP_KEY, APP_SECRET);
}

// main setup function
void setup() {
  Serial.begin(BAUD_RATE);
  Serial.printf("\r\n\r\n");
  setupWiFi();
  setupSinricPro();
  pinMode(2, OUTPUT);
}

void loop() {
  SinricPro.handle();
}
