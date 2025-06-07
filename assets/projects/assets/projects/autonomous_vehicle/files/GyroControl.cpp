#include "GyroControl.h"
#include <Wire.h>                    // For I2C communication
#include <Adafruit_MPU6050.h>       // MPU6050 sensor library
#include <Adafruit_Sensor.h>        // Required for sensor event objects
#include <Arduino.h>
#include "MotorControl.h"           // For motor movement functions

// Create MPU6050 object
Adafruit_MPU6050 mpu;

// Variables to track gyro rotation angle and bias
float angleZ = 0.0;                 // Integrated Z-axis angle (in degrees)
float gyroZ_bias = 0.0;            // Bias to correct for gyro drift
unsigned long lastTime = 0;        // Time of last gyro update

// Initializes the gyroscope (MPU6050) and calibrates Z-axis gyro
void initGyro() {
  Wire1.begin();                   // Start I2C on Wire1

  // Try to connect to the MPU6050 sensor
  if (!mpu.begin(MPU6050_I2CADDR_DEFAULT, &Wire1)) {
    Serial.println("MPU6050 not detected on Wire1!");
    while (1) delay(10);          // Halt if not detected
  }

  Serial.println("MPU6050 connected.");

  // Configure sensor ranges and filtering
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);      // ±8g range
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);           // ±250°/s range
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);        // Noise filtering
  delay(1000);                                        // Wait for sensor to stabilize

  calibrateGyroZ();  // Measure average gyro offset
}

// Calibrates Z-axis gyro by averaging multiple samples while stationary
void calibrateGyroZ() {
  float sum = 0;
  sensors_event_t a, g, temp;

  // Take 100 samples of Z-axis gyro data
  for (int i = 0; i < 100; i++) {
    mpu.getEvent(&a, &g, &temp);
    sum += g.gyro.z;  // Z-axis angular velocity in rad/s
    delay(5);         // Small delay between samples
  }

  gyroZ_bias = sum / 100.0;  // Store average bias
  Serial.print("Gyro Z bias: ");
  Serial.println(gyroZ_bias, 6);  // Print with 6 decimal places
}

// Resets the integrated angle and timing
void resetGyro() {
  angleZ = 0.0;
  lastTime = millis();
}

// Updates the integrated Z angle by reading gyro and computing angular displacement
void updateGyroAngle() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);  // Get latest sensor data

  unsigned long now = millis();        // Current time
  float dt = (now - lastTime) / 1000.0; // Convert time delta to seconds
  lastTime = now;

  // Convert gyro reading from rad/s to deg/s and subtract bias
  float degPerSec = (g.gyro.z - gyroZ_bias) * (180.0 / PI);

  angleZ += degPerSec * dt;  // Integrate to get angle
}

// Performs a turn using the gyroscope until reaching a target degree angle
// dir: "LEFT" or "RIGHT", targetDeg: degrees to turn, speed: motor speed (0–100)
void gyroTurn(String dir, float targetDeg, int speed) {
  resetGyro();                          // Clear previous angle and start time
  unsigned long startTime = millis();   // For timeout safety

  // Continue turning until target angle reached or timeout
  while (abs(angleZ) < targetDeg && millis() - startTime < 5000) {
    updateGyroAngle();  // Update angle reading

    if (dir == "LEFT") moveLeft(speed);     // Turn left
    else if (dir == "RIGHT") moveRight(speed); // Turn right

    delay(5);  // Short delay to allow time for motors to act and sensors to update
  }

  stop();       // Stop all motors once target reached or timeout
  delay(300);   // Allow robot to settle after stopping
}
