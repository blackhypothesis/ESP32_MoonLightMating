#pragma once

#include "Arduino.h"
#include <ESPAsyncWebServer.h>
#include "requests.h"

AsyncWebServer* newWebServer();
AsyncWebSocket* newWebSocket();

void notifyClients(AsyncWebSocket *, String);
void handleWebSocketMessage(void *, uint8_t *, size_t);
void onEvent(AsyncWebSocket *, AsyncWebSocketClient *, AwsEventType, void *, uint8_t *, size_t);
