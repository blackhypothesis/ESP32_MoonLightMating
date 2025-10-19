#ifndef REQUESTS_H
#define REQUESTS_H

#include <ESPAsyncWebServer.h>
// #include <ArduinoJson.h>
#include "parameters.h"

void requestRootURL(AsyncWebServerRequest *request);
void requestSaveHiveWifiConfig(AsyncWebServerRequest *request);
void requestGetVersion(AsyncWebServerRequest *request);
void requestGetDateTime(AsyncWebServerRequest *request);
void requestGetHiveConfig(AsyncWebServerRequest *request);
void requestGetWifiConfig(AsyncWebServerRequest *request);


#endif