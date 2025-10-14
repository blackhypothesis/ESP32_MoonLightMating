#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoHttpClient.h>
// #include <ElegantOTA.h>
#include <AccelStepper.h>
#include <TimeLib.h>

#define DEBUG(...) sprintf(message, __VA_ARGS__); debug(task_name, message);

const String VERSION = "0.17.10";

// type of hife: 0 -> bees drones hive, 1 -> bees queens hive
const int HIVE_DRONES = 0;
const int HIVE_QUEENS = 1;

const int MODE_WIFI_STA = 1;
const int MODE_WIFI_AP = 2;

const char* HIVE_DEFAULT_CONFIG_FILE = "/hiveconfig_default.json";
const char* HIVE_CONFIG_FILE = "/hiveconfig.json";

// const int WIFI_MODE_GPIO = 13;
const char* WIFI_DEFAULT_CONFIG_FILE = "/wificonfig_default.json";
const char* WIFI_CONFIG_FILE = "/wificonfig.json";

char root_html[16];
const char* DRONES_HTML = "/drones.html";
const char* QUEENS_HTML = "/queens.html";
const char* WIFI_CONFIG_HTML = "/config.html";

typedef struct hive_cfg {
  int hive_type;
  int wifi_mode;
} hive_cfg_t;

hive_cfg_t hive_config;

typedef struct wifi_cfg {
  String ssid;
  String pass;
  String ip;
  String gateway;
  String dns;
} wifi_cfg_t;

wifi_cfg_t wifi_config;

String mac_address = "00:00:00:00:00:00";

const int QUEENS_HIVE_UPDATE_SECONDS = 10;

const int SLEEP_AFTER_INACTIVITY_SECONDS = 3600;
const int WAKEUP_BEFORE_MOTOR_MOVE_SECONDS = 30;

// Motor config
// GPIOs for motors
const int IN1 = 19;
const int IN2 = 5;
const int IN3 = 18;
const int IN4 = 17;

const int IN5 = 32;
const int IN6 = 33;
const int IN7 = 25;
const int IN8 = 26;

// to initialize motor
const int MAX_MOTOR = 2;
const int MOTOR_STEPS_OPEN_CLOSE = 1300;

typedef struct motor_init {
  int motor_nr;
  int in1, in2, in3, in4;
  int max_speed;
  int acceleration;
} motor_init_t;

// commands for motor
typedef struct motor_cmd {
  int steps;
  int direction;
} motor_cmd_t;

static const motor_init_t motor_init[] = { {0, IN1, IN2, IN3, IN4, 200, 200}, {1, IN5, IN6, IN7, IN8, 200, 200} };
static motor_cmd_t motor_cmd[MAX_MOTOR];

// end switches (analog)
const int END_SWITCH0 = 36;
const int END_SWITCH1 = 39;
const int END_SWITCH2 = 34;
const int END_SWITCH3 = 35;

const int end_switch[]= {END_SWITCH0, END_SWITCH1};

const int MAX_CLIENTS = 8;

// state of clients
typedef struct state_client {
  char ip[16];
  char mac[20];
  int active;
  int wifi_config_sent;
  time_t last_update_seconds;
} state_client_t;

static state_client_t state_client[MAX_CLIENTS];

// LED
const int LED = 2;

typedef struct schedule_motor {
  int hour_door_open;
  int minute_door_open;
  int hour_door_close;
  int minute_door_close;
  int queens_delay;
  int config_enable;
} schedule_motor_t;

// ---------------------------------------------------------
// static thread save variables
// ---------------------------------------------------------
// last action in seconds
static volatile time_t last_action_seconds;
// Schedule; door open close
static volatile int seconds_till_door_open;
static volatile int seconds_till_door_close;
// schedule motor
static schedule_motor_t sched_motor = {0, 0, 0, 0, 0, 0};

// mutex
static SemaphoreHandle_t run_motor_mutex;
static SemaphoreHandle_t setdatetime_mutex;
static SemaphoreHandle_t hive_config_mutex;
static SemaphoreHandle_t wifi_config_mutex;
static SemaphoreHandle_t state_client_mutex;
static SemaphoreHandle_t last_action_mutex;
static SemaphoreHandle_t seconds_till_door_move_mutex;
static SemaphoreHandle_t schedule_motor_mutex;

// Queues
static QueueHandle_t motor_cmd_queue[MAX_MOTOR];
static QueueHandle_t log_queue;

