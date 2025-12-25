#include "network_wifi.h"

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
