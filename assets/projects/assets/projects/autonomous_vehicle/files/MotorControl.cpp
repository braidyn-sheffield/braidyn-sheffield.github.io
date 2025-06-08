#include "MotorControl.h"
#include <Arduino.h>

// Global PWM values for motor speed control
volatile int PWMval = 0;   // Main PWM duty cycle value
volatile int PWMval2 = 0;  // Secondary PWM value used for slight directional bias

// Initializes all motor-related pins and stops motors initially
void initMotors() 
{
  // Enable pins for front and back motors
  pinMode(FENA, OUTPUT);
  pinMode(FENB, OUTPUT);
  pinMode(BENA, OUTPUT);
  pinMode(BENB, OUTPUT);

  // Direction control pins for front motors
  pinMode(FIN1, OUTPUT);
  pinMode(FIN2, OUTPUT);
  pinMode(FIN3, OUTPUT);
  pinMode(FIN4, OUTPUT);

  // Direction control pins for back motors
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(BIN3, OUTPUT);
  pinMode(BIN4, OUTPUT);

  stop();  // Ensure all motors are stopped on startup
}

// Stops all motors by setting PWM to 0 and disabling all direction control pins
void stop() {
  analogWrite(FENA, 0);
  analogWrite(FENB, 0);
  analogWrite(BENA, 0);
  analogWrite(BENB, 0);

  digitalWrite(FIN1, LOW);
  digitalWrite(FIN2, LOW);
  digitalWrite(FIN3, LOW);
  digitalWrite(FIN4, LOW);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN3, LOW);
  digitalWrite(BIN4, LOW);
}

// Moves robot forward at a given speed (0–100%)
void moveFWD(int speed) {
  PWMval = 255 * speed / 100;  // Convert % speed to 0–255 PWM range

  // Set motor enable pins to speed value
  analogWrite(FENA, PWMval);
  analogWrite(FENB, PWMval);
  analogWrite(BENA, PWMval);
  analogWrite(BENB, PWMval);

  // Set direction pins for forward movement
  digitalWrite(FIN1, HIGH);
  digitalWrite(FIN2, LOW);
  digitalWrite(FIN3, LOW);
  digitalWrite(FIN4, HIGH);

  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN3, LOW);
  digitalWrite(BIN4, HIGH);
}

// Moves robot backward at a given speed (0–100%)
void moveBWD(int speed) {
  PWMval = 255 * speed / 100;

  analogWrite(FENA, PWMval);
  analogWrite(FENB, PWMval);
  analogWrite(BENA, PWMval);
  analogWrite(BENB, PWMval);

  // Set direction pins for reverse movement
  digitalWrite(FIN1, LOW);
  digitalWrite(FIN2, HIGH);
  digitalWrite(FIN3, HIGH);
  digitalWrite(FIN4, LOW);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  digitalWrite(BIN3, HIGH);
  digitalWrite(BIN4, LOW);
}

// Turns the robot right by biasing speed and setting opposite wheel directions
void moveRight(int speed) {
  PWMval = 255 * speed / 100;
  PWMval2 = 255 * (speed + 10) / 100;  // Slightly faster on one set for turning

  analogWrite(FENA, PWMval2);
  analogWrite(FENB, PWMval);
  analogWrite(BENA, PWMval2);
  analogWrite(BENB, PWMval);

  // Set direction pins for a right turn
  digitalWrite(FIN1, HIGH);
  digitalWrite(FIN2, LOW);
  digitalWrite(FIN3, HIGH);
  digitalWrite(FIN4, LOW);

  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  digitalWrite(BIN3, LOW);
  digitalWrite(BIN4, HIGH);
}

// Turns the robot left by biasing speed and setting opposite wheel directions
void moveLeft(int speed) {
  PWMval = 255 * speed / 100;
  PWMval2 = 255 * (speed + 10) / 100;

  analogWrite(FENA, PWMval);
  analogWrite(FENB, PWMval2);
  analogWrite(BENA, PWMval);
  analogWrite(BENB, PWMval2);

  // Set direction pins for a left turn
  digitalWrite(FIN1, LOW);
  digitalWrite(FIN2, HIGH);
  digitalWrite(FIN3, LOW);
  digitalWrite(FIN4, HIGH);

  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  digitalWrite(BIN3, HIGH);
  digitalWrite(BIN4, LOW);
}