String getVersion() {
  String v;
  JsonDocument version;

  if (hive_config.hive_type == HIVE_DRONES) {
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

// write debug messages
char message[512];
void debug(char *task_name, char* message) {
    char task_message[192];
    sprintf(task_message, "%s task=%s %s", getDateTime().c_str(), task_name, message);
    Serial.printf("%s\n", task_message);
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

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }

  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

void resetDefaultConfigs() {
  String wifi_config = readFile(SPIFFS, WIFI_DEFAULT_CONFIG_FILE);
  writeFile(SPIFFS, WIFI_CONFIG_FILE, wifi_config.c_str());
  String hive_config = readFile(SPIFFS, HIVE_DEFAULT_CONFIG_FILE);
  writeFile(SPIFFS, HIVE_CONFIG_FILE, hive_config.c_str());
}

void readHiveConfigFile() {
  String config = readFile(SPIFFS, HIVE_CONFIG_FILE);
  Serial.println(config);
  JsonDocument cfg_json;
  DeserializationError error = deserializeJson(cfg_json, config.c_str());
  if (error) {
    Serial.println("Error: deserialization failed.");
    Serial.println(error.f_str());
  }
  JsonObject documentRoot = cfg_json.as<JsonObject>();
  int hive_type = documentRoot["hive_type"];
  int wifi_mode = documentRoot["wifi_mode"];
  if (xSemaphoreTake(hive_config_mutex, 200) == pdTRUE) {
    hive_config.hive_type = hive_type;
    hive_config.wifi_mode = wifi_mode;
    xSemaphoreGive(hive_config_mutex);
    Serial.printf("%s read Hive config from file.\n", getDateTime().c_str());
  } else {
    Serial.printf("%s cannot read Hive cnfig file: mutex locked.\n", getDateTime().c_str());
  }
}

void writeHiveConfigFile() {
  JsonDocument h_config;
  if (xSemaphoreTake(hive_config_mutex, 200) == pdTRUE) {
    h_config["hive_type"] = hive_config.hive_type;
    h_config["wifi_mode"] = hive_config.wifi_mode;
    xSemaphoreGive(hive_config_mutex);
    Serial.printf("%s wrote Hive cnfig to file.\n", getDateTime().c_str());
  } else {
    Serial.printf("%s cannot write Hive cnfig file: mutex locked.\n", getDateTime().c_str());
  }
  char serialized_h_config[512];
  serializeJson(h_config, serialized_h_config);
  Serial.printf("Save Hive config: %s\n", serialized_h_config);
  writeFile(SPIFFS, HIVE_CONFIG_FILE, serialized_h_config);
}

void readWifiConfigFile() {
  String config = readFile(SPIFFS, WIFI_CONFIG_FILE);
  Serial.println(config);
  JsonDocument cfg_json;
  DeserializationError error = deserializeJson(cfg_json, config.c_str());
  if (error) {
    Serial.println("Error: deserialization failed.");
    Serial.println(error.f_str());
  }
  JsonObject documentRoot = cfg_json.as<JsonObject>();
  const char* ssid = documentRoot["ssid"];
  const char* pass = documentRoot["pass"];
  const char* ip = documentRoot["ip"];
  const char* gateway = documentRoot["gateway"];
  const char* dns = documentRoot["dns"];
  if (xSemaphoreTake(wifi_config_mutex, 200) == pdTRUE) {
    wifi_config.ssid = String(ssid);
    wifi_config.pass = String(pass);
    wifi_config.ip = String(ip);
    wifi_config.gateway = String(gateway);
    wifi_config.dns = String(dns);
    xSemaphoreGive(wifi_config_mutex);
    Serial.printf("%s read WiFi config from file.\n", getDateTime().c_str());
  } else {
    Serial.printf("%s cannot read WiFi cnfig file: mutex locked.\n", getDateTime().c_str());
  }
}

void writeWifiConfigFile() {
  JsonDocument w_config;
  if (xSemaphoreTake(wifi_config_mutex, 200) == pdTRUE) {  
    w_config["ssid"] = wifi_config.ssid.c_str();
    w_config["pass"] = wifi_config.pass.c_str();
    w_config["ip"] = wifi_config.ip.c_str();
    w_config["gateway"] = wifi_config.gateway.c_str();
    w_config["dns"] = wifi_config.dns.c_str();
    xSemaphoreGive(wifi_config_mutex);
    Serial.printf("%s wrote WiFi cnfig to file.\n", getDateTime().c_str());
  } else {
    Serial.printf("%s cannot write WiFi cnfig file: mutex locked.\n", getDateTime().c_str());
  }
  char serialized_w_config[512];
  serializeJson(w_config, serialized_w_config);
  Serial.printf("Save WiFi config: %s\n", serialized_w_config);
  writeFile(SPIFFS, WIFI_CONFIG_FILE, serialized_w_config);
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

String getHiveConfig() {
  JsonDocument hive_c;
  hive_c["hive_type"] = hive_config.hive_type;
  hive_c["wifi_mode"] = hive_config.wifi_mode;
  char serialized_hive_c[128];
  serializeJson(hive_c, serialized_hive_c);
  return String(serialized_hive_c);
}

String getWifiConfig() {
  JsonDocument wifi_c;
  wifi_c["ssid"] = wifi_config.ssid;
  wifi_c["pass"] = wifi_config.pass;
  wifi_c["ip"] = wifi_config.ip;
  wifi_c["gateway"] = wifi_config.gateway;
  wifi_c["ssid"] = wifi_config.dns;
  char serialized_wifi_c[128];
  serializeJson(wifi_c, serialized_wifi_c);
  return String(serialized_wifi_c);
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


// Scan WiFi networks
void scanWiFi() {
  WiFi.mode(WIFI_STA);

  int n = WiFi.scanNetworks();
  Serial.println("WiFi scan done");
  if (n == 0) {
      Serial.println("no WiFi networks found");
  } 
  else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
      delay(10);
    }
  }
}

// ---------------------------------------------------------
// WiFi
// ---------------------------------------------------------

bool initWiFi() {
  unsigned long previousMillis = 0;
  const long interval = 10000;

  IPAddress localIP;
  IPAddress localGateway;
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns;

  if(wifi_config.ssid=="" || wifi_config.ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(wifi_config.ip.c_str());
  localGateway.fromString(wifi_config.gateway.c_str());
  dns.fromString(wifi_config.dns.c_str());

  // hive_type: Drones -> static IP config, Queens -> dynamic IP config
  if (hive_config.hive_type == HIVE_DRONES) {
    if (WiFi.config(localIP, localGateway, subnet, dns, dns) == false){
      Serial.println("WiFi STA Failed to configure");
      return false;
    }
  }
  WiFi.begin(wifi_config.ssid.c_str(), wifi_config.pass.c_str());
  Serial.print("Connecting to WiFi ");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    vTaskDelay(300 / portTICK_PERIOD_MS);
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }
  Serial.printf("\nMAC: %s IP: %s RSSI: %4d\n", WiFi.macAddress().c_str(), ip_addr_to_str(WiFi.localIP()).c_str(), WiFi.RSSI());
  return true;
}

// Initialize Access Point
bool initAP() {
  IPAddress localIP;
  IPAddress localGateway;
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns;

  if(wifi_config.ssid=="" || wifi_config.ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_AP);
  localIP.fromString(wifi_config.ip.c_str());
  localGateway.fromString(wifi_config.gateway.c_str());
  dns.fromString(wifi_config.dns.c_str());
  Serial.println("Set WiFi mode to AP.");

  WiFi.softAP(wifi_config.ssid.c_str(), wifi_config.pass.c_str());
  vTaskDelay(500 / portTICK_PERIOD_MS);
  if (WiFi.softAPConfig(localIP, localGateway, subnet) == false) {
    Serial.println("WiFi AP Failed to configure");
    return false;
  }

  Serial.print("Soft AP SSID:  ");
  Serial.println(WiFi.softAPSSID());
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("GW IP:         ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet Mask:   ");
  Serial.println(WiFi.subnetMask());
  return true;
}

// ---------------------------------------------------------
// Web Server
// ---------------------------------------------------------
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
// Create a WebSocket object
AsyncWebSocket ws("/ws");

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
      const int steps = control_motor["steps"];
      const int direction = control_motor["direction"];
      Serial.printf("steps = %d, direction = %d\n", steps, direction);  
      motor_cmd[0] = {steps, direction};
      motor_cmd[1] = {steps, direction};

      for (int i = 0; i < MAX_MOTOR; i++) {
        if (xQueueSend(motor_cmd_queue[i], (void *) &motor_cmd[i], 1000) != pdTRUE) {
          Serial.printf("Queue %d full.\n", i);
        }
      }
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

// Task: schedule motor commands 
// ---------------------------------------------------------
void scheduleMotorCommands(void *pvParameters) {
  int steps, direction;
  int config_enable = 0;

  while(true) {
    if (xSemaphoreTake(schedule_motor_mutex, 10) == pdTRUE) {
      config_enable = sched_motor.config_enable;
      xSemaphoreGive(schedule_motor_mutex);
    }

    if (xSemaphoreTake(seconds_till_door_move_mutex, 10) == pdTRUE) {
      seconds_till_door_open = secondsTillMotorStart("open");
      seconds_till_door_close = secondsTillMotorStart("close");
      xSemaphoreGive(seconds_till_door_move_mutex);
    }

    if (seconds_till_door_open < 2) {
      if (config_enable == 0) {
        Serial.printf("Schedule config disabled. Command for motors to open will not queued.\n");
      } else {
        steps = MOTOR_STEPS_OPEN_CLOSE;
        direction = 1;
        motor_cmd[0] = {steps, direction};
        motor_cmd[1] = {steps, direction};
        for (int i = 0; i < MAX_MOTOR; i++) {
          if (xQueueSend(motor_cmd_queue[i], (void *) &motor_cmd[i], 1000) != pdTRUE) {
            Serial.printf("Queue %d full.\n", i);
          }
        }
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (seconds_till_door_close < 2) {
      if (config_enable == 0) {
        Serial.printf("Schedule config disabled. Command for motors to close will not queued.\n");
      } else {
        steps = MOTOR_STEPS_OPEN_CLOSE;
        direction = -1;
        motor_cmd[0] = {steps, direction};
        motor_cmd[1] = {steps, direction};
        for (int i = 0; i < MAX_MOTOR; i++) {
          if (xQueueSend(motor_cmd_queue[i], (void *) &motor_cmd[i], 1000) != pdTRUE) {
            Serial.printf("Queue %d full.\n", i);
          }
        }
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}


// Task: control stepper motors
// ---------------------------------------------------------
void controlStepperMotor(void *pvParameters) {
  const motor_init_t *mc = (motor_init_t *) pvParameters;
  motor_cmd_t cmd;
  bool force_motor_stop = false;
  int end_switch_analog = 0;
  TickType_t ticks;

  Serial.printf("%s motor_init %d, %d, %d, %d, %d, %d, %d\n", getDateTime().c_str(), mc->motor_nr, mc->in1, mc->in2, mc->in3, mc->in4, mc->max_speed, mc->acceleration);
  AccelStepper stepper =
    AccelStepper(AccelStepper::HALF4WIRE, mc->in1, mc->in2, mc->in3, mc->in4);
  stepper.setMaxSpeed(mc->max_speed);
  stepper.setAcceleration(mc->acceleration);


  while(true) {
    // if motor is idle, try to get new command from queue
    if (stepper.distanceToGo() == 0) {
      if (xQueueReceive(motor_cmd_queue[mc->motor_nr], (void *) &cmd, 1000) == pdTRUE) {
        Serial.printf("%s Motor %d: received cmd: steps: %d; direction: %d\n", getDateTime().c_str(), mc->motor_nr, cmd.steps, cmd.direction);
        set_last_action_to_now();
        stepper.move(cmd.steps * cmd.direction);
      }
    }

    // if motor got a command, try to run the motor
    if (stepper.distanceToGo() != 0) {
      if (xSemaphoreTake(run_motor_mutex, 1000) == pdTRUE) {
        ticks = xTaskGetTickCount();

        JsonDocument motor_status;
        char serialized_motor_status[64];
        motor_status["motor_nr"] = mc->motor_nr;
        motor_status["direction"] = cmd.direction;
        serializeJson(motor_status, serialized_motor_status);
        notifyClients(String(serialized_motor_status));

        while (stepper.distanceToGo() != 0) {
          // read end switch, every 200 ms
          if ((xTaskGetTickCount() - ticks) / portTICK_PERIOD_MS > 200 && force_motor_stop == false) {
            end_switch_analog = analogRead(end_switch[mc->motor_nr]);
            // Serial.printf("%s End switch %d value = %d\n", getDateTime().c_str(), end_switch[mc->motor_nr], end_switch_analog);
            // stop motor, if end switch is on
            if (end_switch_analog < 500) {
              // do not stop motor, because there are no end switches build in
              // stepper.stop();
              // force_motor_stop = true;
            }
            ticks = xTaskGetTickCount();
          }

          stepper.run();
          vTaskDelay(1 / portTICK_PERIOD_MS);
        }

        // to save energy
        stepper.disableOutputs();
        xSemaphoreGive(run_motor_mutex);
        force_motor_stop = false;

        motor_status["direction"] = 0;
        serializeJson(motor_status, serialized_motor_status);
        notifyClients(String(serialized_motor_status));
        Serial.printf("%s Motor %d: executed cmd: steps: %d; direction: %d\n", getDateTime().c_str(), mc->motor_nr, cmd.steps, cmd.direction);
        set_last_action_to_now();
      }
      else {
        Serial.printf("%s Motor %d: other motor is currently running, waiting, ...\n", getDateTime().c_str(), mc->motor_nr);
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);    
  }
}

// Task: get configuration from server
// this task will only be initialized if type of hife: 1 -> bees queens hive
// ---------------------------------------------------------
void queenHiveUpdate(void *pvParameters) {
  const String CONFIG_PATH_NAME = "/getconfigstatusclient?mac=" + mac_address;
  WiFiClient wifi;

  while(true) {
    HttpClient client = HttpClient(wifi, wifi_config.ip.c_str(), 80);
    int status = client.get(CONFIG_PATH_NAME.c_str());
    
    if (status == 0) {
      int statusCode = client.responseStatusCode();
      String response = client.responseBody();

      Serial.print("Status code: ");
      Serial.println(statusCode);
      Serial.print("Response: ");
      Serial.println(response);

      JsonDocument config;

      DeserializationError error = deserializeJson(config, response.c_str());
      if (error) {
        Serial.println("Error:deserialization failed.");
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
        Serial.printf("%s JSON object does not have valid keys: %s\n", getDateTime().c_str(), response.c_str());
      }
    } else {
      Serial.printf("%s Connection error: status: %d\n", getDateTime().c_str(), status);
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

  while(true) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (strcmp(state_client[i].ip, "0.0.0.0") == 0) {
        break;
      }

      if (xSemaphoreTake(wifi_config_mutex, 200) == pdTRUE) {
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
            Serial.printf("%s Sent WiFi config to host: %s, status: %d, response: %s\n", getDateTime().c_str(), state_client[i].ip, statuscode, response.c_str());
          } else {
            Serial.printf("%s Connection error: host: %s, status: %d\n", getDateTime().c_str(), state_client[i].ip, statuscode);
          }
        }
        xSemaphoreGive(wifi_config_mutex);
      } else {
        Serial.printf("%s cannot set WiFi config to client %s: mutex locked.\n", getDateTime().c_str(), state_client[i].ip);
      }
    }
    set_last_action_to_now();
    vTaskDelay(QUEENS_HIVE_UPDATE_SECONDS * 1000 / portTICK_PERIOD_MS);
  }
}

// Task: app initialize
// ---------------------------------------------------------
void initApp(void *pvParameters) {
  time_t since_last_action_seconds;

  // create mutexes
  run_motor_mutex = xSemaphoreCreateMutex();
  setdatetime_mutex = xSemaphoreCreateMutex();
  hive_config_mutex = xSemaphoreCreateMutex();
  wifi_config_mutex = xSemaphoreCreateMutex();
  state_client_mutex = xSemaphoreCreateMutex();
  last_action_mutex = xSemaphoreCreateMutex();
  seconds_till_door_move_mutex = xSemaphoreCreateMutex();
  schedule_motor_mutex = xSemaphoreCreateMutex();

  Serial.begin(115200);
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  // Filesystem: init
  // ---------------------------------------------------------
  initFS();

  // read Hive / WiFi configs
  // ---------------------------------------------------------
  readWifiConfigFile();
  readHiveConfigFile();

  Serial.printf("%s Version %s\n", getDateTime().c_str(), getVersion().c_str());

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
      ESP.restart();
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
    motor_cmd_queue[i] = xQueueCreate(5, sizeof(motor_cmd_t));
  }
  // initialize log queue
  log_queue = xQueueCreate(5, 100);

  // initialize schedule motor commands task
  xTaskCreate(scheduleMotorCommands, "Schedule Motor Commands", 2048, NULL, 3, NULL);

  // initialize motor tasks
  const char *task_name[] = {"Control Stepper Motor 1", "Control Stepper Motor 2"};
  for (int i = 0; i < MAX_MOTOR; i++) {
    xTaskCreate(controlStepperMotor, task_name[i], 2048, (void *) &motor_init[i], 4, NULL);
  }

  // initialize drone hive task
  if (hive_config.hive_type == HIVE_DRONES) {
    xTaskCreate(sendWifiConfigToClients, "Send Wifi config", 4096, NULL, 2, NULL);
  }
  // initialize queen hive update task
  if (hive_config.hive_type == HIVE_QUEENS) {
    xTaskCreate(queenHiveUpdate, "Queen Hive Update", 4096, NULL, 2, NULL);
  }

  // Webserver: initialize
  initWebSocket();
  // ---------------------------------------------------------
  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, root_html, "text/html");
    set_last_action_to_now();
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
    for(int i=0;i<params;i++){
      const AsyncWebParameter* p = request->getParam(i);
      if (xSemaphoreTake(wifi_config_mutex, 200) == pdTRUE) {
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
        Serial.printf("%s get wifi config from POST request.\n", getDateTime().c_str());
      } else {
        Serial.printf("%s cannot get wifi config from POST request: mutex locked.\n", getDateTime().c_str());
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
  });


  server.on("/getversion", HTTP_GET, [](AsyncWebServerRequest *request){
    String version = getVersion();
    Serial.printf("%s getversion %s\n", getDateTime().c_str(), version.c_str());
    request-> send(200, "application/json", version);
    set_last_action_to_now();
  });

  server.on("/getdatetime", HTTP_GET, [](AsyncWebServerRequest *request){
    JsonDocument dt;
    dt["datetime"] = getDateTime();
    char serialized_dt[64];
    serializeJson(dt, serialized_dt);
    Serial.printf("%s getdatetime %s\n", getDateTime().c_str(), serialized_dt);
    request-> send(200, "application/json", String(serialized_dt));
    set_last_action_to_now();
  });

  server.on("/gethiveconfig", HTTP_GET, [](AsyncWebServerRequest *request){
    if (xSemaphoreTake(hive_config_mutex, 200) == pdTRUE) {
      JsonDocument hc;
      hc["hivetype"] = hive_config.hive_type;
      hc["wifimode"] = hive_config.wifi_mode;
      char serialized_hc[128];
      serializeJson(hc, serialized_hc);
      request-> send(200, "application/json", String(serialized_hc));
      set_last_action_to_now();
      xSemaphoreGive(hive_config_mutex);
      Serial.printf("%s /gethiveconfig %s\n", getDateTime().c_str(), serialized_hc);
    } else {
      Serial.printf("%s /gethiveconfig, cant't get config: mutex locked.\n", getDateTime().c_str());
    }
  });

  server.on("/getwificonfig", HTTP_GET, [](AsyncWebServerRequest *request){
    if (xSemaphoreTake(wifi_config_mutex, 200) == pdTRUE) {
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
      xSemaphoreGive(wifi_config_mutex);
      Serial.printf("%s /getwificonfig %s\n", getDateTime().c_str(), serialized_wc);
    } else {
      Serial.printf("%s /getwificonfig, cant't get config: mutex locked.\n", getDateTime().c_str());
    }
  });

  server.on("/resetdefaultconfig", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.printf("%s /resetdefaultconfig %s\n", getDateTime().c_str());
    resetDefaultConfigs();
    ESP.restart();
  });

  server.on("/getconfigstatus", HTTP_GET, [](AsyncWebServerRequest *request){
    IPAddress client_ip = request->client()->remoteIP();
    char client_ip_c[16];
    strcpy(client_ip_c, ip_addr_to_str(client_ip).c_str());
    String config_status = getConfigStatus();
    Serial.printf("%s getconfigstatus %s %s\n", getDateTime().c_str(), client_ip_c, config_status.c_str());
    request->send(200, "application/json", config_status);
    set_last_action_to_now();
  });

  server.on("/getconfigstatusclient", HTTP_GET, [](AsyncWebServerRequest *request){
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
  });

  server.on("/setdatetime", HTTP_GET, [](AsyncWebServerRequest *request){
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
  });

  server.on("/setscheduleconfig", HTTP_GET, [](AsyncWebServerRequest *request) {
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
      Serial.printf("%s setscheduleconfig %s\n", getDateTime().c_str(), serialized_hiveconfig);
      request->send(200, "application/json", String(serialized_hiveconfig));
      set_last_action_to_now();
    }
  });
   
  server.on("/getclientstates", HTTP_GET, [](AsyncWebServerRequest *request){
    String client_states = getClientStates();
    Serial.printf("%s, getclientstates %s\n", getDateTime().c_str(), client_states.c_str());
    request->send(200, "application/json", client_states);
    set_last_action_to_now();
  });

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
    vTaskDelay(1000 / portTICK_PERIOD_MS);
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