#pragma once

#include "main.h"

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
