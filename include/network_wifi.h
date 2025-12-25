#pragma once

#include <WiFi.h>
#include "parameters.h"
#include "helper_functions.h"

void scanWiFi();
bool initWiFi();
bool initAP();