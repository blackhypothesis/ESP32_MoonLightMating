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
    const AsyncWebParameter* p = request->getParam(i);
    if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {
      if(p->isPost()){
        if (p->name() == "hive-type") {
          hive_config.hive_type = p->value().toInt();
        }
        if (p->name() == "wifi-mode") {
          hive_config.wifi_mode = p->value().toInt();
        }
        if (p->name() == "offset-open-door") {
          hive_config.offset_open_door = p->value().toInt();
        }
        if (p->name() == "offset-close-door") {
          hive_config.offset_close_door = p->value().toInt();
        }
        if (p->name() == "photoresistor-edge-delta") {
          hive_config.photoresistor_edge_delta = p->value().toInt();
        }
        if (p->name() == "photoresistor-read-interval-ms") {
          hive_config.photoresistor_read_interval_ms = p->value().toInt();
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
      xSemaphoreGive(config_mutex);
      Serial.printf("%s get hive/wifi config from POST request.\n", getDateTime().c_str());
    } else {
      Serial.printf("%s cannot get hive/wifi config from POST request: mutex locked.\n", getDateTime().c_str());
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
  request->send(200, "text/plain", "Done. Reboot ESP, if WiFi settings have changed. IP address: " + wifi_config.ip);
}

void requestGetVersion(AsyncWebServerRequest *request) {
  String version = getVersion(hive_config.hive_type);
  Serial.printf("%s getversion %s\n", getDateTime().c_str(), version.c_str());
  request-> send(200, "application/json", version);
  set_last_action_to_now();
}

void requestGetDateTime(AsyncWebServerRequest *request) {
  JsonDocument dt;
  dt["datetime"] = getDateTime();
  char serialized_dt[64];
  serializeJson(dt, serialized_dt);
  Serial.printf("%s getdatetime %s\n", getDateTime().c_str(), serialized_dt);
  request-> send(200, "application/json", String(serialized_dt));
  set_last_action_to_now();
}

void requestGetHiveConfig(AsyncWebServerRequest *request) {
  if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {
    JsonDocument hc;
    hc["hive_type"] = hive_config.hive_type;
    hc["wifi_mode"] = hive_config.wifi_mode;
    hc["offset_open_door"] = hive_config.offset_open_door;
    hc["offset_close_door"] = hive_config.offset_close_door;
    hc["photoresistor_edge_delta"] = hive_config.photoresistor_edge_delta;
    hc["photoresistor_read_interval_ms"] = hive_config.photoresistor_read_interval_ms;

    char serialized_hc[256];
    serializeJson(hc, serialized_hc);
    request-> send(200, "application/json", String(serialized_hc));
    set_last_action_to_now();
    xSemaphoreGive(config_mutex);
    Serial.printf("%s /gethiveconfig %s\n", getDateTime().c_str(), serialized_hc);
  } else {
    Serial.printf("%s /gethiveconfig, cant't get config: mutex locked.\n", getDateTime().c_str());
  }
}

void requestGetWifiConfig(AsyncWebServerRequest *request) {
  if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {
    JsonDocument wc;
    wc["ssid"] = wifi_config.ssid;
    wc["pass"] = wifi_config.pass;
    wc["ip"] = wifi_config.ip;
    wc["gateway"] = wifi_config.gateway;
    wc["dns"] = wifi_config.dns;
    char serialized_wc[128];
    serializeJson(wc, serialized_wc);
    request-> send(200, "application/json", String(serialized_wc));
    set_last_action_to_now();
    xSemaphoreGive(config_mutex);
    Serial.printf("%s /getwificonfig %s\n", getDateTime().c_str(), serialized_wc);
  } else {
    Serial.printf("%s /getwificonfig, cant't get config: mutex locked.\n", getDateTime().c_str());
  }
}

void requestResetDefaultConfig(AsyncWebServerRequest *request) {
  Serial.printf("%s /resetdefaultconfig %s\n", getDateTime().c_str());
  resetDefaultConfigs();
  ESP.restart();
}

void requestReboot(AsyncWebServerRequest *request) {
  Serial.printf("%s /reboot %s\n", getDateTime().c_str());
  ESP.restart();
}

void requestGetConfigStatus(AsyncWebServerRequest *request) {
  IPAddress client_ip = request->client()->remoteIP();
  char client_ip_c[16];
  strcpy(client_ip_c, ip_addr_to_str(client_ip).c_str());
  String config_status = getConfigStatus();
  Serial.printf("%s getconfigstatus %s %s\n", getDateTime().c_str(), client_ip_c, config_status.c_str());
  request->send(200, "application/json", config_status);
  set_last_action_to_now();
}

void requestGetConfigStatusClient(AsyncWebServerRequest *request) {
  String client_mac = "00:00:00:00:00:00";
  if (request->hasParam("mac")) {
    client_mac = request->getParam("mac")->value();
  }
  IPAddress client_ip = request->client()->remoteIP();
  char client_ip_c[16];
  strcpy(client_ip_c, ip_addr_to_str(client_ip).c_str());
  char client_mac_c[20];
  strcpy(client_mac_c, client_mac.c_str());
  String config_status = getConfigStatus();
  Serial.printf("%s %s %s getconfigstatusclient %s\n", getDateTime().c_str(), client_ip_c, client_mac_c, config_status.c_str());
   // add client ip to array if it is a new one
  update_clients(client_ip_c, client_mac_c);
  request->send(200, "application/json", config_status);
  set_last_action_to_now();
}

void requestSetDateTime(AsyncWebServerRequest *request) {
  String epochseconds;
  if (request->hasParam("epochseconds")) {
    epochseconds = request->getParam("epochseconds")->value();
    JsonDocument dt;
    dt["datetime"] = getDateTime();    
    char serialized_dt[64];
    serializeJson(dt, serialized_dt);
    Serial.printf("%s setdatetime epochseconds: %ld\n", getDateTime().c_str(), epochseconds.toInt());
    // update datetime and last action atomic, to avoid race time condition with while loop in initApp (this) task.
    if (xSemaphoreTake(setdatetime_mutex, 100) == pdTRUE) {
      setTime(epochseconds.toInt());
      request->send(200, "application/json", serialized_dt);
      set_last_action_to_now();
      xSemaphoreGive(setdatetime_mutex);
    }
  }
}

void requestSetScheduleConfig(AsyncWebServerRequest *request) {
  if (request->hasParam("hour_open") && request->hasParam("minute_open") && request->hasParam("hour_close") && request->hasParam("minute_close") && request->hasParam("queens_delay") && request->hasParam("config_enable")) {
    if(xSemaphoreTake(schedule_motor_mutex, 300 / portTICK_PERIOD_MS) == pdTRUE) {
      sched_motor.hour_door_open = request->getParam("hour_open")->value().toInt();
      sched_motor.minute_door_open = request->getParam("minute_open")->value().toInt();
      sched_motor.hour_door_close = request->getParam("hour_close")->value().toInt();
      sched_motor.minute_door_close = request->getParam("minute_close")->value().toInt();
      sched_motor.queens_delay = request->getParam("queens_delay")->value().toInt();
      sched_motor.config_enable = request->getParam("config_enable")->value().toInt();        
      xSemaphoreGive(schedule_motor_mutex);
    }
    JsonDocument hiveconfig;
    hiveconfig["hour_open"] = sched_motor.hour_door_open;
    hiveconfig["minute_open"] = sched_motor.minute_door_open;
    hiveconfig["hour_close"] = sched_motor.hour_door_close;
    hiveconfig["minute_close"] = sched_motor.minute_door_close;
    hiveconfig["queens_delay"] = sched_motor.queens_delay;
    hiveconfig["config_enable"] = sched_motor.config_enable;
    char serialized_hiveconfig[256];
    serializeJson(hiveconfig, serialized_hiveconfig);
    Serial.printf("%s requestSetScheduleConfig: %s\n", getDateTime().c_str(), serialized_hiveconfig);
    request->send(200, "application/json", String(serialized_hiveconfig));
    set_last_action_to_now();
  } else {
    Serial.printf("%s, requestSetScheduleConfig: not all parameters in request\n", getDateTime().c_str());
  }
}
 
void requestGetClientStates(AsyncWebServerRequest *request) {
  String client_states = getClientStates();
  Serial.printf("%s, getclientstates %s\n", getDateTime().c_str(), client_states.c_str());
  request->send(200, "application/json", client_states);
  set_last_action_to_now();
}

void requestScanWifi(AsyncWebServerRequest *request) {
  scanWiFi();
}

void requestSecondsSinceBoot(AsyncWebServerRequest *request) {
  int64_t uptimeSeconds = esp_timer_get_time() / 1000000;
  JsonDocument ssb;
  ssb["seconds_since_boot"] = (int)uptimeSeconds;
  char serialized_ssb[64];
  serializeJson(ssb, serialized_ssb);
  Serial.printf("%s, secondssinceboot: %s\n", getDateTime().c_str(), serialized_ssb);
  request->send(200, "application/json", String(serialized_ssb));
  set_last_action_to_now();
}

void requestMotorControl(AsyncWebServerRequest *request) {
  Serial.println("request MotorControl");
  if (request->hasParam("mcmd") && request->hasParam("steps")) {
    int mcmd = request->getParam("mcmd")->value().toInt();
    int steps = request->getParam("steps")->value().toInt();
    JsonDocument mctl;
    mctl["mcmd"] = mcmd;
    mctl["steps"] = steps;
    char serialized_mctl[128];
    serializeJson(mctl, serialized_mctl);
    Serial.printf("%s requestMotorControl: %s\n", getDateTime().c_str(), serialized_mctl);
    request->send(200, "application/json", String(serialized_mctl));
    queueMotorControl((MotorCommand)mcmd, steps);
    set_last_action_to_now();
  }
}
 