#pragma once

#include "main.h"

void scheduleMotorCommands(void *);
void controlStepperMotor(void *);
bool moveMotor(AccelStepper *, const motor_init_t *, const int, const QueryEndSwitch);
