#pragma once

#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif

#include <Arduino.h>
#include <CRC32.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncHTTPUpdateServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <ESP_EEPROM.h>
#include <PicoMQTT.h>
#include <Ticker.h>

#include "eeprom.h"
#include "html.h"
#include "version.h"

const char ESP_INFO[] PROGMEM =
    "ESP8266\n"
    "\tESP.getFreeHeap(): %d\n"
    "\tESP.getChipId(): %08X\n"
    "\tESP.getSdkVersion(): %s\n"
    "\tESP.getBootVersion(): %d\n"
    "\tESP.getBootMode(): %d\n"
    "\tESP.getCpuFreqMHz(): %d\n"
    "\tESP.getFlashChipId(): %08X\n"
    "\tESP.getFlashChipRealSize(): %d\n"
    "\tESP.getFlashChipSize(): %d\n"
    "\tESP.getFlashChipSpeed(): %dMHz\n"
    "\tESP.getFlashChipSizeByChipId(): %d\n"
    "\tESP.getSketchSize(): %d\n"
    "\tESP.getFreeSketchSpace(): %d\n"
    "\tESP.getResetInfo(): %s\n";

void dump_esp8266() {
  Serial.printf_P(ESP_INFO, ESP.getFreeHeap(), ESP.getChipId(),
                  ESP.getSdkVersion(), ESP.getBootVersion(), ESP.getBootMode(),
                  ESP.getCpuFreqMHz(), ESP.getFlashChipId(),
                  ESP.getFlashChipRealSize(), ESP.getFlashChipSize(),
                  ESP.getFlashChipSpeed() / 1000000,
                  ESP.getFlashChipSizeByChipId(), ESP.getSketchSize(),
                  ESP.getFreeSketchSpace(), ESP.getResetInfo().c_str());
}
