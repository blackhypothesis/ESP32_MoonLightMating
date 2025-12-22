#include "main.h"

#define DEBUG(...) sprintf(message, __VA_ARGS__); debug(task_name, message);

// write debug messages
void debug(char *task_name, char* message) {
    char task_message[192];
    sprintf(task_message, "%s task=%s %s", getDateTime().c_str(), task_name, message);
    Serial.printf("%s\n", task_message);
}

// ---------------------------------------------------------
// Web Server
// ---------------------------------------------------------
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Task: app initialize
// ---------------------------------------------------------
void initApp(void *pvParameters) {
  time_t since_last_action_seconds;


  Serial.begin(115200);
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  // Interrupt
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleButtonPress, FALLING);

  // Filesystem: init
  // ---------------------------------------------------------
  initFS();

  // read Hive / WiFi configs
  // ---------------------------------------------------------
  readWifiConfigFile();
  readHiveConfigFile();

  Serial.printf("%s Version %s\n", getDateTime().c_str(), getVersion(hive_config.hive_type).c_str());

  // Configuration for webserver according to hive_type
  // ---------------------------------------------------------
  if (hive_config.hive_type == HIVE_DRONES) {
    strcpy(root_html, DRONES_HTML);
  }
  else {
    strcpy(root_html, QUEENS_HTML);
  }


  // WiFi: init
  // ---------------------------------------------------------
  Serial.printf("%s Init WiFi\n", getDateTime().c_str());
  
  Serial.printf("%s hive_config.hive_type = %d\n", getDateTime().c_str(), hive_config.hive_type);
  Serial.printf("%s hive_config.wifi_mode = %d\n", getDateTime().c_str(), hive_config.wifi_mode);
  if (hive_config.wifi_mode == MODE_WIFI_STA) {
    Serial.printf("%s WiFi Mode: STA.\n", getDateTime().c_str());
    if (initWiFi() == false) {
      Serial.printf("%s Unable to connect to SSID %s.\n", getDateTime().c_str(), wifi_config.ssid);
      if (resetButtonPressed == true) {
        Serial.printf("%s during Wifi setup, ...\n", getDateTime().c_str());
        interruptFunction();
      } else {
        ESP.restart();
      }
    }
  } else {
    Serial.printf("%s WiFi Mode: AP.\n", getDateTime().c_str());
    initAP();
  }

  // get MAC address after successful connected to WiFi
  mac_address = WiFi.macAddress();

  // initialize state_client array
	for (int i = 0; i < MAX_CLIENTS; i++) {
		strcpy(state_client[i].ip, "0.0.0.0");
    strcpy(state_client[i].mac, "00:00:00:00:00:00");
    state_client[i].active = 0;
    state_client[i].last_update_seconds = now();
	}
  // initialize motor command queues
  for (int i = 0; i < MAX_MOTOR; i++) {
    motor_cmd_queue[i] = xQueueCreate(5, sizeof(motor_control_t));
  }
  // initialize motor init parameters
  for (int i = 0; i < MAX_MOTOR; i++) {
    motor_init[i].offset_open_door = hive_config.offset_open_door;
    motor_init[i].offset_close_door = hive_config.offset_close_door;
    motor_init[i].photoresistor_edge_delta = hive_config.photoresistor_edge_delta;
    motor_init[i].photoresistor_read_interval_ms = hive_config.photoresistor_read_interval_ms;
  }
  // initialize log queue
  log_queue = xQueueCreate(5, 100);

  // initialize schedule motor commands task
  xTaskCreate(scheduleMotorCommands, "Schedule Motor Commands", 2048, NULL, 3, NULL);
  Serial.printf("%s Created Task: Schedule Motor Commands\n", getDateTime().c_str());

  // initialize motor tasks
  const char *task_name[] = {"Control Stepper Motor 0", "Control Stepper Motor 1"};
  for (int i = 0; i < MAX_MOTOR; i++) {
    xTaskCreate(controlStepperMotor, task_name[i], 4096, (void *) &motor_init[i], 4, NULL);
    Serial.printf("%s Created Task: %s\n", getDateTime().c_str(), task_name[i]);

  }

  // initialize drone hive task
  if (hive_config.hive_type == HIVE_DRONES) {
    xTaskCreate(sendWifiConfigToClients, "Send Wifi Config", 4096, NULL, 2, NULL);
    Serial.printf("%s Created Task: Send Wifi Config\n", getDateTime().c_str());
  }
  // initialize queen hive update task
  if (hive_config.hive_type == HIVE_QUEENS) {
    xTaskCreate(queenHiveUpdate, "Queen Hive Update", 4096, NULL, 2, NULL);
    Serial.printf("%s Created Task: Queen Hive Update\n", getDateTime().c_str());
  }

  // Webserver: initialize
  initWebSocket();
  // ---------------------------------------------------------
  // Web Server Root URL
  server.on("/", HTTP_GET, requestRootURL);
  server.on("/", HTTP_POST, requestSaveHiveWifiConfig);
  server.on("/getversion", HTTP_GET, requestGetVersion);
  server.on("/getdatetime", HTTP_GET, requestGetDateTime);
  server.on("/gethiveconfig", HTTP_GET, requestGetHiveConfig);
  server.on("/getwificonfig", requestGetWifiConfig);
  server.on("/resetdefaultconfig", HTTP_GET, requestResetDefaultConfig);
  server.on("/reboot", HTTP_GET, requestReboot);
  server.on("/getconfigstatus", HTTP_GET, requestGetConfigStatus);
  server.on("/getconfigstatusclient", HTTP_GET, requestGetConfigStatusClient);
  server.on("/setdatetime", HTTP_GET, requestSetDateTime);
  server.on("/setscheduleconfig", HTTP_GET, requestSetScheduleConfig);
  server.on("/getclientstates", HTTP_GET, requestGetClientStates);
  server.on("/scanwifi", HTTP_GET, requestScanWifi);
  server.on("/secondssinceboot", HTTP_GET, requestSecondsSinceBoot);

  server.serveStatic("/", SPIFFS, "/");

  // Start OTA capability to webserver
  // ElegantOTA.begin(&server);
  // Start server
  server.begin();

  actionBlink(3, 300);

  // Loop
  // ---------------------------------------------------------
  while(true) {
    // When the maximum number of clients is exceeded this function closes the oldest client.
    ws.cleanupClients();

     // during the date and time is set via webinterface, we should wait, till datetime and last_action are updated.
    if (xSemaphoreTake(setdatetime_mutex, 0) == pdTRUE) {
      since_last_action_seconds = now() - last_action_seconds;
      xSemaphoreGive(setdatetime_mutex);
    }
    else {
      Serial.printf("%s cannot update seconds_till_last_action: mutex locked.\n", getDateTime().c_str());
      since_last_action_seconds = 0;
    }

    if (since_last_action_seconds > SLEEP_AFTER_INACTIVITY_SECONDS) {
      int seconds_to_sleep = 0;
      Serial.printf("%s since_last_action_seconds = %ld; now() = %ld; last_action_seconds = %ld\n", getDateTime().c_str(), since_last_action_seconds, now(), last_action_seconds);

      if (xSemaphoreTake(seconds_till_door_move_mutex, 10) == pdTRUE) {
        seconds_to_sleep = seconds_till_door_open;
        if (seconds_till_door_close < seconds_till_door_open) {
          seconds_to_sleep = seconds_till_door_close;
        }
        xSemaphoreGive(seconds_till_door_move_mutex);
      }

      // Wake up 30 seconds before the motor has to move
      seconds_to_sleep = seconds_to_sleep - WAKEUP_BEFORE_MOTOR_MOVE_SECONDS;

      // Sleep only, if there is enough time till motor has to move
      if (seconds_to_sleep > 0) {
        Serial.printf("%s Going to light sleep [seconds]: %d\n", getDateTime().c_str(), seconds_to_sleep);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        WiFi.disconnect(true);
        WiFi.getSleep();
        uint64_t microseconds_to_sleep = (uint64_t) seconds_to_sleep * (uint64_t) 1000000;
        esp_sleep_enable_timer_wakeup(microseconds_to_sleep);
        esp_light_sleep_start();
        Serial.printf("%s Back from light sleep.\n", getDateTime().c_str());
        actionBlink(5, 300);

        Serial.printf("%s WiFi mode: WIFI_STA\n", getDateTime().c_str());
        if (initWiFi() == false) {
          // TODO: clear handling
          if (hive_config.hive_type == HIVE_DRONES) {
            initAP();
            strcpy(root_html, WIFI_CONFIG_HTML);
          }
        }
      }
      set_last_action_to_now();
    }

    if (resetButtonPressed == true) {
      Serial.printf("%s in while loop, ...\n", getDateTime().c_str());
      interruptFunction();
    }
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


// Update last action
void set_last_action_to_now() {
  if (xSemaphoreTake(last_action_mutex, 1000) == pdTRUE) {
    last_action_seconds = now();
    xSemaphoreGive(last_action_mutex);
    actionBlink(1, 150);
  }
  else {
    Serial.printf("%s cannot update last_action: mutex locked.\n", getDateTime().c_str());
  }
}

// add client IP to array if it is a new one
void update_clients(char* ip, char* mac) {
  if (xSemaphoreTake(state_client_mutex, 200) == pdTRUE) {  
    bool new_client = true;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (strcmp(state_client[i].ip, ip) == 0) {
        new_client = false;
        state_client[i].last_update_seconds = now();
        state_client[i].active++;
      }
    }
    if (new_client == true) {
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(state_client[i].ip, "0.0.0.0") == 0) {
          strcpy(state_client[i].ip, ip);
          strcpy(state_client[i].mac, mac);
          state_client[i].active = 1;
          state_client[i].wifi_config_sent = 0;
          state_client[i].last_update_seconds = now();
          break;
        }
      }
    }
    xSemaphoreGive(state_client_mutex);
    Serial.printf("%s update_client\n", getDateTime().c_str());
  } else {
    Serial.printf("%s update_client: mutex locked.\n", getDateTime().c_str());
  }    
}

