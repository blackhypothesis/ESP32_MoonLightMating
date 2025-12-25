#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <ESPAsyncWebServer.h>
#include "requests.h"

AsyncWebServer* newWebServer(AsyncWebSocket *);
AsyncWebSocket* newWebSocket();
void notifyClients(AsyncWebSocket*, String);
void handleWebSocketMessage(void *, uint8_t *, size_t);
void onEvent(AsyncWebSocket *, AsyncWebSocketClient *, AwsEventType, void *, uint8_t *, size_t);


#endif