#ifndef MAIN_H
#define MAIN_H

#include "Arduino.h"
#include "SPIFFS.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoHttpClient.h>
// #include <ElegantOTA.h>
#include <AccelStepper.h>
#include <TimeLib.h>
#include "parameters.h"
#include "helper_functions.h"

// Queues
static QueueHandle_t motor_cmd_queue[MAX_MOTOR];
static QueueHandle_t log_queue;

void set_last_action_to_now();
void update_clients(char *, char *);
void initFS();

String readFile(fs::FS &, const char *);
void writeFile(fs::FS &, const char *, const char *);
void resetDefaultConfigs();
void readHiveConfigFile();
void writeHiveConfigFile();
void readWifiConfigFile();
void writeWifiConfigFile();

int secondsTillMotorStart(String);

String getConfigStatus();
String getHiveConfig();
String getWifiConfig();

String getClientStates();

void scanWiFi();
bool initWiFi();
bool initAP();

void notifyClients(String);
void handleWebSocketMessage(void *, uint8_t *, size_t);
void onEvent(AsyncWebSocket *, AsyncWebSocketClient *, AwsEventType, void *, uint8_t *, size_t);
void initWebSocket();
void webSocketNotifyClients(void *);

void scheduleMotorCommands(void *);
void controlStepperMotor(void *);
void queenHiveUpdate(void *);
void sendWifiConfigToClients(void *);

void initApp(void *pvParameters);

void setup();
void loop();

#endif
