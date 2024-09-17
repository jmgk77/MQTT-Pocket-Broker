#include <Arduino.h>
#include <espnow.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SERVER "MQTT_SERVER_DEBUG"

#define DUMP_ESPNOW_PACKET

uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#define ESP2MQTT_SIGNATURE 'm'

enum { ESPNOW_PING, ESPNOW_PONG, ESPNOW_MQTT };

// MQTT|type|device_name|topic|payload
typedef struct {
  char signature = ESP2MQTT_SIGNATURE;
  char type;
  char device_name[32];
  //
  char topic[100];
  char payload[100];
} ESPNOW_DATA;

ESPNOW_DATA espnow_data;

bool espnow_ok;

#define ESP_OK 0
#define LOWEST_CHANNEL 1
#define HIGHEST_CHANNEL 16

long last_update;

bool check_espnow() {
  bool found = false;
  if (esp_now_init() == ESP_OK) {
    Serial.printf("ESPNOW channel %d...\n", WiFi.channel());
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(
    [](uint8_t *mac, uint8_t *incomingData, uint8_t len) {
      // receive data
#ifdef DUMP_ESPNOW_PACKET
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

      ESPNOW_DATA* packet = (ESPNOW_DATA*)incomingData;
      if (packet->signature == ESP2MQTT_SIGNATURE) {
        //        Serial.println("*sign ok");
        if (strcmp(SERVER, packet->device_name) == 0) {
          //          Serial.println("*name ok");
          // PING
          if (packet->type == ESPNOW_PONG) {
            //            Serial.println("*is pong");
            espnow_ok = true;
          }
        }
      }
    });

    esp_now_add_peer(broadcast_mac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

    // ping?
    espnow_ok = false;
    esp_now_send(0, (u8 *)&espnow_data, sizeof(espnow_data));

    // wait fro reply...
    delay(5000);
    esp_now_unregister_recv_cb();

    if (!espnow_ok) {
      Serial.println("NO REPLY...");
      esp_now_del_peer(broadcast_mac);
      esp_now_deinit();
    } else {
      Serial.println("FOUND!");
      found = true;
    }
  }
  return found;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP TEST");

  //build ping packet
  espnow_data = {};
  espnow_data.signature = ESP2MQTT_SIGNATURE;
  espnow_data.type = ESPNOW_PING;
  strcpy(espnow_data.device_name, SERVER);
  strcpy(espnow_data.topic, "TESTE");
  strcpy(espnow_data.payload, "TESTE");

  bool found = false;

  // scan all channels
  for (uint8_t channel = LOWEST_CHANNEL; channel <= HIGHEST_CHANNEL;
       channel++) {
    // set channel
    WiFi.mode(WIFI_STA);
    WiFi.begin("ESPNOW_SCAN", "", channel, NULL, false);
    WiFi.disconnect();

    found = check_espnow();
    if (found) {
      break;
    }
  }
  Serial.printf("ESPNOW %s\n", found ? "OK" : "NOK");
  while (!found) {}

  espnow_data.type = ESPNOW_MQTT;
  strcpy(espnow_data.topic, "TESTE_ESPNOW/RND");
  last_update = millis();
}

void loop() {
  char buf[64];
  if ((millis() - last_update) > (30 * 1000)) {
    last_update = millis();
    Serial.printf("Sending %d\n", last_update);
    strcpy(espnow_data.payload, itoa(last_update, buf, 10));
    esp_now_send(0, (u8 *)&espnow_data, sizeof(espnow_data));
  }
}
