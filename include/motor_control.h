#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "main.h"

void scheduleMotorCommands(void *);
void controlStepperMotor(void *);
bool moveMotor(AccelStepper *, const motor_init_t *, const int, const QueryEndSwitch);

#endif
