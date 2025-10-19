#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "Arduino.h"
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "parameters.h"

String getVersion(int hive_type);

void actionBlink(int, int);
String int2str(int);
String ip_addr_to_str(const IPAddress &);
String getDateTime();

#endif