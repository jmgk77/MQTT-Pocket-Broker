/*
███╗   ███╗ ██████╗ ████████╗████████╗
████╗ ████║██╔═══██╗╚══██╔══╝╚══██╔══╝
██╔████╔██║██║   ██║   ██║      ██║
██║╚██╔╝██║██║▄▄ ██║   ██║      ██║
██║ ╚═╝ ██║╚██████╔╝   ██║      ██║
╚═╝     ╚═╝ ╚══▀▀═╝    ╚═╝      ╚═╝


Copyright JMGK 2024
*/

#define DEBUG
// #define DUMP_ESPNOW_PACKET

#ifdef DEBUG
#define DEFAULT_DEVICE_NAME "MQTT_SERVER_DEBUG"
#else
#define DEFAULT_DEVICE_NAME "MQTT_SERVER"
#endif

#include "main.h"

DNSServer dns;
AsyncWebServer server(80);
AsyncWiFiManager wm(&server, &dns);
ESPAsyncHTTPUpdateServer updateServer;
PicoMQTT::Server* mqtt_broker;
PicoMQTT::Client* mqtt_client;

char boot_time[32];

uint32_t startup_heap;

/*
███████╗███████╗██████╗ ███╗   ██╗ ██████╗ ██╗    ██╗
██╔════╝██╔════╝██╔══██╗████╗  ██║██╔═══██╗██║    ██║
█████╗  ███████╗██████╔╝██╔██╗ ██║██║   ██║██║ █╗ ██║
██╔══╝  ╚════██║██╔═══╝ ██║╚██╗██║██║   ██║██║███╗██║
███████╗███████║██║     ██║ ╚████║╚██████╔╝╚███╔███╔╝
╚══════╝╚══════╝╚═╝     ╚═╝  ╚═══╝ ╚═════╝  ╚══╝╚══╝
*/

