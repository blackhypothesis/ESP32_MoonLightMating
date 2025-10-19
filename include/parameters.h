#ifndef PARAMETERS_H
#define PARAMETERS_H

#include "Arduino.h"

extern const String VERSION;

// mutex
extern SemaphoreHandle_t run_motor_mutex;
extern SemaphoreHandle_t setdatetime_mutex;
extern SemaphoreHandle_t config_mutex;
extern SemaphoreHandle_t state_client_mutex;
extern SemaphoreHandle_t last_action_mutex;
extern SemaphoreHandle_t seconds_till_door_move_mutex;
extern SemaphoreHandle_t schedule_motor_mutex;

// type of hife: 0 -> bees drones hive, 1 -> bees queens hive
extern const int HIVE_DRONES;
extern const int HIVE_QUEENS;

extern const int MODE_WIFI_STA;
extern const int MODE_WIFI_AP;

extern const char *HIVE_DEFAULT_CONFIG_FILE;
extern const char *HIVE_CONFIG_FILE;

// const int WIFI_MODE_GPIO = 13;
extern const char *WIFI_DEFAULT_CONFIG_FILE;
extern const char *WIFI_CONFIG_FILE;

extern char root_html[16];
extern const char *DRONES_HTML;
extern const char *QUEENS_HTML;
extern const char *WIFI_CONFIG_HTML;



typedef struct hive_cfg
{
    int hive_type;
    int wifi_mode;
} hive_cfg_t;

extern hive_cfg_t hive_config;

typedef struct wifi_cfg
{
    String ssid;
    String pass;
    String ip;
    String gateway;
    String dns;
} wifi_cfg_t;

extern wifi_cfg_t wifi_config;

extern String mac_address;

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

typedef struct motor_init
{
    int motor_nr;
    int in1, in2, in3, in4;
    int max_speed;
    int acceleration;
} motor_init_t;

// commands for motor
typedef struct motor_cmd
{
    int steps;
    int direction;
} motor_cmd_t;

static const motor_init_t motor_init[] = {{0, IN1, IN2, IN3, IN4, 200, 200}, {1, IN5, IN6, IN7, IN8, 200, 200}};
static motor_cmd_t motor_cmd[MAX_MOTOR];

// end switches (analog)
const int END_SWITCH0 = 36;
const int END_SWITCH1 = 39;
const int END_SWITCH2 = 34;
const int END_SWITCH3 = 35;

const int end_switch[] = {END_SWITCH0, END_SWITCH1};

const int MAX_CLIENTS = 8;

// state of clients
typedef struct state_client
{
    char ip[16];
    char mac[20];
    int active;
    int wifi_config_sent;
    time_t last_update_seconds;
} state_client_t;

static state_client_t state_client[MAX_CLIENTS];

// LED
const int LED = 2;

typedef struct schedule_motor
{
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

#endif