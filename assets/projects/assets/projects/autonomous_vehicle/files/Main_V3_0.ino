#include "MotorControl.h"
#include "ColorControl.h"
#include "ServoControl.h"
#include "GyroControl.h"
#include "UltrasonicControl.h"

//---------------------------------Control Macros---------------------------------//
#define MODE_BUTTON 6
#define START_BUTTON 7
#define GRIPPER_BUTTON 1
#define MODE_IND 12

//-------------------------------Global Variables--------------------------------//
int gMode = 0;
int gBoxColor = -1;
int gBoxSize = 0;
int gDropOffStatus = 0;

//-------------------- -------State Machine Enumerate----------------------------//

enum Vehicle_State {
  IDLE_STATE,
  MAIN_PEDESTAL_FIND_STATE,
  PICK_UP_STATE,
  BOX_IDENTIFICATION_STATE,
  FIND_RED_LINE_STATE,
  FIND_BLUE_LINE_STATE,
  FOLLOW_LINE_STATE,
  SMALL_PEDESTAL_FIND_STATE,
  LARGE_PEDESTAL_FIND_STATE,
  DROP_OFF_STATE,
  RETURN_TO_LINE_STATE,
  NAVIGATE_STATE,
  WALL_FOUND_STATE,
  FOLLOW_RIGHT_WALL_STATE,
  FOLLOW_LEFT_WALL_STATE,
  COURSE_EDGE_STATE,
  RIGHT_OPENING_STATE,
  LEFT_OPENING_STATE,
  FINISH_LINE_STATE
};

Vehicle_State currentState = IDLE_STATE;

void setup() 
{
  Serial.begin(115200);

  // Sensor and Motor Initialization
  initMotors();
  initUltrasonics();
  initColorSensors();
  initServos();
  initGyro();

  // Controls Set Up
  pinMode(MODE_BUTTON, INPUT_PULLUP);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(GRIPPER_BUTTON, INPUT_PULLUP);
  pinMode(MODE_IND, OUTPUT);

}

