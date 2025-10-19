  #include "requests.h"
  #include "SPIFFS.h"
  #include "parameters.h"
  #include "main.h"
  
  // Web Server Root URL
  void requestRootURL(AsyncWebServerRequest *request) {
    request->send(SPIFFS, root_html, "text/html");
    set_last_action_to_now();
  }

  void requestSaveHiveWifiConfig(AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      Serial.printf("%s 1. hive and wifi config parameters from POST request.\n", getDateTime().c_str());
      const AsyncWebParameter* p = request->getParam(i);
      Serial.printf("%s 2. hive and wifi config parameters from POST request.\n", getDateTime().c_str());
      if (xSemaphoreTake(wifi_config_mutex, 200) == pdTRUE) {
        Serial.printf("%s 3. got semaphore\n", getDateTime().c_str());
        if(p->isPost()){
          if (p->name() == "hivetype") {
            hive_config.hive_type = p->value().toInt();
          }
          if (p->name() == "wifimode") {
            hive_config.wifi_mode = p->value().toInt();
          }
          if (p->name() == "ssid") {
            wifi_config.ssid = p->value().c_str();
          }
          if (p->name() == "pass") {
            wifi_config.pass = p->value().c_str();
          }
          if (p->name() == "ip") {
            wifi_config.ip = p->value().c_str();
          }
          if (p->name() == "gateway") {
            wifi_config.gateway = p->value().c_str();
          }
          if (p->name() == "dns") {
            wifi_config.dns = p->value().c_str();
          }
        }
        xSemaphoreGive(wifi_config_mutex);
        Serial.printf("%s get hive and wifi config from POST request.\n", getDateTime().c_str());
      } else {
        Serial.printf("%s cannot get hive and wifi config from POST request: mutex locked.\n", getDateTime().c_str());
      }
      Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
    writeWifiConfigFile();
    writeHiveConfigFile();

    // set wifi_config_sent to 0 in order that the current wifi_config will be sent to all clients again
    if (hive_config.hive_type == HIVE_DRONES) {
      for (int i = 0; i < MAX_CLIENTS; i++) {
      state_client[i].wifi_config_sent = 0;
      }
    }
    request->send(200, "text/plain", "Done. ESP will be restarted to connect with the WiFi settings. New IP address: " + wifi_config.ip);
    ESP.restart();
  }