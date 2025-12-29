#include "motor_control.h"

// Task: schedule motor commands 
// ---------------------------------------------------------
void scheduleMotorCommands(void *pvParameters) {
  int steps;
  enum MotorCommand command;
  int config_enable = 0;
  Serial.printf("%s Task scheduleMotorCommands started\n", getDateTime().c_str());

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
        steps = STEPS_ONE_TURN;
        command = MotorCommand::DOOR_OPEN;
        queueMotorControl(command, steps);
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    if (seconds_till_door_close < 2) {
      if (config_enable == 0) {
        Serial.printf("Schedule config disabled. Command for motors to close will not queued.\n");
      } else {
        steps = STEPS_ONE_TURN;
        command = MotorCommand::DOOR_CLOSE;
        queueMotorControl(command, steps);
      }
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// Task: control stepper motors
// ---------------------------------------------------------
void controlStepperMotor(void *pvParameters) {
  UBaseType_t uxHighWaterMark;
  const motor_init_t *mc = (motor_init_t *) pvParameters;
  motor_control_t cmd;
  bool res;

  Serial.printf("%s motor_init %d, %d, %d, %d, %d, %d, %d, %d, %d, %d\n", getDateTime().c_str(),
    mc->motor_nr, mc->in1, mc->in2, mc->in3, mc->in4, mc->max_speed, mc->acceleration, mc->offset_open_door, mc->offset_close_door, mc->photoresistor_edge_delta);
  AccelStepper stepper =
    AccelStepper(AccelStepper::HALF4WIRE, mc->in1, mc->in2, mc->in3, mc->in4);
  stepper.setMaxSpeed(mc->max_speed);
  stepper.setAcceleration(mc->acceleration);
  Serial.printf("%s Task controlStepperMotor started: motor_nr: %d\n", getDateTime().c_str(), mc->motor_nr);

  while(true) {
    // if motor is idle, try to get new command from queue ...
    if (stepper.distanceToGo() == 0) {
      if (xSemaphoreTake(run_motor_mutex, 1000) == pdTRUE) {
        if (xQueueReceive(motor_cmd_queue[mc->motor_nr], (void *) &cmd, 1000) == pdTRUE) {
          Serial.printf("%s Motor %d: received cmd: steps: %d; command: %d\n", getDateTime().c_str(), mc->motor_nr, cmd.steps, cmd.command);
          set_last_action_to_now();

          // .. notify clients ..
          JsonDocument motor_status;
          char serialized_motor_status[64];
          motor_status["motor_nr"] = mc->motor_nr;
          motor_status["command"] = cmd.command;
          serializeJson(motor_status, serialized_motor_status);
          // TODO: fix websocket message
          // notifyClients(String(serialized_motor_status));

          // .. and run the command
          switch(cmd.command) {

            case MotorCommand::RUN_ANTICLOCKWISE:
            case MotorCommand::RUN_CLOCKWISE:
              res = moveMotor(&stepper, mc, cmd.steps * cmd.command, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              break;

            case MotorCommand::DOOR_OPEN:
              res = moveMotor(&stepper, mc, STEPS_ONE_TURN, QueryEndSwitch::POSITIVE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, mc->offset_open_door, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, mc->offset_open_door, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              break;
            case MotorCommand::DOOR_CLOSE:
              res = moveMotor(&stepper, mc, -1 * mc->offset_close_door/2, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, -1 * STEPS_ONE_TURN, QueryEndSwitch::NEGATIVE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, mc->offset_close_door, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              break;
            case MotorCommand::MOTOR_INIT:
              res = moveMotor(&stepper, mc, STEPS_ONE_TURN, QueryEndSwitch::POSITIVE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, 400, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, -1 * STEPS_ONE_TURN, QueryEndSwitch::NEGATIVE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, -400, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, STEPS_ONE_TURN, QueryEndSwitch::POSITIVE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              res = moveMotor(&stepper, mc, -400, QueryEndSwitch::NONE);
              if (!res) {
                Serial.printf("%s Motor %d: Error during moving motor.\n", getDateTime().c_str(), mc->motor_nr);
              }
              break;
          }
          set_last_action_to_now();
          motor_status["command"] = MotorCommand::MOTOR_IDLE;
          serializeJson(motor_status, serialized_motor_status);
          // TODO: fix websocket message
          // notifyClients(String(serialized_motor_status));
          Serial.printf("%s Motor %d: executed cmd: steps: %d; command: %d\n", getDateTime().c_str(), mc->motor_nr, cmd.steps, cmd.command);
        }
        xSemaphoreGive(run_motor_mutex);
      }
      else {
        Serial.printf("%s Motor %d: other motor is currently running, waiting, ...\n", getDateTime().c_str(), mc->motor_nr);
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void queueMotorControl(const MotorCommand command, const int steps) {
  for (int i = 0; i < MAX_MOTOR; i++) {
    motor_ctrl[i] = {steps, command};
  }

  for (int i = 0; i < MAX_MOTOR; i++) {
    if (xQueueSend(motor_cmd_queue[i], (void *) &motor_ctrl[i], 1000) != pdTRUE) {
      Serial.printf("%s Queue %d full.\n", getDateTime().c_str(), i);
    }
  }
}

bool moveMotor(AccelStepper *stepper, const motor_init_t *mc, int steps, const QueryEndSwitch queryEndSwitch) {
  bool force_motor_stop = false;
  int end_switch_old = 0;
  int end_switch_current = 0;
  bool end_switch_first = true;
  TickType_t ticks;

  ticks = xTaskGetTickCount();
  stepper->move(steps);

  while(stepper->distanceToGo() != 0) {
    if ((xTaskGetTickCount() - ticks) / portTICK_PERIOD_MS > mc->photoresistor_read_interval_ms && force_motor_stop == false) {
      end_switch_current = analogRead(end_switch[mc->motor_nr]);

      if (end_switch_first) {
        end_switch_old = end_switch_current;
        end_switch_first = false;
      }
      int end_switch_delta = end_switch_current - end_switch_old;
      end_switch_old = end_switch_current;
      Serial.printf("%s End switch %d value = %d, delta = %d\n", getDateTime().c_str(), end_switch[mc->motor_nr], end_switch_current, end_switch_delta);
      if (abs(end_switch_delta) > mc->photoresistor_edge_delta) {
        if (end_switch_delta > 0) {
          Serial.printf("%s POSITIVE signal flank: photoresistor_edge_delta < end_switch_delta, %d < %d\n", getDateTime().c_str(), mc->photoresistor_edge_delta, end_switch_delta);
          if (queryEndSwitch == QueryEndSwitch::POSITIVE) {
            stepper->stop();
            force_motor_stop = true;
            Serial.printf("%s POSITIVE signal flank: motor stop.\n", getDateTime().c_str());
          }
        }
        if (end_switch_delta < 0) {
          Serial.printf("%s NEGATIVE signal flank: photoresistor_edge_delta < end_switch_delta, %d < %d\n", getDateTime().c_str(), mc->photoresistor_edge_delta, end_switch_delta);
          if (queryEndSwitch == QueryEndSwitch::NEGATIVE) {
            stepper->stop();
            force_motor_stop = true;
            Serial.printf("%s NEGATIVE signal flank: motor stop.\n", getDateTime().c_str());
          }
        }      
      }
      ticks = xTaskGetTickCount();
    }

    stepper->run();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
  // to save energy
  stepper->disableOutputs();

  if (queryEndSwitch != NONE && force_motor_stop == false) {
    return false;
  }

  return true;
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
