#include "helper_functions.h"

String getVersion(int hive_type) {
  String v;
  JsonDocument version;

  if (hive_type == HIVE_DRONES) {
    v = VERSION + "-D";
  } else {
    v = VERSION + "-Q";
  }
  version["version"] = v;
  char serialized_version[32];
  serializeJson(version, serialized_version);
  return String(serialized_version);
}

// use LED to signal actions
void actionBlink(int n, int ms) {
  pinMode(LED, OUTPUT);
  for (int i = 0; i < n; i++) {
    digitalWrite(LED, HIGH);
    vTaskDelay(ms / portTICK_PERIOD_MS);
    digitalWrite(LED,LOW);
    vTaskDelay(ms / portTICK_PERIOD_MS);
  }
}

// Time handling
// ---------------------------------------------------------
String int2str (int x) {
  String xStr = String(x);
  if (x < 10) {
    xStr = "0" + xStr;
  }
  return xStr;
}

// convert IPAddress to String
String ip_addr_to_str(const IPAddress& ipAddress) {
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

String getDateTime() {
  String timeStr = String(year()) + "-" + int2str(month()) + "-" + int2str(day())
              + " " + int2str(hour()) + ":" + int2str(minute()) + ":" + int2str(second());
  return timeStr;
}

