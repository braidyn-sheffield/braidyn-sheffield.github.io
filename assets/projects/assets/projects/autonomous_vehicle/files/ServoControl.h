#ifndef SERVOCONTROL_H
#define SERVOCONTROL_H

#include <Servo.h>

// -------- Servo Pin Definitions --------
#define Servo1Pin 24
#define Servo2Pin 25
#define Servo3Pin 26

// -------- Servo Objects --------
extern Servo Servo1;
extern Servo Servo2;
extern Servo Servo3;

// -------- Functions --------
void initServos();
void moveServosTogether(int target1, int target2, int target3, int delayMs = 15);
void stowModeEmpty();
void stowModeLarge();
void stowModeSmall();
void extendMode();
void pickUpModeLarge();
void pickUpModeSmall();
void colorReadModeLarge();
void colorReadModeSmall();
void waveMode();

#endif
