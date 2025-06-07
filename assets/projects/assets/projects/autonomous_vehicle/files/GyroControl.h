#ifndef GYROCONTROL_H
#define GYROCONTROL_H

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// -------- Gyro Object and Variables --------
extern Adafruit_MPU6050 mpu;

extern float angleZ;
extern float gyroZ_bias;
extern unsigned long lastTime;

// -------- Functions --------
void initGyro();  // Initializes MPU6050 and performs calibration
void calibrateGyroZ();
void resetGyro();
void updateGyroAngle();
void gyroTurn(String direction, float targetDeg, int speed = 70);

#endif
