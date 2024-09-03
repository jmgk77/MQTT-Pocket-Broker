/*
███╗   ███╗ ██████╗ ████████╗████████╗
████╗ ████║██╔═══██╗╚══██╔══╝╚══██╔══╝
██╔████╔██║██║   ██║   ██║      ██║
██║╚██╔╝██║██║▄▄ ██║   ██║      ██║
██║ ╚═╝ ██║╚██████╔╝   ██║      ██║
╚═╝     ╚═╝ ╚══▀▀═╝    ╚═╝      ╚═╝


Copyright JMGK 2022/2023
*/

#define DEFAULT_DEVICE_NAME "MQTT_SERVER"

#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif

#include "main.h"

#include <Arduino.h>

WiFiManager wm;
ESP8266WebServer server;
ESP8266HTTPUpdateServer httpUpdater;

#ifdef DEBUG_RAM
Ticker debug_ram;
#endif

char boot_time[32];

PicoMQTT::Server* mqtt;

/*
db   d8b   db d88888b d8888b.
88   I8I   88 88'     88  `8D
88   I8I   88 88ooooo 88oooY'
Y8   I8I   88 88~~~~~ 88~~~b.
`8b d8'8b d8' 88.     88   8D
 `8b8' `8d8'  Y88888P Y8888P'
*/

void handle_404() { server.send(200, F("text/txt"), F("Not found")); }

void handle_root() {
  String s;
  //
  s += "<form action='/config' method='POST'><input type='submit' "
       "value='CONFIG'></form>";
  s += "<form action='/reboot' method='POST'><input type='submit' "
       "value='REBOOT'></form>";
  s += "<form action='/reset' method='POST'><input type='submit' "
       "value='RESET'></form><br>";
  // info
  s += "IP: <i>" + WiFi.localIP().toString() + "</i><br>";
  s += "Data de ínicio: <i>" + String(boot_time) + "</i><br>";
  // version
  s += "Version: " + String(VERSION) + "<br><br>";
  // update
  s += "<form action='/update' method='POST' "
       "enctype='multipart/form-data'><label for='firmware'>Atualizar "
       "firmware:</label><input type='file' accept='.bin,.bin.gz' "
       "name='firmware'><input type='submit' value='ATUALIZAR'></form>";
  // send config page
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send_P(200, "text/html", html_header);
  server.sendContent_P(s.c_str());
  server.sendContent_P(html_footer);
}

