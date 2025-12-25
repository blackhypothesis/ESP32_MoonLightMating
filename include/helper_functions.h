#pragma once

#include "Arduino.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include <TimeLib.h>
#include "parameters.h"

String getVersion(int hive_type);

void actionBlink(int, int);
String int2str(int);
String ip_addr_to_str(const IPAddress &);
String getDateTime();

void scanWiFi();
bool initWiFi();
bool initAP();

String readFile(fs::FS &, const char *);
void writeFile(fs::FS &, const char *, const char *);
void resetDefaultConfigs();
void readHiveConfigFile();
void writeHiveConfigFile();
void readWifiConfigFile();
void writeWifiConfigFile();
String getHiveConfig();
String getWifiConfig();

void IRAM_ATTR handleButtonPress();
void interruptFunction();
