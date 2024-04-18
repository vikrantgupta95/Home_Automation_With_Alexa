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
#define RELAYPIN_2 25  //Lamp
#define RELAYPIN_3 19  //Fan
#define RELAYPIN_4 26  // Socket


typedef struct {  // struct for the std::map below
  int relayPIN;
  int flipSwitchPIN;  //Lights
  bool activeLow;
} deviceConfig_t;

// this is the main configuration
// please put in your deviceId, the PIN for Relay and PIN for flipSwitch
// this can be up to N devices...depending on how much pin's available on your device ;)
// right now we have 4 devicesIds going to 4 relays and 4 flip switches to switch the relay manually
std::map<String, deviceConfig_t> devices = {
  //{deviceId, {relayPIN,  flipSwitchPIN, activeLow}}
  { "65f3423038f6f4a3cdc212b1", { 18, 18, true } },  //Lights
  { "65f3424638f6f4a3cdc21303", { 25, 25, true } },  //Lamp
  { "65f3421538f6f4a3cdc21294", { 19, 19, true } },  // Fan

};

typedef struct {  // struct for the std::map below
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
  bool activeLow;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;  // this map is used to map flipSwitch PINs to deviceId and handling debounce and last flipSwitch state checks
                                                 // it will be setup in "setupFlipSwitches" function, using informations from devices map

void setupRelays() {
  for (auto &device : devices) {            // for each device (relay, flipSwitch combination)
    int relayPIN = device.second.relayPIN;  // get the relay pin
    pinMode(relayPIN, OUTPUT);              // set relay pin to OUTPUT
  }
}

void setupFlipSwitches() {
  for (auto &device : devices) {          // for each device (relay / flipSwitch combination)
    flipSwitchConfig_t flipSwitchConfig;  // create a new flipSwitch configuration

    flipSwitchConfig.deviceId = device.first;         // set the deviceId
    flipSwitchConfig.lastFlipSwitchChange = 0;        // set debounce time
    flipSwitchConfig.lastFlipSwitchState = false;     // set lastFlipSwitchState to false (LOW)
    int flipSwitchPIN = device.second.flipSwitchPIN;  // get the flipSwitchPIN
    bool activeLow = device.second.activeLow;         // set the activeLow
    flipSwitchConfig.activeLow = activeLow;
    flipSwitches[flipSwitchPIN] = flipSwitchConfig;  // save the flipSwitch config to flipSwitches map

    if (activeLow) {
      pinMode(flipSwitchPIN, INPUT_PULLUP);  // set the flipSwitch pin to INPUT_PULLUP
    } else {
      pinMode(flipSwitchPIN, INPUT);  // set the flipSwitch pin to INPUT
    }
  }
}

bool onPowerState(String deviceId, bool &state) {
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN;  // get the relay pin for corresponding device
  digitalWrite(relayPIN, state);              // set the new relay state

  Serial.println("******************************");
  Serial.print("Relay PIN: ");
  Serial.print(relayPIN);
  Serial.print(" = is set to ");
  Serial.println(state);
  Serial.println("******************************");


  return true;
}

void handleFlipSwitches() {
  unsigned long actualMillis = millis();                                          // get actual millis
  for (auto &flipSwitch : flipSwitches) {                                         // for each flipSwitch in flipSwitches map
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  // get the timestamp when flipSwitch was pressed last time (used to debounce / limit events)

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {  // if time is > debounce time...

      int flipSwitchPIN = flipSwitch.first;                              // get the flipSwitch pin from configuration
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;  // get the lastFlipSwitchState
      bool activeLow = flipSwitch.second.activeLow;
      bool flipSwitchState = digitalRead(flipSwitchPIN);  // read the current flipSwitch state
      if (activeLow) flipSwitchState = !flipSwitchState;

      if (flipSwitchState != lastFlipSwitchState) {  // if the flipSwitchState has changed...
#ifdef TACTILE_BUTTON
        if (flipSwitchState) {  // if the tactile button is pressed
#endif
          flipSwitch.second.lastFlipSwitchChange = actualMillis;  // update lastFlipSwitchChange time
          String deviceId = flipSwitch.second.deviceId;           // get the deviceId from config
          int relayPIN = devices[deviceId].relayPIN;              // get the relayPIN from config
          bool newRelayState = !digitalRead(relayPIN);            // set the new relay State
          digitalWrite(relayPIN, newRelayState);                  // set the trelay to the new state

          SinricProSwitch &mySwitch = SinricPro[deviceId];  // get Switch device from SinricPro
          mySwitch.sendPowerStateEvent(newRelayState);      // send the event
#ifdef TACTILE_BUTTON
        }
#endif
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;  // update lastFlipSwitchState
      }
    }
  }
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
  }

  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro() {
  for (auto &device : devices) {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }


  SinricPro.begin(APP_KEY, APP_SECRET);
}

void setup() {
  Serial.begin(BAUD_RATE);
  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}

void loop() {
  SinricPro.handle();
  handleFlipSwitches();
}
