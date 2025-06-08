#include "ColorControl.h"
#include <Arduino.h>

// Array of structs holding the S2, S3, and OUT pins for each sensor
ColorSensorPins sensorPins[] = {
  {0, 0, 0},                     // Dummy 0-index (so indexing starts at 1)
  {S2_1, S3_1, OUT_1},           // Sensor 1 pin configuration
  {S2_2, S3_2, OUT_2},           // Sensor 2 pin configuration
  {S2_4, S3_4, OUT_4}            // Sensor 4 pin configuration
};

// Initializes color sensors and sets up control and output pins
void initColorSensors() {
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);

  // Set frequency scaling for TCS230/TCS3200 sensors to 100%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);  // LOW for 100% scaling

  // Configure sensor 1 pins
  pinMode(S2_1, OUTPUT);
  pinMode(S3_1, OUTPUT);
  pinMode(OUT_1, INPUT);

  // Configure sensor 2 pins
  pinMode(S2_2, OUTPUT);
  pinMode(S3_2, OUTPUT);
  pinMode(OUT_2, INPUT);

  // Configure sensor 4 pins
  pinMode(S2_4, OUTPUT);
  pinMode(S3_4, OUTPUT);
  pinMode(OUT_4, INPUT);
}

// Utility function to check if a value is within a tolerance of a target
bool inRange(int val, int target, int tol) {
  return abs(val - target) <= tol;
}

// Reads the pulse value for a specific color component from a sensor
// s2 and s3: color filter selection pins
// out: signal pin for the frequency-based output
// s2State, s3State: logic level for color selection (based on datasheet)
int readColorComponent(int s2, int s3, int out, bool s2State, bool s3State) {
  digitalWrite(s2, s2State);
  digitalWrite(s3, s3State);
  delay(10);  // Wait for filter settings to stabilize
  return pulseIn(out, LOW, 10000);  // Measure duration of LOW pulse (timeout = 10ms)
}

// Classifies a color reading based on pre-defined thresholds
// sensorIndex = 1 (Sensor 1), 2 (Sensor 2), 3 (Sensor 4)
// Returns: 2 = Blue, 3 = Red, 4 = Green, -1 = Unknown
int classifyColor(int sensorIndex) {
  // Check if index is valid
  if (sensorIndex < 1 || sensorIndex > 3) return -1;

  // Get pin config for the selected sensor
  ColorSensorPins p = sensorPins[sensorIndex];

  // Read red, green, and blue components by setting filter selection
  int r = readColorComponent(p.s2, p.s3, p.out, LOW, LOW);    // Red
  int g = readColorComponent(p.s2, p.s3, p.out, HIGH, HIGH);  // Green
  int b = readColorComponent(p.s2, p.s3, p.out, LOW, HIGH);   // Blue

  switch (sensorIndex) {

    // ------------------------------------
    // Sensor 1 – Detects RED and BLUE only
    // ------------------------------------
    case 1:
      // RED: R low, G & B higher
      if (r < 200 && g > 100 && b > 90) return 3;

      // BLUE: R high, G moderate, B lower
      if (r > 250 && g > 100 && b > 70 && r > g && r > b) return 2;
      break;

    // ------------------------------------
    // Sensor 2 – RGB classification
    // ------------------------------------
    case 2:
      // RED: low R, high G/B
      if (r >= 60 && r <= 100 && g >= 170 && g <= 280 && b >= 140 && b <= 240) return 3;

      // GREEN: all components lower
      if (r >= 70 && r <= 120 && g >= 60 && g <= 100 && b >= 90 && b <= 140) return 4;

      // BLUE: R mid-high, G/B lower
      if (r >= 180 && r <= 320 && g >= 90 && g <= 160 && b >= 60 && b <= 100) return 2;

      break;

    // ------------------------------------
    // Sensor 4 – RGB classification
    // ------------------------------------
    case 3:
      // RED: R medium, G/B high
      if (r >= 110 && r <= 170 && g >= 500 && g <= 750 && b >= 370 && b <= 540) return 3;

      // GREEN: R mid, G low-mid, B high
      if (r >= 140 && r <= 210 && g >= 110 && g <= 150 && b >= 170 && b <= 260) return 4;

      // BLUE: high R, mid G, mid-low B
      if (r >= 650 && r <= 900 && g >= 220 && g <= 340 && b >= 110 && b <= 180) return 2;

      break;
  }

  return -1;  // Return -1 if no valid match
}
