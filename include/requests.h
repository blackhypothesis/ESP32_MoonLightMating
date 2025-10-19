#ifndef REQUESTS_H
#define REQUESTS_H

#include <ESPAsyncWebServer.h>

void requestRootURL(AsyncWebServerRequest *request);
void requestSaveHiveWifiConfig(AsyncWebServerRequest *request);

#endif