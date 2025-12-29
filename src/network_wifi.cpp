#include "network_wifi.h"

// ---------------------------------------------------------
// WiFi
// ---------------------------------------------------------

// Scan WiFi networks
void scanWiFi() {
  WiFi.mode(WIFI_STA);

  int n = WiFi.scanNetworks();
  Serial.printf("%s WiFi scan done.\n", getDateTime().c_str());
  if (n == 0) {
      Serial.printf("%s no WiFi networks found\n", getDateTime().c_str());
  } 
  else {
    Serial.print(n);
    Serial.printf("%s networks found\n", getDateTime().c_str());
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.printf("%s %d\n", getDateTime().c_str(), i + 1);
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
    Serial.printf("%s Undefined SSID or IP address.\n", getDateTime().c_str());
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(wifi_config.ip.c_str());
  localGateway.fromString(wifi_config.gateway.c_str());
  dns.fromString(wifi_config.dns.c_str());

  // hive_type: Drones -> static IP config, Queens -> dynamic IP config
  if (hive_config.hive_type == HIVE_DRONES) {
    if (WiFi.config(localIP, localGateway, subnet, dns, dns) == false){
      Serial.printf("%s WiFi STA Failed to configure.\n", getDateTime().c_str());
      return false;
    }
  }
  WiFi.begin(wifi_config.ssid.c_str(), wifi_config.pass.c_str());
  Serial.printf("%s Connecting to WiFi ", getDateTime().c_str());

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    vTaskDelay(300 / portTICK_PERIOD_MS);
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.printf("%s Failed to connect.", getDateTime().c_str());
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
    Serial.printf("%s Undefined SSID or IP address.\n", getDateTime().c_str());
    return false;
  }

  WiFi.mode(WIFI_AP);
  localIP.fromString(wifi_config.ip.c_str());
  localGateway.fromString(wifi_config.gateway.c_str());
  dns.fromString(wifi_config.dns.c_str());
  Serial.printf("%s Set WiFi mode to AP.\n", getDateTime().c_str());

  WiFi.softAP(wifi_config.ssid.c_str(), wifi_config.pass.c_str());
  vTaskDelay(500 / portTICK_PERIOD_MS);
  if (WiFi.softAPConfig(localIP, localGateway, subnet) == false) {
    Serial.printf("%s WiFi AP Failed to configure.\n", getDateTime().c_str());
    return false;
  }

  Serial.printf("%s Soft AP SSID: %s\n", getDateTime().c_str(), WiFi.softAPSSID().c_str());
  Serial.printf("%s AP IP address: %s\n", getDateTime().c_str(), WiFi.softAPIP().toString().c_str());
  Serial.printf("%s GW IP: %s\n", getDateTime().c_str(), WiFi.gatewayIP().toString().c_str());
  Serial.printf("%s Subnet Mask: %s\n", getDateTime().c_str(), WiFi.subnetMask().toString().c_str());
  return true;
}