// Initialize SPIFFS
void initFS() {
  if (!SPIFFS.begin(true)) {
    Serial.printf("%s An error has occurred while mounting SPIFFS.\n", getDateTime().c_str());
  }
  else{
    Serial.printf("%s SPIFFS mounted successfully.\n", getDateTime().c_str());
  }
}

// calculate amount of seconds, till motor should start
int secondsTillMotorStart(String openClose) {
  int h_action = 0, m_action = 0, s_action = 0;
  int h_now, m_now, s_now;
  int seconds_till_motor_start;

  h_now = hour();
  m_now = minute();
  s_now = second();

  if(xSemaphoreTake(schedule_motor_mutex, 300 / portTICK_PERIOD_MS) == pdTRUE) {
    if (openClose == "open") {
      h_action = sched_motor.hour_door_open;
      m_action = sched_motor.minute_door_open;
    }
    else {
      h_action = sched_motor.hour_door_close;
      m_action = sched_motor.minute_door_close;
    }
    xSemaphoreGive(schedule_motor_mutex);
  }

  if (hive_config.hive_type == HIVE_QUEENS) {
    int h_qd, m_qd, s_qd;
    int divisor_for_minutes, divisor_for_seconds;

    h_qd = floor(sched_motor.queens_delay / (60 * 60));
    divisor_for_minutes = sched_motor.queens_delay % (60 * 60);
    m_qd = floor(divisor_for_minutes / 60);
    divisor_for_seconds = divisor_for_minutes % 60;
    s_qd = ceil(divisor_for_seconds);

    s_action += s_qd;
    if (s_action > 59) {
      s_action -= 60;
      m_action++;
    }
    m_action += m_qd;
    if (m_action > 59) {
      m_action -= 60;
      h_action++;
    }
    h_action += h_qd;
    if (h_action > 23) {
      h_action -= 24;
    }
  }

  if (h_action < h_now || (h_action == h_now && m_action < m_now) || (h_action == h_now && m_action == m_now && s_action < s_now)) {
    h_action += 24;
  }

  seconds_till_motor_start = (h_action - h_now) * 3600 + (m_action - m_now) * 60 + (s_action - s_now);
  return seconds_till_motor_start;
}