void loop() 
{
  switch (currentState) { // State Machine for Robot
  //--------------- IDLE_STATE ---------------//
  case IDLE_STATE: 
  {
    // Vehicle is stopped, waiting for start button to be pressed, moves to
    // MAIN_PEDESTAL_STATE or NAVIGATE_STATE depending on state of mode button.

    digitalWrite(MODE_IND, gMode); // Show current mode selection on LED
    stowModeEmpty(); // Move arm to default position

    // Toggle mode variable when mode button is pressed
    if (digitalRead(MODE_BUTTON) == 0)
    {
      gMode = !gMode;
      delay(250); // Delay for debouncing
    }

    // Start robot in the chosen mode
    if (digitalRead(START_BUTTON) == 0)
    {
      if (gMode == 1)
      {
        currentState = MAIN_PEDESTAL_FIND_STATE;
      }
      else
      {
        currentState = NAVIGATE_STATE;
      }
    }
    break;
  }

  //--------------- MAIN_PEDESTAL_FIND_STATE ---------------//
  case MAIN_PEDESTAL_FIND_STATE: 
  {
    // Vehicle moves towards pedestal, reading front ultrasonic sensor. Once front
    // ultrasonic sensor is below a certain threshold, move to PICK_UP_STATE.

    moveFWD(70);
    int distance = readUltrasonicDistance(TRIG1, ECHO1); // Read values from front ultrasonic sensor

    if (distance <= 21) // Check if vehicle is closer than 21 cm to the pedestal
    {
      int distance = readUltrasonicDistance(TRIG1, ECHO1); // Checks distance again for verification
      if (distance <= 21)
      {
        stop();
        currentState = PICK_UP_STATE; // Move to PICK_UP_STATE
      }
    }
    break;
  }

  //--------------- PICK_UP_STATE ---------------//
  case PICK_UP_STATE: 
  {
    // Vehicle is to pick up box. Close to large box size. If button on gripper is triggered,
    // the system moves to BOX_IDENTIFICATION_STATE. If the button is not triggered, the gripper
    // closes to the small box size. The system then movves to BOX_IDENTIFICATION_STATE. The size 
    // of the box is saved in a global variable called gBoxSize. 1 for large box, and 2 for small box.

    // Attempt to pick up large box
    moveServosTogether(96, 85, 138);
    delay(1000);
    pickUpModeLarge();
    delay(2000);
    
    // Check if the button on the gripper has been pressed
    if (digitalRead(GRIPPER_BUTTON) == 0)
    {
      stowModeLarge();
      delay(2000);
      gBoxSize = 1;
      currentState = BOX_IDENTIFICATION_STATE;
    }

    // Try and pick up small box if large box does not trigger gripper button
    else
    {
      delay(1000);
      pickUpModeSmall();
      delay(500);
      stowModeSmall();
      delay(2000);
      gBoxSize = 2;
      currentState = BOX_IDENTIFICATION_STATE;
    }

    break;
  }

  //--------------- BOX_IDENTIFICATION_STATE ---------------//
  case BOX_IDENTIFICATION_STATE: 
  {
    // Vehicle moves backwards until green line is found. The vehicle than moves the servos
    // to place the box in front of the color sensor. The box color is read and if the box is
    // red, the system goes to FIND_RED_LINE_STATE. If it is blue, it goes to FIND_BLUE_LINE_STATE
    // the box color is saved as a global variable called gBoxColor. gBoxColor is red if the box is 
    // red and 2 if the box is blue.

    moveBWD(70);

    // Keep moving backward until one of the color sensors detects the green line
    if ((classifyColor(2) == 4) || (classifyColor(3) == 4))
    {
      stop();

      if (gBoxSize == 1) // Large
      {
        colorReadModeLarge();
        delay(2000);
      
        if (classifyColor(1) == 3) // Red
        {
          gBoxColor = 3;

          currentState = FIND_RED_LINE_STATE;
        }
        else if (classifyColor(1) == 2) // Blue
        {
          gBoxColor = 2;

          currentState = FIND_BLUE_LINE_STATE;
        }

        stowModeLarge();

      }

      else // Small
      {
        colorReadModeSmall();
        delay(2000);

        if (classifyColor(1) == 3) // Red
        {
          gBoxColor = 3;

          currentState = FIND_RED_LINE_STATE;
        }
        else if (classifyColor(1) == 2) // Blue
        {
          gBoxColor = 2;

          currentState = FIND_BLUE_LINE_STATE;
        }

        stowModeSmall();

      }

    }

    break;
  }

  //--------------- FIND_RED_LINE_STATE ---------------//
  case FIND_RED_LINE_STATE: 
  {
      // Vehicle moves foward to the center of rotation of the vehicle. It then uses the
      // accelerometer to turn 90 degrees to the right. It then moves foward until the red
      // line is found. It then turns right again 90 degrees and moves to FOLLOW_LINE_STATE

      moveFWD(70);
      delay(1000);
      stop();
      gyroTurn("RIGHT", 93);

      while (true) 
      {
        int color = classifyColor(3);
        Serial.print("Sensor 4 color: ");
        Serial.println(color);

        if (color == 3) break;

      moveFWD(70);
      delay(50); // Give time to read the color properly
      }

      delay(1500);

      gyroTurn("RIGHT", 90);

      currentState = FOLLOW_LINE_STATE;

      break;
  }

  //--------------- FIND_BLUE_LINE_STATE ---------------//
  case FIND_BLUE_LINE_STATE: 
  {
    // Vehicle moves foward to the center of rotation of the vehicle. It then uses the
    // accelerometer to turn 90 degrees to the left. It then moves foward until the blue
    // line is found. It then turns left again 90 degrees and moves to FOLLOW_LINE_STATE

    moveFWD(70);
    delay(1000);
    stop();
    gyroTurn("LEFT", 85);

    while(classifyColor(2) != 2)
    {
      moveFWD(70);
    }

    delay(1500);

    gyroTurn("LEFT", 88);

    currentState = FOLLOW_LINE_STATE;

    break;
  }

  //--------------- FOLLOW_LINE_STATE ---------------//
  case FOLLOW_LINE_STATE: 
  {
    // The vehicle will follow the line until the second green line is found. Then, depending on the state of
    // gBoxSize, the system moves to SMALL_DEDESTAL_FIND_STATE, or LARGE_PEDESTAL_FIND_STATE. This state is also
    // used to return the vehicle to the first green line where the procedure is complete. The state will differentiate
    // by keeping track of the gDropOffStatus global variable. If the variable is 0, the drop off is not complete
    // and 1 after the drop off. The variable becomes 3 once the vehicle crosses the green line closest to the 
    // drop off point.

    moveFWD(70);

    int leftColor = classifyColor(2);
    int rightColor = classifyColor(3);

    // Detects if the left or right color sensor detects the same color of the box
    if ((rightColor == gBoxColor) || (leftColor == gBoxColor))
    {
      stop();

      // This is true at the cross when the vehicle needs to make a decision on what pedestal to go to
      if ((rightColor == gBoxColor) && (leftColor == gBoxColor))
      {
        if (gBoxSize == 1) // Large
        {
          currentState = LARGE_PEDESTAL_FIND_STATE;
        }
        if (gBoxSize == 2) // Small
        {
          currentState = SMALL_PEDESTAL_FIND_STATE;
        }
      }

      // The vehicle will make 5 degree corrective turns to stay following the line
      else if (rightColor == gBoxColor)
      {
        gyroTurn("RIGHT", 5);
      }

      else if (leftColor == gBoxColor)
      {
        gyroTurn("LEFT", 5);
      }     
      
    }

    // This is true if either of the left or right color sensors detects the green line
    else if ((rightColor == 4) || (leftColor == 4))
    {
      if (gDropOffStatus == 2)
      {
        stop();
        currentState = IDLE_STATE;
      }

      if (gDropOffStatus == 1)
      {
        gDropOffStatus++;
        delay(1000); // Delay is to allow the vehicle to cross the green line before reading sensors again
      }
    }

    break;
  }

  //--------- SMALL_PEDESTAL_FIND_STATE ---------//
  case SMALL_PEDESTAL_FIND_STATE: 
  {
    // The vehicle will move foward until the split is found, it then will turn right and
    // follow the line until the pedestal is found by using the front ultrasonic sensor. The
    // system then moves to DROP_OFF_STATE

    moveFWD(70);
    delay(1500);
    gyroTurn("RIGHT", 87);

    int finalStraight = 0;
    int dropOff = 0;

    while(!finalStraight)
    {
      int leftColor = classifyColor(2);

      moveFWD(70);

      if (leftColor == gBoxColor)
      {
        delay(1300);
        gyroTurn("LEFT", 90);
        finalStraight = 1;
      }  

    }

    while(!dropOff)
    {
      moveFWD(70);
      int distance = readUltrasonicDistance(TRIG1, ECHO1);

      if (distance <= 21)
      {
        int distance = readUltrasonicDistance(TRIG1, ECHO1);
        if (distance <= 21)
        {
          stop();
          dropOff = 1;
          currentState = DROP_OFF_STATE;
        }
      }

    }

    
    
    break;
  }

  //--------- LARGE_PEDESTAL_FIND_STATE ---------//
  case LARGE_PEDESTAL_FIND_STATE: 
  {
    // The vehicle will move foward until the split is found, it then will turn left and
    // follow the line until the pedestal is found by using the front ultrasonic sensor. The
    // system then moves to DROP_OFF_STATE

    moveFWD(70);
    delay(1500);
    gyroTurn("LEFT", 88);

    int finalStraight = 0;
    int dropOff = 0;

    while(!finalStraight)
    {
      int rightColor = classifyColor(3);

      moveFWD(70);

      if (rightColor == gBoxColor)
      {
        delay(1500);
        gyroTurn("RIGHT", 88);
        finalStraight = 1;
      }  

    }

    while(!dropOff)
    {
      moveFWD(70);
      int distance = readUltrasonicDistance(TRIG1, ECHO1);

      if (distance <= 21)
      {
        int distance = readUltrasonicDistance(TRIG1, ECHO1);
        if (distance <= 21)
        {
          stop();
          dropOff = 1;
          currentState = DROP_OFF_STATE;
        }
      }

    }

    break;
  }

  //--------------- DROP_OFF_STATE ---------------//
  case DROP_OFF_STATE: 
  {
    // The vehicle will drop the box off onto the pedestal. Then move to RETURN_TO_LINE_STATE

    if (gBoxSize == 1) // Large
    {
      pickUpModeLarge();
      delay(200);
      moveServosTogether(93, 85, 138);
      delay(200);
    }
    if (gBoxSize == 2) // Small
    {
      pickUpModeSmall();
      delay(200);
      moveServosTogether(93, 85, 138);
      delay(200);
    }

    stowModeEmpty();
    gDropOffStatus = 1;
    

    currentState = RETURN_TO_LINE_STATE;



    break;
  }

  //----------- RETURN_TO_LINE_STATE -----------//
  case RETURN_TO_LINE_STATE: 
  {
    // The vehicle will move backwards until the first return line is found. It will then move to 
    //the center of rotation and turn to start following the line again.

    int crossFound = 0;
    int lineFound = 0;

    moveBWD(70);
    delay(500);

    while (!crossFound)
    {
      int leftColor = classifyColor(2);
      int rightColor = classifyColor(3);

      if ((leftColor == gBoxColor) || (rightColor == gBoxColor))
      {
        stop();
        moveFWD(70);
        delay(1500);
        crossFound = 1;
      }

    }

    if (gBoxSize == 1)
    {
      gyroTurn("RIGHT", 87);

    }

    if (gBoxSize == 2)
    {
      gyroTurn("LEFT", 87);
    }

    moveFWD(70);

    while(!lineFound)
    {
      int leftColor = classifyColor(2);
      int rightColor = classifyColor(3);

      if ((leftColor == gBoxColor) || (rightColor == gBoxColor))
      {
        if (gBoxSize == 1)
        {
          delay(1500);
          gyroTurn("RIGHT", 87);

        }

        if (gBoxSize == 2)
        {
          delay(1500);
          gyroTurn("LEFT", 87);
        }

        lineFound = 1;
        stop();
        currentState = FOLLOW_LINE_STATE;

      }

    }

    break;
  }
  //--------------- NAVIGATE_STATE ---------------//
  case NAVIGATE_STATE: 
  {
    // Move forward and check for walls or the finish line by using the front ultrasonic sensor
    // and the color sensors.

    moveFWD(90);
    // Read front distance once
    int distanceFront = readUltrasonicDistance(TRIG1, ECHO1);

    // Check for wall ahead
    if (distanceFront <= 15) {
      stop();
      delay(100);
      // Confirm with a second reading to reduce false triggers
      distanceFront = readUltrasonicDistance(TRIG1, ECHO1);
      if (distanceFront <= 15) {
        Serial.println("Wall detected");
        currentState = WALL_FOUND_STATE;
        break;
      }
    }

    // Check finish line via side color sensors
    int color2 = classifyColor(2);
    int color4 = classifyColor(3);

    if (color2 == 4 || color4 == 4) {
      stop();
      currentState = FINISH_LINE_STATE;
      break;
    }

    // Brief delay to avoid sensor saturation
    delay(50);
    break;
  }

  //--------------- WALL_FOUND_STATE ---------------//
  case WALL_FOUND_STATE: 
  {
    // The vehicle will stop and turn left then currentState will move to NAVIGATION_STATE

    stop();
    delay(500);
    gyroTurn("LEFT", 87);

    currentState = FOLLOW_RIGHT_WALL_STATE;
    break;
  }

  //--------------- FOLLOW_RIGHT_WALL_STATE ---------------//
  case FOLLOW_RIGHT_WALL_STATE: 
  {
    // The vehicle moves foward until an opening on the right side is sensed by the
    // right ultrasonic sensor, or the boundry is detected by the color sensors. If an 
    // opening is found, the vehicle moves to RIGHT_OPENING_STATE. If the boundery is 
    // found, the system moves to COURSE_EDGE_STATE.

    moveFWD(90);

    int distanceRight = readUltrasonicDistance(TRIG3, ECHO3);

    if (distanceRight > 30)
    {
      distanceRight = readUltrasonicDistance(TRIG3, ECHO3);
      if (distanceRight > 30)
      {
        currentState = RIGHT_OPENING_STATE;
      }
    }

    else if ((classifyColor(2) == 1) && (classifyColor(3) == 1))
    {
      stop();
      delay(500);
      moveFWD(70);
      delay(150);
      if ((classifyColor(2) == 1) && (classifyColor(3) == 1))
      {
        currentState = COURSE_EDGE_STATE;
      }
    }
    break;
  }

  //--------------- FOLLOW_left_WALL_STATE ---------------//
  case FOLLOW_LEFT_WALL_STATE: 
  {
    // The vehicle moves foward until an opening on the left side is sensed by the
    // left ultrasonic sensor, or the boundry is detected by the color sensors. If an 
    // opening is found, the vehicle moves to LEFT_OPENING_STATE. If the boundery is 
    // found, the system moves to COURSE_EDGE_STATE.

    moveFWD(90);
    int distanceLeft = readUltrasonicDistance(TRIG2, ECHO2);

    if (distanceLeft > 30)
    {
      distanceLeft = readUltrasonicDistance(TRIG3, ECHO3);
      if (distanceLeft > 30)
      {
        currentState = LEFT_OPENING_STATE;
      }
    }
    
    else if ((classifyColor(2) == 1) && (classifyColor(3) == 1))
    {
      if ((classifyColor(2) == 1) && (classifyColor(3) == 1))
      {
        currentState = COURSE_EDGE_STATE;
      }
    }
    break;
  }

  //--------------- COURSE_EDGE_STATE ---------------//
  case COURSE_EDGE_STATE: 
  {
    // The vehicle will stop at the white line, turn around, then the system will check for a wall
    // using the side ultrasonic sensors and move to either FOLLOW_RIGHT_WALL_STATE or 
    // FOLLOW_LEFT_WALL_STATE. 

    stop();
    delay(1000);
    moveBWD(70);
    delay(2000);
    stop();

    gyroTurn("LEFT", 87);
    delay(1000);
    gyroTurn("LEFT", 87);

    if (readUltrasonicDistance(TRIG2, ECHO2) < 30)
    {
      currentState = FOLLOW_LEFT_WALL_STATE;
    }

    else
    {
      currentState = FOLLOW_RIGHT_WALL_STATE;
    }

    break;
  }

  //--------------- RIGHT_OPENING_STATE ---------------//
  case RIGHT_OPENING_STATE: 
  {
    // The vehicle will move foward so that the center of rotation is centered at the opening,
    // then the vehicle will do a right turn and the vehicle will move to NAVIGATE_STATE.

    moveFWD(90);
    delay(1700);

    gyroTurn("RIGHT", 87);

    currentState = NAVIGATE_STATE;

    break;
  }

  //--------------- LEFT_OPENING_STATE ---------------//
  case LEFT_OPENING_STATE: 
  {
    // The vehicle will move foward so that the center of rotation is centered at the opening,
    // then the vehicle will do a left turn and the system will move to NAVIGATE_STATE.

    moveFWD(90);
    delay(1700);

    gyroTurn("LEFT", 87);

    currentState = NAVIGATE_STATE;

    break;
  }

  //---------------FINISH_LINE_STATE ---------------//
  case FINISH_LINE_STATE: 
  {
    // The vehicle has found the finish line of the obstacle course, and will stop moving and move to IDLE_STATE
    
    stop();
    delay(1000);
    waveMode();
    delay(200);
    stowModeEmpty();

    currentState = IDLE_STATE;

    break;
  }
}
}

