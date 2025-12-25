#pragma once

#include "Arduino.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
// #include <ElegantOTA.h>
#include <AccelStepper.h>
#include <TimeLib.h>
#include "webserver.h"
#include "parameters.h"
#include "helper_functions.h"
#include "requests.h"
#include "motor_control.h"


void set_last_action_to_now();
void update_clients(char *, char *);
void initFS();

String getConfigStatus();
String getClientStates();

void queenHiveUpdate(void *);
void sendWifiConfigToClients(void *);

void initApp(void *pvParameters);

void setup();
void loop();