String getConfigStatus() {
  JsonDocument config_status;
  config_status["drone_ip"] = wifi_config.ip;
  config_status["datetime"] = getDateTime();
  config_status["epochseconds"] = now();
  if (xSemaphoreTake(schedule_motor_mutex, 300 / portTICK_PERIOD_MS) == pdTRUE) {
    config_status["hour_door_open"] = sched_motor.hour_door_open;
    config_status["minute_door_open"] = sched_motor.minute_door_open;
    config_status["hour_door_close"] = sched_motor.hour_door_close;
    config_status["minute_door_close"] = sched_motor.minute_door_close;
    config_status["queens_delay"] = sched_motor.queens_delay;
    config_status["config_enable"] = sched_motor.config_enable;
    xSemaphoreGive(schedule_motor_mutex);
  }
  config_status["seconds_till_door_open"] = secondsTillMotorStart("open");
  config_status["seconds_till_door_close"] = secondsTillMotorStart("close");

  char serialized_config_status[384];
  serializeJson(config_status, serialized_config_status);

  return String(serialized_config_status);
}

String getClientStates() {
  if (xSemaphoreTake(state_client_mutex, 200) == pdTRUE) {  
    JsonDocument client_states;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (strcmp(state_client[i].ip, "0.0.0.0") == 0) {
        break;
      }
      client_states[i]["ip"] = state_client[i].ip;
      client_states[i]["mac"] = state_client[i].mac;
      client_states[i]["active"] = state_client[i].active;
      client_states[i]["wifi_config_sent"] = state_client[i].wifi_config_sent;
      client_states[i]["seconds"] = now() - state_client[i].last_update_seconds;
    }
    char serialized_client_states[512];
    serializeJson(client_states, serialized_client_states);
    
    xSemaphoreGive(state_client_mutex);
    Serial.printf("%s getClientStates.\n", getDateTime().c_str());
    return String(serialized_client_states);
  } else {
    Serial.printf("%s getClientStates: mutex locked.\n", getDateTime().c_str());
    return String("");
  }
}

