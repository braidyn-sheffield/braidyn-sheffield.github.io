#include "ServoControl.h"
#include <Arduino.h>
#include <math.h>

// Declare Servo objects for controlling three servos
Servo Servo1, Servo2, Servo3;

// Initializes the servo motors by attaching them to the defined pins
// and setting Servo3 to an initial position
void initServos() {
  Servo1.attach(Servo1Pin);  // Attach Servo1 to its control pin
  Servo2.attach(Servo2Pin);  // Attach Servo2 to its control pin
  Servo3.attach(Servo3Pin);  // Attach Servo3 to its control pin

  Servo3.write(138);         // Set initial position for Servo3
  delay(500);                // Allow time for movement
}

// Moves all three servos simultaneously from current positions to target positions
// by interpolating intermediate steps with a specified delay between each step
void moveServosTogether(int target1, int target2, int target3, int delayMs) {
  int pos1 = Servo1.read(); // Get current position of Servo1
  int pos2 = Servo2.read(); // Get current position of Servo2
  int pos3 = Servo3.read(); // Get current position of Servo3

  // Calculate the maximum number of steps needed based on the largest movement
  int steps = max(max(abs(target1 - pos1), abs(target2 - pos2)), abs(target3 - pos3));
  if (steps == 0) return; // Exit if no movement needed

  // Calculate the incremental step values for each servo
  float step1 = (float)(target1 - pos1) / steps;
  float step2 = (float)(target2 - pos2) / steps;
  float step3 = (float)(target3 - pos3) / steps;

  // Incrementally move each servo to its target position
  for (int i = 0; i <= steps; i++) {
    Servo1.write(round(pos1 + step1 * i));
    Servo2.write(round(pos2 + step2 * i));
    Servo3.write(round(pos3 + step3 * i));
    delay(delayMs); // Delay between each step for smooth motion
  }
}

// Moves arm to stowed position after releasing or before gripping a box
// Configuration for when the gripper is empty
void stowModeEmpty() {
  moveServosTogether(50, 41, 138);
}

// Configuration for stowing after gripping a large box
void stowModeLarge() {
  moveServosTogether(50, 41, 112);
}

// Configuration for stowing after gripping a small box
void stowModeSmall() {
  moveServosTogether(50, 41, 103);
}

// Fully extends the arm to reach out
void extendMode() {
  moveServosTogether(80, 85, 103);
}

// Moves servos into position to grip a large box
void pickUpModeLarge() {
  moveServosTogether(93, 85, 112);
}

// Moves servos into position to grip a small box
void pickUpModeSmall() {
  moveServosTogether(93, 85, 103);
}

// Positions arm to allow front color sensor to read color of large box
void colorReadModeLarge() {
  moveServosTogether(82, 16, 112);
}

// Positions arm to allow front color sensor to read color of small box
void colorReadModeSmall() {
  moveServosTogether(82, 16, 103);
}

// Performs a waving animation by quickly oscillating Servo2
void waveMode()
{
  moveServosTogether(50, 46, 138); // Wave position 1
  delay(100);
  moveServosTogether(50, 32, 138); // Wave position 2
  delay(100);
  moveServosTogether(50, 46, 138); // Repeat
  delay(100);
  moveServosTogether(50, 32, 138);
  delay(100);
}
