#pragma once

#define EEPROM_SIGNATURE 'J'
#define MAX_USER 4
#define MAX_MEDLEVS 4

struct eeprom_data {
  unsigned char sign = EEPROM_SIGNATURE;
  unsigned int checksum;
  //
  char device_name[32];
  char mqtt_server_ip[64];
  unsigned int mqtt_server_port;
  bool mqtt_remote_enable;
  char mqtt_remote_ip[64];
  unsigned int mqtt_remote_port;
  char mqtt_remote_username[32];
  char mqtt_remote_password[32];
  bool mqtt_remote_send;
  bool mqtt_remote_receive;
} eeprom;

const char EEPROM_INFO[] PROGMEM =
    "EEPROM\n"
    "\tsign: %c\n"
    "\tchecksum: %08x\n"
    "\tdevice_name: %s\n"
    "\tmqtt_server_ip: %s\n"
    "\tmqtt_server_port: %d\n"
    "\tmqtt_remote_enable: %s\n"
    "\tmqtt_remote_ip: %s\n"
    "\tmqtt_remote_port: %d\n"
    "\tmqtt_remote_username: %s\n"
    "\tmqtt_remote_password: %s\n"
    "\tmqtt_remote_send: %s\n"
    "\tmqtt_remote_receive: %s\n";

void dump_eeprom() {
  Serial.printf_P(EEPROM_INFO, eeprom.sign, eeprom.checksum, eeprom.device_name,
                  eeprom.mqtt_server_ip, eeprom.mqtt_server_port,
                  eeprom.mqtt_remote_enable ? "YES" : "NO",
                  eeprom.mqtt_remote_ip, eeprom.mqtt_remote_port,
                  eeprom.mqtt_remote_username, eeprom.mqtt_remote_password,
                  eeprom.mqtt_remote_send ? "YES" : "NO",
                  eeprom.mqtt_remote_receive ? "YES" : "NO");
}

String dump_eeprom_string() {
  char buffer[512];
  sprintf(buffer, EEPROM_INFO, eeprom.sign, eeprom.checksum, eeprom.device_name,
          eeprom.mqtt_server_ip, eeprom.mqtt_server_port,
          eeprom.mqtt_remote_enable ? "YES" : "NO", eeprom.mqtt_remote_ip,
          eeprom.mqtt_remote_port, eeprom.mqtt_remote_username,
          eeprom.mqtt_remote_password, eeprom.mqtt_remote_send ? "YES" : "NO",
          eeprom.mqtt_remote_receive ? "YES" : "NO");
  String s = buffer;
  s.replace("\n", "<br>");
  return "<FONT color=blue>" + s + "</FONT<br><br>";
}

unsigned int calculate_eeprom_checkum() {
  // save old checksum
  unsigned int temp_checksum = eeprom.checksum;
  eeprom.checksum = 0;

  // copy eeprom data
  unsigned char buffer[sizeof(eeprom_data)];
  memcpy(buffer, &eeprom, sizeof(eeprom_data));

  unsigned int checksum = 0;
  for (unsigned int i = 0; i < sizeof(eeprom_data); i++) {
    checksum += buffer[i];
  }
  checksum = ~checksum;
  checksum ^= 'JMGK';

  // restore old checksum
  eeprom.checksum = temp_checksum;
  return checksum;
}

void save_eeprom() {
  eeprom.checksum = calculate_eeprom_checkum();
  EEPROM.put(0, eeprom);
  EEPROM.commit();
}

void default_eeprom() {
  eeprom = {};
  eeprom.sign = EEPROM_SIGNATURE;
  strcpy(eeprom.device_name, DEFAULT_DEVICE_NAME);
  eeprom.mqtt_server_port = 1883;
  eeprom.mqtt_remote_port = 1883;
  eeprom.checksum = calculate_eeprom_checkum();
}

bool verify_eeprom() {
  unsigned int checksum = calculate_eeprom_checkum();
  if ((eeprom.sign != EEPROM_SIGNATURE) || (eeprom.checksum != checksum)) {
    Serial.printf("EEPROM error (want %08x has %08x)\n", eeprom.checksum,
                  checksum);
    return false;
  } else {
    Serial.println("EEPROM ok");
    return true;
  }
}