void notifyClients(String state) {
  ws.textAll(state);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    JsonDocument control_motor;
    Serial.printf("String data: %s\n", (char *)data);
    DeserializationError error = deserializeJson(control_motor, (char *)data);
    if (error) {
      Serial.println("Error:deserialization failed.");
      Serial.println(error.f_str());
    }
    else {
      int steps = control_motor["steps"];
      int command = control_motor["command"];
      Serial.printf("%s handleWebSocketMessage: steps = %d, command = %d\n", getDateTime().c_str(), steps, command);
      queueMotorControl(steps, (MotorCommand)command);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("%s WebSocket client #%u connected from %s\n", getDateTime().c_str(), client->id(), client->remoteIP().toString().c_str());
      //Notify client of state
      notifyClients("state");
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("%s WebSocket client #%u disconnected\n", getDateTime().c_str(), client->id());
      break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
     break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// Task: webSocketNotifyClients
// ---------------------------------------------------------
void webSocketNotifyClients(void *pvParameters) {

}

// Task: get configuration from server
// this task will only be initialized if type of hife: 1 -> bees queens hive
// ---------------------------------------------------------
void queenHiveUpdate(void *pvParameters) {
  const String CONFIG_PATH_NAME = "/getconfigstatusclient?mac=" + mac_address;
  WiFiClient wifi;
  Serial.printf("%s Task queenHiveUpdate started\n", getDateTime().c_str());

  while(true) {
    HttpClient client = HttpClient(wifi, wifi_config.ip.c_str(), 80);
    int status = client.get(CONFIG_PATH_NAME.c_str());
    
    if (status == 0) {
      int statusCode = client.responseStatusCode();
      String response = client.responseBody();

      Serial.printf("%s queenHiveUpdate: status code: %d\n", getDateTime().c_str(), statusCode);
      Serial.printf("%s queenHiveUpdate: response: %s\n", getDateTime().c_str(), response.c_str());

      JsonDocument config;

      DeserializationError error = deserializeJson(config, response.c_str());
      if (error) {
        Serial.printf("%s queenHiveUpdate: error: deserialization failed: ", getDateTime().c_str());
        Serial.println(error.f_str());
      }

      JsonObject root = config.as<JsonObject>();
      if (root["epochseconds"].is<JsonVariant>()
          && root["hour_door_open"].is<JsonVariant>() && root["minute_door_open"].is<JsonVariant>()
          && root["hour_door_close"].is<JsonVariant>() && root["minute_door_close"].is<JsonVariant>()
          && root["queens_delay"].is<JsonVariant>() && root["config_enable"].is<JsonVariant>()) {
        if (xSemaphoreTake(schedule_motor_mutex, 10) == pdTRUE) {
          setTime(root["epochseconds"]);
          sched_motor.hour_door_open = root["hour_door_open"];
          sched_motor.minute_door_open = root["minute_door_open"];
          sched_motor.hour_door_close = root["hour_door_close"];
          sched_motor.minute_door_close = root["minute_door_close"];
          sched_motor.queens_delay = root["queens_delay"];
          sched_motor.config_enable = root["config_enable"];
          xSemaphoreGive(schedule_motor_mutex);
        }
      } else{
        Serial.printf("%s queenHiveUpdate: JSON object does not have valid keys: %s\n", getDateTime().c_str(), response.c_str());
      }
    } else {
      Serial.printf("%s queenHiveUpdate: connection error: status: %d\n", getDateTime().c_str(), status);
    }
    set_last_action_to_now();
    vTaskDelay(QUEENS_HIVE_UPDATE_SECONDS * 1000 / portTICK_PERIOD_MS);
  }
}

// Task send WiFi config to cients
// this task will only be initialized if type of hife: 0 -> bees drones hive
// --------------------------------------------------------- 
void sendWifiConfigToClients(void *pvParameters) {
  WiFiClient wifi;
  Serial.printf("%s Task sendWifiConfigToClients started\n", getDateTime().c_str());


  while(true) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (strcmp(state_client[i].ip, "0.0.0.0") == 0) {
        break;
      }

      if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {
        if (state_client[i].wifi_config_sent == 0) {
          HttpClient client = HttpClient(wifi, state_client[i].ip, 80);
          String contentType = "application/x-www-form-urlencoded";
          String postData = "ssid=" + wifi_config.ssid + "&pass=" + wifi_config.pass + "&ip=" + wifi_config.ip + "&gateway=" + wifi_config.gateway + "&dns=" + wifi_config.dns;
          int status = client.post("/", contentType, postData);
          int statuscode = 0;

          if (status == 0) {
            statuscode = client.responseStatusCode();
            String response = client.responseBody();
            state_client[i].wifi_config_sent = 1;
            Serial.printf("%s sendWifiConfigToClients: sent WiFi config to host: %s, status: %d, response: %s\n", getDateTime().c_str(), state_client[i].ip, statuscode, response.c_str());
          } else {
            Serial.printf("%s sendWifiConfigToClients: connection error: host: %s, status: %d\n", getDateTime().c_str(), state_client[i].ip, statuscode);
          }
        }
        xSemaphoreGive(config_mutex);
      } else {
        Serial.printf("%s sendWifiConfigToClients: cannot set WiFi config to client %s: mutex locked.\n", getDateTime().c_str(), state_client[i].ip);
      }
    }
    set_last_action_to_now();
    vTaskDelay(QUEENS_HIVE_UPDATE_SECONDS * 1000 / portTICK_PERIOD_MS);
  }
}

void queueMotorControl(const int steps, const MotorCommand command) {
  for (int i = 0; i < MAX_MOTOR; i++) {
    motor_ctrl[i] = {steps, command};
  }

  for (int i = 0; i < MAX_MOTOR; i++) {
    if (xQueueSend(motor_cmd_queue[i], (void *) &motor_ctrl[i], 1000) != pdTRUE) {
      Serial.printf("%s Queue %d full.\n", getDateTime().c_str(), i);
    }
  }
}

// use "setup & loop" task only to start init task
void setup() {
  xTaskCreate(initApp, "Init App", 4096, NULL, 1, NULL);
  vTaskDelete(NULL);
}
void loop() {
  Serial.println("This text should never be printed!");
}