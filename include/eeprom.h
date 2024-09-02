#pragma once

#define EEPROM_SIGNATURE 'J'
#define MAX_USER 4
#define MAX_MEDLEVS 4

struct eeprom_data {
  unsigned char sign = EEPROM_SIGNATURE;
  //
  char device_name[32];
  //
  char fixed_ip[32];
  //
  unsigned int mqtt_server_port;
  //
} eeprom;

const char EEPROM_INFO[] PROGMEM =
    "EEPROM\n"
    // "\tsign: %c\n"
    "\tdevice_name: %s\n"
    "\tfixed_ip: %s\n"
    "\tmqtt_server_port: %d\n";

void dump_eeprom() {
  Serial.printf_P(EEPROM_INFO, /*eeprom.sign,*/ eeprom.device_name,
                  eeprom.fixed_ip, eeprom.mqtt_server_port);
}

void save_eeprom() {
  EEPROM.put(0, eeprom);
  EEPROM.commit();
}
