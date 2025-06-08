#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H

// -------- Motor Pin Definitions --------
#define FENA 18
#define FENB 19
#define FIN1 13
#define FIN2 14
#define FIN3 15
#define FIN4 39

#define BENA 22
#define BENB 23
#define BIN1 8
#define BIN2 9
#define BIN3 10
#define BIN4 11

// -------- Global PWM Variables --------
extern volatile int PWMval;
extern volatile int PWMval2;

// -------- Motor Control Functions --------
void initMotors();
void moveFWD(int speed);
void moveBWD(int speed);
void moveLeft(int speed);
void moveRight(int speed);
void stop();

#endif