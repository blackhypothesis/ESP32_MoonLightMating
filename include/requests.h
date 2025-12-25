#pragma once

#include <ESPAsyncWebServer.h>
#include "parameters.h"
#include "motor_control.h"
#include "network_wifi.h"

void requestRootURL(AsyncWebServerRequest *request);
void requestSaveHiveWifiConfig(AsyncWebServerRequest *request);
void requestGetVersion(AsyncWebServerRequest *request);
void requestGetDateTime(AsyncWebServerRequest *request);
void requestGetHiveConfig(AsyncWebServerRequest *request);
void requestGetWifiConfig(AsyncWebServerRequest *request);
void requestResetDefaultConfig(AsyncWebServerRequest *request);
void requestReboot(AsyncWebServerRequest *request);
void requestGetConfigStatus(AsyncWebServerRequest *request);
void requestGetConfigStatusClient(AsyncWebServerRequest *request);
void requestSetDateTime(AsyncWebServerRequest *request);
void requestSetScheduleConfig(AsyncWebServerRequest *request);
void requestGetClientStates(AsyncWebServerRequest *request);
void requestScanWifi(AsyncWebServerRequest *request);
void requestSecondsSinceBoot(AsyncWebServerRequest *request);
void requestMotorControl(AsyncWebServerRequest *request);
