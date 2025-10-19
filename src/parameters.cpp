#include "parameters.h"

const String VERSION = "0.17.18";

// type of hife: 0 -> bees drones hive, 1 -> bees queens hive
const int HIVE_DRONES = 0;
const int HIVE_QUEENS = 1;

const int MODE_WIFI_STA = 1;
const int MODE_WIFI_AP = 2;

const char *HIVE_DEFAULT_CONFIG_FILE = "/hiveconfig_default.json";
const char *HIVE_CONFIG_FILE = "/hiveconfig.json";

// const int WIFI_MODE_GPIO = 13;
const char *WIFI_DEFAULT_CONFIG_FILE = "/wificonfig_default.json";
const char *WIFI_CONFIG_FILE = "/wificonfig.json";

char root_html[16];
const char *DRONES_HTML = "/drones.html";
const char *QUEENS_HTML = "/queens.html";
const char *WIFI_CONFIG_HTML = "/config.html";

hive_cfg_t hive_config;
wifi_cfg_t wifi_config;

String mac_address = "00:00:00:00:00:00";