#define FORM_SAVE_STRING(VAR) \
  strncpy(eeprom.VAR, server.arg(#VAR).c_str(), sizeof(eeprom.VAR));
#define FORM_SAVE_INT(VAR) eeprom.VAR = server.arg(#VAR).toInt();
#define FORM_SAVE_BOOL(VAR) \
  eeprom.VAR = server.arg(#VAR) == "on" ? true : false;

#define FORM_START(URL) \
  s += "<form action='" + String(URL) + "' method='POST'>";
#define FORM_ASK_VALUE(VAR, TXT)                                           \
  s += "<label for='" + String(#VAR) + "'>" + String(TXT) +                \
       ":</label><input type='text' name='" + String(#VAR) + "' value='" + \
       eeprom.VAR + "'><br>";
#define FORM_ASK_BOOL(VAR, TXT)                                         \
  s += "<label for='" + String(#VAR) + "'>" + String(TXT) +             \
       ":</label><input type='checkbox' name='" + String(#VAR) + "' " + \
       String(eeprom.VAR ? "checked" : "") + "><br>";
#define FORM_END(BTN)                                                          \
  s +=                                                                         \
      "<input type='hidden' name='s' value='1'><input type='submit' value='" + \
      String(BTN) + "'></form>";

void handle_config() {
  if (server.hasArg("s")) {
    // read options
    FORM_SAVE_STRING(device_name)
    FORM_SAVE_STRING(fixed_ip)
    FORM_SAVE_INT(mqtt_server_port)
    // save data to eeprom
    dump_eeprom();
    save_eeprom();
  } else {
    String s;
    FORM_START("/config")
    FORM_ASK_VALUE(device_name, "Device name:")
    FORM_ASK_VALUE(fixed_ip, "Fixed IP:")
    FORM_ASK_VALUE(mqtt_server_port, "MQTT Broker Port:")
    FORM_END("SALVAR")
    //
    // s += "<form action='/config' method='POST'><input type='submit' "
    //      "value='CONFIG'></form>";
    s += "<form action='/reboot' method='POST'><input type='submit' "
         "value='REBOOT'></form>";
    s += "<form action='/reset' method='POST'><input type='submit' "
         "value='RESET'></form><br>";
    // update
    s += "<form action='/update' method='POST' "
         "enctype='multipart/form-data'><label for='firmware'>Atualizar "
         "firmware:</label><input type='file' accept='.bin,.bin.gz' "
         "name='firmware'><input type='submit' value='ATUALIZAR'></form>";
    // info
    s += "IP: <i>" + WiFi.localIP().toString() + "</i><br>";
    s += "Data de ínicio: <i>" + String(boot_time) + "</i><br>";
    // version
    s += "Version: " + String(VERSION) + "<br><br>";
    // send config page
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send_P(200, "text/html", html_header);
    server.sendContent_P(s.c_str());
    server.sendContent_P(html_footer);
  }
}

void handle_reboot() {
  server.send(200, F("text/html"),
              F("<meta http-equiv='refresh' content='15; url=/' />"));
  delay(1 * 1000);
  ESP.restart();
  delay(2 * 1000);
}

void handle_reset() {
  // erase eeprom
  eeprom = {};
  save_eeprom();
  // reset wifi
  wm.resetSettings();
  handle_reboot();
}

/*
.d8888. d88888b d888888b db    db d8888b.
88'  YP 88'     `~~88~~' 88    88 88  `8D
`8bo.   88ooooo    88    88    88 88oodD'
  `Y8b. 88~~~~~    88    88    88 88~~~
db   8D 88.        88    88b  d88 88
`8888Y' Y88888P    YP    ~Y8888P' 88
*/

void setup() {
  // init eeprom
  EEPROM.begin(sizeof(eeprom_data));

  // if there's valid eeprom config, load it
  EEPROM.get(0, eeprom);
  if (eeprom.sign != EEPROM_SIGNATURE) {
    // default eeprom
    eeprom = {};
    eeprom.sign = EEPROM_SIGNATURE;
    strcpy(eeprom.device_name, DEFAULT_DEVICE_NAME);
    eeprom.mqtt_server_port = 1883;
  }

  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n--------------------------------------------------"));
  Serial.print("MQTT_SERVER ");
  Serial.println(VERSION);
  dump_esp8266();
  dump_eeprom();

  // connect to internet
  WiFi.mode(WIFI_STA);
  delay(10);
  wm.setDebugOutput(false);
  WiFi.hostname(eeprom.device_name);
  wm.setConfigPortalTimeout(180);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // set fixed ip
  if (strlen(eeprom.fixed_ip)) {
    IPAddress ip, gateway;

    ip.fromString(eeprom.fixed_ip);

    String s = eeprom.fixed_ip;
    String g = s.substring(0, s.lastIndexOf('.')) + ".1";
    gateway.fromString(g.c_str());

    Serial.print("Set IP: ");
    Serial.println(ip);
    wm.setSTAStaticIPConfig(ip, gateway, IPAddress(255, 255, 255, 0),
                            IPAddress(8, 8, 8, 8));
  }

  if (!wm.autoConnect(eeprom.device_name)) {
    ESP.restart();
    delay(1 * 1000);
  }
  Serial.println("Got IP: " + WiFi.localIP().toString());

  // mqtt
  mqtt = new PicoMQTT::Server((uint16_t)eeprom.mqtt_server_port);
  mqtt->subscribe("#", [](const char* topic, const char* payload) {
    Serial.printf("Received message in topic '%s': %s\n", topic, payload);
  });
  mqtt->begin();

  // install www handlers
  httpUpdater.setup(&server, "/update");
  server.onNotFound(handle_404);
  server.on("/", handle_root);
  server.on("/config", handle_config);
  server.on("/reboot", handle_reboot);
  server.on("/reset", handle_reset);

  server.begin();

  // discovery protocols
  MDNS.begin(eeprom.device_name);
  MDNS.addService("http", "tcp", 80);

  // get internet time (GMT-3)
  configTime("<-03>3", "pool.ntp.org");
  // repeat till get year past 2021...
  while (time(nullptr) < 1609459200) {
    delay(100);
  }

  // get boot time
  time_t t = time(NULL);
  strncpy(boot_time, ctime(&t), sizeof(boot_time));
  Serial.print(boot_time);

#ifdef DEBUG_RAM
  debug_ram.attach(10, []() {
    Serial.printf("MEM (%d)[%d]\n", ESP.getFreeHeap(),
                  ESP.getMaxFreeBlockSize());
  });
#endif

  Serial.println(F("--------------------SETUP DONE--------------------"));
}

/*
db       .d88b.   .d88b.  d8888b.
88      .8P  Y8. .8P  Y8. 88  `8D
88      88    88 88    88 88oodD'
88      88    88 88    88 88~~~
88booo. `8b  d8' `8b  d8' 88
Y88888P  `Y88P'   `Y88P'  88
*/

void loop() {
  // handle www
  server.handleClient();

  // handle discovery protocols
  MDNS.update();

  // handle mqtt
  mqtt->loop();
}