void init_espnow() {
  Serial.print("ESPNOW ");

  // init espnow
  if ((esp_now_init() != 0) ||
      (esp_now_set_self_role(ESP_NOW_ROLE_COMBO) != 0)) {
    Serial.println("NOK");
    return;
  }

  // callback
  esp_now_register_recv_cb(
      [](uint8_t* mac, uint8_t* incomingData, uint8_t len) {
#ifdef DUMP_ESPNOW_PACKET
        // dump incomming packet
        Serial.printf("ESPNOW recv:\t[%02x:%02x:%02x:%02x:%02x:%02x]", mac[0],
                      mac[1], mac[2], mac[3], mac[4], mac[5]);
        for (int i = 0; i < len; i++) {
          if (i % 8 == 0) {
            Serial.print("\n\t");
          }
          Serial.printf("%02x ", incomingData[i]);
        }
        Serial.println();
        for (int i = 0; i < len; i++) {
          if (i % 8 == 0) {
            Serial.print("\n\t");
          }
          Serial.printf(" %c ", (incomingData[i] < 32) ? '.' : incomingData[i]);
        }
        Serial.println();
#endif
        //
        ESPNOW_DATA* packet = (ESPNOW_DATA*)incomingData;
        if (packet->signature == ESP2MQTT_SIGNATURE) {
          if (strcmp(eeprom.device_name, packet->device_name) == 0) {
            // PING
            if (packet->type == ESPNOW_PING) {
              // send a PONG
              packet->type = ESPNOW_PONG;
              esp_now_send(0, incomingData, sizeof(ESPNOW_DATA));
              Serial.println("Received ESPNOW PING, replied PONG");
            }
            // MQTT
            if (packet->type == ESPNOW_MQTT) {
              // publish to mqtt
              Serial.printf("Received ESPNOW message in topic '%s': %s\n",
                            packet->topic, packet->payload);
              mqtt_broker->publish((const char*)packet->topic,
                                   (const void*)packet->payload,
                                   strlen(packet->payload));
              Serial.printf("Sending ESPNOW->LOCAL message in topic '%s': %s\n",
                            packet->topic, packet->payload);
            }
          }
        }
      });

  esp_now_add_peer(broadcast_mac, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
  Serial.println("OK");
}

/*
██╗    ██╗███████╗██████╗
██║    ██║██╔════╝██╔══██╗
██║ █╗ ██║█████╗  ██████╔╝
██║███╗██║██╔══╝  ██╔══██╗
╚███╔███╔╝███████╗██████╔╝
 ╚══╝╚══╝ ╚══════╝╚═════╝
*/

void handle_404(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

void handle_root(AsyncWebServerRequest* request) {
  uint32_t heap = ESP.getFreeHeap();
  String s;
  //
  s += "Memória livre: <i>" + String(heap) + "/" + String(startup_heap) +
       " bytes</i> (" +
       String((float)((float)heap / (float)startup_heap) * (float)100, 2) +
       "\%, frag: <i>" + String(ESP.getHeapFragmentation()) + "%)</i><br>";
  // info
  s += "IP: <i>" + WiFi.localIP().toString() + "</i><br>";
  s += "Data de ínicio: <i>" + String(boot_time) + "</i><br>";
  // version
  s += "Versão: " + String(VERSION) +
#ifdef DEBUG
       "<FONT color=red><b> DEBUG</b></FONT>" +
#endif
       "<br><br>";
#ifdef DEBUG
  // dump eeprom
  s += dump_eeprom_string();
#endif
  // buttons
  s += "<form action='/config' method='POST'><input type='submit' "
       "value='CONFIG'></form>";
  s += "<form action='/reboot' method='POST'><input type='submit' "
       "value='REBOOT'></form>";
  s += "<form action='/reset' method='POST'><input type='submit' "
       "value='RESET'></form>";
  // send root page
  request->send(200, "text/html", html_header + s + html_footer);
}

void handle_config(AsyncWebServerRequest* request) {
  //
  if (request->hasParam("s", true)) {
    // read options
    FORM_SAVE_STRING(device_name)
    FORM_SAVE_STRING(mqtt_server_ip)
    FORM_SAVE_INT(mqtt_server_port)
    FORM_SAVE_BOOL(espnow2mqtt)
    FORM_SAVE_BOOL(mqtt_remote_enable)
    FORM_SAVE_STRING(mqtt_remote_ip)
    FORM_SAVE_INT(mqtt_remote_port)
    FORM_SAVE_STRING(mqtt_remote_username)
    FORM_SAVE_STRING(mqtt_remote_password)
    FORM_SAVE_BOOL(mqtt_remote_send)
    FORM_SAVE_BOOL(mqtt_remote_receive)
    FORM_SAVE_STRING(mqtt_remote_remove_prefix)
    FORM_SAVE_STRING(mqtt_remote_add_prefix)
    // save data to eeprom
    save_eeprom();
    dump_eeprom();
    request->send(200, "text/html",
                  "<meta http-equiv='refresh' content='0; url=/config'/>");
  } else {
    String s;
    FORM_START("/config")
    FORM_ASK_VALUE(device_name, "Device name")
    FORM_ASK_VALUE(mqtt_server_ip, "MQTT Broker fixed IP")
    FORM_ASK_VALUE(mqtt_server_port, "MQTT Broker Port")
    FORM_ASK_BOOL(espnow2mqtt, "Enable ESPNOW to MQTT bridge")
    FORM_ASK_BOOL_JS(mqtt_remote_enable, "Enable remote MQTT",
                     js_mqtt_remote_enable)
    FORM_ASK_VALUE(mqtt_remote_ip, "MQTT remote IP")
    FORM_ASK_VALUE(mqtt_remote_port, "MQTT remote Port")
    FORM_ASK_VALUE(mqtt_remote_username, "MQTT remote username")
    FORM_ASK_VALUE(mqtt_remote_password, "MQTT remote password")
    FORM_ASK_BOOL(mqtt_remote_send, "Send to remote MQTT")
    FORM_ASK_BOOL(mqtt_remote_receive, "Receive from remote MQTT")
    FORM_ASK_VALUE(mqtt_remote_remove_prefix, "Remove PREFIX from remote MQTT")
    FORM_ASK_VALUE(mqtt_remote_add_prefix, "Add PREFIX to remote MQTT")
    FORM_END("SALVAR")
    // update
    s +=
        "<form action='/update?name=firmware'enctype=multipart/form-data "
        "method=POST>Firmware:<br><input type=file accept=.bin,.bin.gz "
        "name=firmware> <input type=submit value='Update Firmware'></form><br>";
    // buttons
    s += "<form action='/' method='POST'><input type='submit' "
         "value='MAIN'></form>";
    s += "<form action='/reboot' method='POST'><input type='submit' "
         "value='REBOOT'></form>";
    s += "<form action='/reset' method='POST'><input type='submit' "
         "value='RESET'></form>";
    // add javascript for config page
    s += js_config;
    // send config page
    request->send(200, "text/html", html_header + s + html_footer);
  }
}

void handle_reboot(AsyncWebServerRequest* request) {
  request->send(200, "text/html",
                "<meta http-equiv='refresh' content='15; url=/' />");
  delay(1 * 1000);
  ESP.restart();
  delay(2 * 1000);
}

void handle_reset(AsyncWebServerRequest* request) {
  // erase eeprom
  default_eeprom();
  save_eeprom();
  // reset wifi
  wm.resetSettings();
  handle_reboot(request);
}

/*
███████╗███████╗████████╗██╗   ██╗██████╗
██╔════╝██╔════╝╚══██╔══╝██║   ██║██╔══██╗
███████╗█████╗     ██║   ██║   ██║██████╔╝
╚════██║██╔══╝     ██║   ██║   ██║██╔═══╝
███████║███████╗   ██║   ╚██████╔╝██║
╚══════╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝
*/

void setup() {
  // save initial free heap
  startup_heap = ESP.getFreeHeap();

  //
  Serial.begin(115200);
  delay(1000);

  // init eeprom
  EEPROM.begin(sizeof(eeprom_data));

  // if there's valid eeprom config, load it
  EEPROM.get(0, eeprom);
  if (!verify_eeprom()) {
    default_eeprom();
  }

  Serial.println(F("\n--------------------------------------------------"));
  Serial.print("MQTT_SERVER ");
  Serial.println(VERSION_SHORT);
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
  if (strlen(eeprom.mqtt_server_ip)) {
    IPAddress ip, gateway;

    ip.fromString(eeprom.mqtt_server_ip);

    String s = eeprom.mqtt_server_ip;
    String g = s.substring(0, s.lastIndexOf('.')) + ".1";
    gateway.fromString(g.c_str());

    WiFi.config(ip, gateway, IPAddress(255, 255, 255, 0),
                IPAddress(8, 8, 8, 8));

    Serial.print("Set IP: ");
    Serial.println(ip);
  }

  // captive portal
  if (!wm.autoConnect(eeprom.device_name)) {
    ESP.restart();
    delay(1 * 1000);
  }
  WiFi.mode(WIFI_STA);

  Serial.println("Got IP: " + WiFi.localIP().toString());

  // mqtt broker
  mqtt_broker = new PicoMQTT::Server((uint16_t)eeprom.mqtt_server_port);

  // mqtt client
  if (eeprom.mqtt_remote_enable) {
    mqtt_client = new PicoMQTT::Client(
        eeprom.mqtt_remote_ip, eeprom.mqtt_remote_port, eeprom.device_name,
        eeprom.mqtt_remote_username, eeprom.mqtt_remote_password);

    if (eeprom.mqtt_remote_receive) {
      // mqtt remote listener
      mqtt_client->subscribe("#", [](const char* topic, const void* payload,
                                     size_t payload_size) {
        Serial.printf("Received REMOTE message in topic '%s': %s\n", topic,
                      (char*)payload);
        // remove topic prefix
        int l = strlen(eeprom.mqtt_remote_remove_prefix);
        if (l) {
          if (strncmp(eeprom.mqtt_remote_remove_prefix, topic, l) == 0) {
            Serial.printf("Removing %s of %s, become %s\n",
                          eeprom.mqtt_remote_remove_prefix, topic, topic + l);
            topic += l;
          }
        }
        // send to local broker
        mqtt_broker->publish(topic, payload, payload_size);
        Serial.printf("Sending REMOTE->LOCAL message in topic '%s': %s\n",
                      topic, (char*)payload);
      });
    }
  }

  // mqtt broker listener
  mqtt_broker->subscribe(
      "#", [](const char* topic, const void* payload, size_t payload_size) {
        Serial.printf("Received LOCAL message in topic '%s': %s\n", topic,
                      (char*)payload);
        if (eeprom.mqtt_remote_enable && eeprom.mqtt_remote_send) {
          if (strlen(eeprom.mqtt_remote_add_prefix)) {
            // add prefix to topic
            char buf[512];
            strcpy(buf, eeprom.mqtt_remote_add_prefix);
            strcat(buf, topic);
            Serial.printf("Adding %s in %s, become %s\n",
                          eeprom.mqtt_remote_add_prefix, topic, buf);
            topic = (const char*)&buf;
          }
          // send to remote mqtt
          mqtt_client->publish(topic, payload, payload_size);
          Serial.printf("Sending LOCAL->REMOTE message in topic '%s': %s\n",
                        topic, (char*)payload);
        };
      });
  mqtt_broker->begin();

  // install www handlers
  server.onNotFound(handle_404);
  server.on("/", HTTP_ANY, handle_root);
  server.on("/config", HTTP_ANY, handle_config);
  server.on("/reboot", HTTP_ANY, handle_reboot);
  server.on("/reset", HTTP_ANY, handle_reset);
  updateServer.setup(&server, "/update");
  server.begin();

  // discovery protocols
  MDNS.begin(eeprom.device_name);
  MDNS.addService("http", "tcp", 80);

  // get internet time (GMT-3)
  configTime("<-03>3", "pool.ntp.org");
  // repeat till get year past 2021...
  int retries = 600;
  while ((time(nullptr) < 1609459200) && (retries--)) {
    Serial.print(".");
    delay(100);
  }

  // get boot time
  time_t t = time(NULL);
  strncpy(boot_time, ctime(&t), sizeof(boot_time));
  Serial.print(boot_time);

  if (eeprom.espnow2mqtt) {
    init_espnow();
  }

  Serial.println(F("--------------------SETUP DONE--------------------"));
}

/*
██╗      ██████╗  ██████╗ ██████╗
██║     ██╔═══██╗██╔═══██╗██╔══██╗
██║     ██║   ██║██║   ██║██████╔╝
██║     ██║   ██║██║   ██║██╔═══╝
███████╗╚██████╔╝╚██████╔╝██║
╚══════╝ ╚═════╝  ╚═════╝ ╚═╝
*/

void loop() {
  // handle discovery protocols
  MDNS.update();

  // handle mqtt broker
  mqtt_broker->loop();

  // handle mqtt client
  if ((mqtt_client) && (eeprom.mqtt_remote_enable)) {
    mqtt_client->loop();
  }
}
