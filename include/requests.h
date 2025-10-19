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
void requestResetDefaultConfig(AsyncWebServerRequest *request);
void requestGetConfigStatus(AsyncWebServerRequest *request);
void requestGetConfigStatusClient(AsyncWebServerRequest *request);
void requestSetDateTime(AsyncWebServerRequest *request);
void requestSetScheduleConfig(AsyncWebServerRequest *request);
void requestGetClientStates(AsyncWebServerRequest *request);
void requestScanWifi(AsyncWebServerRequest *request);

#endif