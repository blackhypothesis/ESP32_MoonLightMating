#pragma once

#include "main.h"

void scheduleMotorCommands(void *);
void controlStepperMotor(void *);
void queueMotorControl(const MotorCommand, const int);
bool moveMotor(AccelStepper *, const motor_init_t *, const int, const QueryEndSwitch);
int secondsTillMotorStart(String);
