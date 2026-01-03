#include "parameters.h"

const String VERSION = "0.22.2";

// create mutexes
SemaphoreHandle_t run_motor_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t setdatetime_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t config_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t state_client_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t last_action_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t seconds_till_door_move_mutex = xSemaphoreCreateMutex();
SemaphoreHandle_t schedule_motor_mutex = xSemaphoreCreateMutex();

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

const int QUEENS_HIVE_UPDATE_SECONDS = 10;

const int SLEEP_AFTER_INACTIVITY_SECONDS = 3600;
const int WAKEUP_BEFORE_MOTOR_MOVE_SECONDS = 30;

// Interrupt
const int INTERRUPT_PIN = 27;
volatile bool resetButtonPressed = false;

// Motor config
// GPIOs for motors
const int IN1 = 19;
const int IN2 = 5;
const int IN3 = 18;
const int IN4 = 17;

const int IN5 = 32;
const int IN6 = 25;
const int IN7 = 33;
const int IN8 = 26;

// to initialize motor
const int MAX_MOTOR = 2;
const int MAX_SPEED = 200;
const int ACCELERATION = 200;
// default values will be overwritten by values from hiveconfig.json
const int DEFAULT_OFFSET_OPEN_DOOR = -50;
const int DEFAULT_OFFSET_CLOSE_DOOR = -400;
const int DEFAULT_PHOTORESISTOR_EDGE_DELTA = 300;
const int DEFAULT_PHOTORESISTOR_READ_INTERVAL_MS = 400;

const int STEPS_ONE_TURN = 4096;

// end switches (analog)
const int END_SWITCH0 = 36;
const int END_SWITCH1 = 39;
const int END_SWITCH2 = 34;
const int END_SWITCH3 = 35;

// const motor_init_t motor_init[] = {{0, IN1, IN2, IN3, IN4, 200, 200}, {1, IN5, IN6, IN7, IN8, 200, 200}};
motor_init_t motor_init[] = {
    {0,
      IN1, IN2, IN3, IN4,
      MAX_SPEED, ACCELERATION,
      DEFAULT_OFFSET_OPEN_DOOR,
      DEFAULT_OFFSET_CLOSE_DOOR,
      DEFAULT_PHOTORESISTOR_EDGE_DELTA,
      DEFAULT_PHOTORESISTOR_READ_INTERVAL_MS
    },
    {1,
      IN5, IN6, IN7, IN8,
      MAX_SPEED, ACCELERATION,
      DEFAULT_OFFSET_OPEN_DOOR,
      DEFAULT_OFFSET_CLOSE_DOOR,
      DEFAULT_PHOTORESISTOR_EDGE_DELTA,
      DEFAULT_PHOTORESISTOR_READ_INTERVAL_MS
    }
};

motor_control_t motor_ctrl[MAX_MOTOR];

schedule_motor_t sched_motor = {0, 0, 0, 0, 0, 0};

// Queues
QueueHandle_t motor_cmd_queue[MAX_MOTOR];
QueueHandle_t log_queue;