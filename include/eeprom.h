#pragma once

#define EEPROM_SIGNATURE 'J'
#define MAX_USER 4
#define MAX_MEDLEVS 4

struct eeprom_data {
  unsigned char sign = EEPROM_SIGNATURE;
  unsigned int crc32;
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
  char mqtt_remote_remove_prefix[64];
  char mqtt_remote_add_prefix[64];
} eeprom;

const char EEPROM_INFO[] PROGMEM =
    "EEPROM\n"
    "\tsign: %c\n"
    "\tcrc32: %08x\n"
    "\tdevice_name: %s\n"
    "\tmqtt_server_ip: %s\n"
    "\tmqtt_server_port: %d\n"
    "\tmqtt_remote_enable: %s\n"
    "\tmqtt_remote_ip: %s\n"
    "\tmqtt_remote_port: %d\n"
    "\tmqtt_remote_username: %s\n"
    "\tmqtt_remote_password: %s\n"
    "\tmqtt_remote_send: %s\n"
    "\tmqtt_remote_receive: %s\n"
    "\tmqtt_remote_remove_prefix: %s\n"
    "\tmqtt_remote_add_prefix: %s\n";

void dump_eeprom() {
  Serial.printf_P(EEPROM_INFO, eeprom.sign, eeprom.crc32, eeprom.device_name,
                  eeprom.mqtt_server_ip, eeprom.mqtt_server_port,
                  eeprom.mqtt_remote_enable ? "YES" : "NO",
                  eeprom.mqtt_remote_ip, eeprom.mqtt_remote_port,
                  eeprom.mqtt_remote_username, eeprom.mqtt_remote_password,
                  eeprom.mqtt_remote_send ? "YES" : "NO",
                  eeprom.mqtt_remote_receive ? "YES" : "NO",
                  eeprom.mqtt_remote_remove_prefix, eeprom.mqtt_remote_add_prefix);
}

String dump_eeprom_string() {
  char buffer[1024];
  sprintf(buffer, EEPROM_INFO, eeprom.sign, eeprom.crc32, eeprom.device_name,
          eeprom.mqtt_server_ip, eeprom.mqtt_server_port,
          eeprom.mqtt_remote_enable ? "YES" : "NO", eeprom.mqtt_remote_ip,
          eeprom.mqtt_remote_port, eeprom.mqtt_remote_username,
          eeprom.mqtt_remote_password, eeprom.mqtt_remote_send ? "YES" : "NO",
          eeprom.mqtt_remote_receive ? "YES" : "NO",
          eeprom.mqtt_remote_remove_prefix, eeprom.mqtt_remote_add_prefix);
  String s = buffer;
  s.replace("\n", "<br>");
  return "<FONT color=blue>" + s + "</FONT><br><br>";
}

unsigned int calculate_eeprom_checkum() {
  //
  unsigned char buffer[sizeof(eeprom_data)];
  CRC32 crc;

  // save old eeprom crc32
  unsigned int temp_crc32 = eeprom.crc32;
  eeprom.crc32 = 0;

  // copy eeprom data
  memcpy(buffer, &eeprom, sizeof(eeprom_data));

  for (unsigned int i = 0; i < sizeof(eeprom_data); i++) {
    crc.update(buffer[i]);
  }

  // restore old eeprom crc32
  eeprom.crc32 = temp_crc32;
  return crc.finalize();
}

void save_eeprom() {
  eeprom.crc32 = calculate_eeprom_checkum();
  EEPROM.put(0, eeprom);
  EEPROM.commit();
}

void default_eeprom() {
  eeprom = {};
  eeprom.sign = EEPROM_SIGNATURE;
  strcpy(eeprom.device_name, DEFAULT_DEVICE_NAME);
  eeprom.mqtt_server_port = 1883;
  eeprom.mqtt_remote_port = 1883;
  eeprom.crc32 = calculate_eeprom_checkum();
}

bool verify_eeprom() {
  unsigned int crc32 = calculate_eeprom_checkum();
  if ((eeprom.sign != EEPROM_SIGNATURE) || (eeprom.crc32 != crc32)) {
    Serial.printf("EEPROM error (want %08x has %08x)\n", eeprom.crc32, crc32);
    return false;
  } else {
    Serial.println("EEPROM ok");
    return true;
  }
}
