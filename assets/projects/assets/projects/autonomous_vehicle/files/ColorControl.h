#ifndef COLORCONTROL_H
#define COLORCONTROL_H

// -------- Shared Frequency Scaling Pins --------
#define S0 33
#define S1 34

// -------- Color Sensor Pin Definitions --------
#define S2_1 35
#define S3_1 36
#define OUT_1 2

#define S2_2 37
#define S3_2 38
#define OUT_2 3

#define S2_4 41
#define S3_4 40
#define OUT_4 5

// -------- Color Codes --------
// 0 = Black, 1 = White, 2 = Blue, 3 = Red, 4 = Green, -1 = Unknown

// -------- Struct for Pin Mapping --------
struct ColorSensorPins {
  int s2;
  int s3;
  int out;
};

// -------- Functions --------
void initColorSensors();
int classifyColor(int sensorNumber);
int readColorComponent(int s2, int s3, int out, bool s2State, bool s3State);

#endif
