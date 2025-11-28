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

// ---------------------------------------------------------
// WiFi
// ---------------------------------------------------------

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
  file.close();
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
  file.close();
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
  int offset_open_door = documentRoot["offset_open_door"];
  int offset_close_door = documentRoot["offset_close_door"];
  int photoresistor_edge_delta = documentRoot["photoresistor_edge_delta"];
  if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {
    hive_config.hive_type = hive_type;
    hive_config.wifi_mode = wifi_mode;
    hive_config.offset_open_door = offset_open_door;
    hive_config.offset_close_door = offset_close_door;
    hive_config.photoresistor_edge_delta = photoresistor_edge_delta;
    xSemaphoreGive(config_mutex);
    Serial.printf("%s read Hive config from file.\n", getDateTime().c_str());
  } else {
    Serial.printf("%s cannot read Hive cnfig file: mutex locked.\n", getDateTime().c_str());
  }
}

void writeHiveConfigFile() {
  JsonDocument h_config;
  if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {
    h_config["hive_type"] = hive_config.hive_type;
    h_config["wifi_mode"] = hive_config.wifi_mode;
    h_config["offset_open_door"] = hive_config.offset_open_door;
    h_config["offset_close_door"] = hive_config.offset_close_door;
    h_config["photoresistor_edge_delta"] = hive_config.photoresistor_edge_delta;
    xSemaphoreGive(config_mutex);
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
  if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {
    wifi_config.ssid = String(ssid);
    wifi_config.pass = String(pass);
    wifi_config.ip = String(ip);
    wifi_config.gateway = String(gateway);
    wifi_config.dns = String(dns);
    xSemaphoreGive(config_mutex);
    Serial.printf("%s read WiFi config from file.\n", getDateTime().c_str());
  } else {
    Serial.printf("%s cannot read WiFi cnfig file: mutex locked.\n", getDateTime().c_str());
  }
}

void writeWifiConfigFile() {
  JsonDocument w_config;
  if (xSemaphoreTake(config_mutex, 200) == pdTRUE) {  
    w_config["ssid"] = wifi_config.ssid.c_str();
    w_config["pass"] = wifi_config.pass.c_str();
    w_config["ip"] = wifi_config.ip.c_str();
    w_config["gateway"] = wifi_config.gateway.c_str();
    w_config["dns"] = wifi_config.dns.c_str();
    xSemaphoreGive(config_mutex);
    Serial.printf("%s wrote WiFi cnfig to file.\n", getDateTime().c_str());
  } else {
    Serial.printf("%s cannot write WiFi cnfig file: mutex locked.\n", getDateTime().c_str());
  }
  char serialized_w_config[512];
  serializeJson(w_config, serialized_w_config);
  Serial.printf("Save WiFi config: %s\n", serialized_w_config);
  writeFile(SPIFFS, WIFI_CONFIG_FILE, serialized_w_config);
}

String getHiveConfig() {
  JsonDocument hive_c;
  hive_c["hive_type"] = hive_config.hive_type;
  hive_c["wifi_mode"] = hive_config.wifi_mode;
  hive_c["offset_open_door"] = hive_config.offset_open_door;
  hive_c["offset_close_door"] = hive_config.offset_close_door;
  hive_c["photoresistor_edge_delta"] = hive_config.photoresistor_edge_delta;
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

// Interrupt Service Routine (ISR)
void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
}

void interruptFunction() {
  Serial.printf("%s interrupt handler\n", getDateTime().c_str());
  actionBlink(5, 100);
  resetDefaultConfigs();
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  actionBlink(5, 100);
  ESP.restart();
}