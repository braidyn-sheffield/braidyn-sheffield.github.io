#ifndef ULTRASONICCONTROL_H
#define ULTRASONICCONTROL_H

//---------------------------Ultrasonic Sensor Macros---------------------
#define TRIG1 27
#define ECHO1 28

#define TRIG2 29
#define ECHO2 30

#define TRIG3 31
#define ECHO3 32

// -------- Function to read distance from an ultrasonic sensor --------
void initUltrasonics();                  // Initializes ultrasonic sensor pins
long readUltrasonicDistance(int trigPin, int echoPin);

#endif
