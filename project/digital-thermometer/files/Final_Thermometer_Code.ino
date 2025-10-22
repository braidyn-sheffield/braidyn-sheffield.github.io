/*
* Program for the PCB design of the thermometer. The PCB board uses five
* push buttons, an LED, an OLED screen, a temp sensor, and an IMU (MPU6050).
* The program makes use of two ISR's for checking if the ON/OFF button or 
* the Lock button has been pressed, as well as two other ISR's for timer 1
* and timer 2. Timer 1 is used to check the current tempurature every 1 second,
* timer 2 is used to allow for the Calibration button and the Unit change button
* to be pressed and held for three seconds. The current temp and whether the 
* thermometer is locked is displayed on the OLED. The IMU controls the orientation
* of the screen. 
*/

// All required libraries that are needed
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DallasTemperature.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "images.h"

// Initializing macros for all the inputs and outputs
#define CAL_BUTTON     5
#define ON_OFF_BUTTON  2
#define ONE_WIRE_BUS   8
#define TEMP_ANALOG    A0
#define UNIT_BUTTON    4
#define LOCK_BUTTON    3
#define LIGHT_BUTTON   6
#define LED            9

// Macros for Timer 1 and Timer 2
#define TIMER1_COMPARE_VALUE 62500 // 1 second
#define TIMER2_COMPARE_VALUE 250 // 4 ms
#define TIMER2_COUNTER_MAX   750 // 3 Seconds

/**********************************************************************
*                       OLED Screen Set Up
**********************************************************************/

#define SCREEN_WIDTH   128 // OLED display width, in pixels
#define SCREEN_HEIGHT  32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


/**********************************************************************
*                   Temperature Sensor Set Up
**********************************************************************/

OneWire oneWire(ONE_WIRE_BUS);// Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);// Pass our oneWire reference to Dallas Temperature.
DeviceAddress insideThermometer; // Arrays to hold device address

/**********************************************************************
*                             IMU Set Up
**********************************************************************/

MPU6050 accelgyro;
int16_t ax, ay, az;


/**********************************************************************
*                           Global Variables
**********************************************************************/

volatile int gOnOffFlag = 0; // Stores state of on/off button
volatile int gLockFlag = 1; // Stores state of lock button
volatile float gCurrentTemp = 0; // Stores current temperature
volatile int gLockUnlock = 0; // Stores state of lock or unlock
volatile int gUnitToggle = 2; // Stores the current unit. Based on index on "images.h"
volatile int gTimer2Counter = 0; // Counter for Timer 2 allowing to turn it from 4 us to 3 sec
volatile int gCalFlag = 0; // Flag that is used for the calibration button in Timer 2
int gLEDToggle = 0; // Store state of LED button

// Enumerate that creates the states of the state machine
enum Thermometer_State {
  OFF_STATE,
  IDLE_STATE,
  UNITS_PRESSED_STATE,
  CAL_PRESSED_STATE
};

Thermometer_State currentState = OFF_STATE; // Start in the off state.

void setup() 
{
  Serial.begin(9600); // Initialize Serial Communication

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.display(); // Show initial display buffer contents on the screen
  delay(500); // Pause for 0.5 seconds

  display.clearDisplay(); // Clear the buffer

  // Joins the I2C bus for the tempurature sensor
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin();
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif

  accelgyro.initialize(); // Initialize device

  // Sets all external components as inputs or outputs
  pinMode(CAL_BUTTON, INPUT);
  pinMode(ON_OFF_BUTTON, INPUT);
  pinMode(TEMP_ANALOG, INPUT);
  pinMode(UNIT_BUTTON, INPUT);
  pinMode(LOCK_BUTTON, INPUT);
  pinMode(LIGHT_BUTTON, INPUT);
  pinMode(LED, OUTPUT);

  // Tempurature sensor initialization
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) 
  {
    sensors.setResolution(insideThermometer, 9);
  } 

  attachInterrupt(digitalPinToInterrupt(ON_OFF_BUTTON), onOffToggle, FALLING); // ISR for the ON/OFF Button
  attachInterrupt(digitalPinToInterrupt(LOCK_BUTTON), lockToggle, FALLING); // ISR for the Lock Button

  noInterrupts(); // Disable Interrupts

  // Init Timer 1 (16 bit)
  // Speed of Timer1 = 16MHz/256 = 62.5kHz (16us)
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = TIMER1_COMPARE_VALUE;
  TCCR1B |= (1<<WGM12); // CTC Mode
  TIMSK1 |= (1<<OCIE1A); // enable interrupt

  // Init Timer 2 (8 bit)
  // Speed of Timer1 = 16MHz/256 = 62.5kHz (1.6us)
  TCCR2A = 0;
  TCCR2B = 0;
  OCR2A = TIMER2_COMPARE_VALUE;
  TCCR2A |= (1<<WGM21); // CTC Mode

  // Start Timer
  TIMSK2 |= (1<<OCIE2A); // enable interrupt

  interrupts(); // Enable Interrupts
}

void loop() 
{
  // Switch for the state machine
  switch(currentState){
    case OFF_STATE: // Initial state. Screen is off
      checkButtons(); // External function that checks the states of the push buttons

      display.clearDisplay();  // Clear the display buffer
      display.display();       // Update the display to show the cleared buffer
      delay(100);
      break;
    
    case IDLE_STATE: // Normal state for the thermometer. Displays current temp and lock/unlock symbol
      checkButtons(); // Checks for changes in the push buttons

      display.clearDisplay(); // Clears the display

      // Prints the lock or unlock symbol depending if the screen is locked or not.
      // This is changed in the lockToggle ISR
      display.drawBitmap(0,0, images[gLockUnlock], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE); 

      // Prints the degree symbol and the selected unit. The symbol is selected within Timer 1
      display.drawBitmap(5,2, images[gUnitToggle], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE); 

      display.setTextColor(SSD1306_WHITE); // Sets the text color to white

      accelgyro.getAcceleration(&ax, &ay, &az); // Gets the acceleration information of the IMU
      
      // Checks the value of the IMU information and sets the orientation of the screen as needed
      if (ax < -5000)
      {
        display.setRotation(2);
      }
      else
      {
        display.setRotation(0);
      }

      display.setTextSize(1); // Draw 1X-scale test
      display.setCursor(10, 0); // Sets cursor position on the OLED
      display.print("Tempurature");

      display.setTextSize(2); // Draw 2X-scale test
      display.setCursor(10, 10); // Sets cursor position on the OLED
    
      display.print(gCurrentTemp); // Prints the current tempruature on the OLED

      display.display();
      delay(100);
      break;

    // State machine is moved to this state when the units button is pressed. 
    case UNITS_PRESSED_STATE:
      if (digitalRead(UNIT_BUTTON) == LOW) // Checks if the units button is depressed.
      {
        currentState = IDLE_STATE; // Moves state machine back to normal state
        TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20)); // Turns off Timer 2
        gTimer2Counter = 0; // Resets the counter for Timer 2
      }
      // The else statement works exactly as the normal idle state except a small dot in the lower left is displayed 
      // showing that the unit button is being pressed
      else 
      {
        display.clearDisplay();

        // Prints the lock or unlock symbol depending if the screen is locked or not.
        // This is changed in the lockToggle ISR
        display.drawBitmap(0,0, images[gLockUnlock], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

        // Prints the degree symbol and the selected unit. The symbol is selected within Timer 1
        display.drawBitmap(5,2, images[gUnitToggle], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

        // Prints the little white dot in the lower left
        display.drawBitmap(5,2, images[4], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE); 

        display.setTextColor(SSD1306_WHITE); // Sets the text color to white

        accelgyro.getAcceleration(&ax, &ay, &az); // Gets the acceleration information of the IMU

        // Checks the value of the IMU information and sets the orientation of the screen as needed
        if (ax < -5000)
        {
          display.setRotation(2);
        }
        else
        {
          display.setRotation(0);
        }
      }

      display.setTextSize(1); // Draw 1X-scale test
      display.setCursor(10, 0); // Sets cursor position on the OLED
      display.print("Tempurature");

      display.setTextSize(2); // Draw 2X-scale test
      display.setCursor(10, 10); // Sets cursor position on the OLED
    
      display.print(gCurrentTemp); // Prints the current tempruature on the OLED

      display.display();
      delay(100);
      break;

    // State machine is moved to this state when the calibration button is pressed
    case CAL_PRESSED_STATE:
      if (digitalRead(CAL_BUTTON) == LOW) // Checks if the calibration button is depressed
      {
        currentState = IDLE_STATE; // Moves the state machine back to the normal idle state
        TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20)); // Turns off Timer 2
        gTimer2Counter = 0; // Resets the counter for Timer 2
      }
      // The else statement works exactly as the normal idle state except a small dot in the lower left is displayed 
      // showing that the calibration button is being pressed. The only change is that if the calibration button
      // is pressed for three seconds, it prints a message to the OLED then moves back to the normal idle state
      else
      {
        if (gCalFlag == 1) // Checks if the calibration flag is activated
        {
          display.clearDisplay();  // Clear the display buffer
          display.setTextSize(1); // Draw 1X-scale test
          display.setTextColor(SSD1306_WHITE); // Turns the text color to white
          display.setCursor(0, 10); // Sets cursor position on the OLED

          // Prints the required message to the OLED
          display.println("Calibrating."); 
          display.println("Please Hold!");

          display.display();       // Update the display to show the cleared buffer
          delay(100);
          gCalFlag = 0; // Resets the calibration flag
          delay(2000); // Shows the message for two seconds
          currentState = IDLE_STATE; // Moves the state machine back to the normal idle state

        }
        // During the three seconds that the calibration button is being pressed, the thermometer works as normal
        // like in the idle state
        display.clearDisplay();

        display.drawBitmap(0,0, images[gLockUnlock], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        display.drawBitmap(5,2, images[gUnitToggle], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
        display.drawBitmap(5,2, images[4], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

        display.setTextColor(SSD1306_WHITE);

        accelgyro.getAcceleration(&ax, &ay, &az);
        if (ax < -500)
        {
          display.setRotation(2);
        }
        else
        {
          display.setRotation(0);
        }
      }

      display.setTextSize(1); // Draw 2X-scale test
      display.setCursor(10, 0);
      display.print("Tempurature");

      display.setTextSize(2); // Draw 2X-scale test
      display.setCursor(10, 10);
    
      display.print(gCurrentTemp);

      display.display();
      delay(100);
      break;

  }
  
}

/**********************************************************************
*                           ISR Functions
**********************************************************************/

void onOffToggle() // ISR that will toggle the On/Off Flag
{
gOnOffFlag = !gOnOffFlag;
}

//*********************************************************************

void lockToggle() // ISR that will toggle the lock Flag
{
gLockFlag = !gLockFlag;
}

//*********************************************************************

ISR(TIMER1_COMPA_vect) // Timer 1 ISR. Used to retrieve current temperature from sensor
{
  sensors.requestTemperatures(); // Reads the current temperature every 1 second

  // If ladder that checks what unit is selected and makes the variable that value
  if (gUnitToggle == 2)
  {
    gCurrentTemp = sensors.getTempC(insideThermometer);
  }
  else if (gUnitToggle == 3)
  {
    gCurrentTemp = sensors.getTempF(insideThermometer); 
  }

}

//*********************************************************************

// ISR for Timer 2. Used by the Unit button and the Calibration button to check 
// if either of the buttons has been held for three seconds
ISR(TIMER2_COMPA_vect) 
{
  gTimer2Counter++; // Increment Counter

  if (gTimer2Counter >= TIMER2_COUNTER_MAX) // Checks counter if the timer has reached 3 seconds
  {
    if (digitalRead(UNIT_BUTTON) == HIGH) // Checks if the Unit button is still being pressed
    {
      // Toggles the global variable to select between celcius or fahrenheit
      if (gUnitToggle == 2)
      {
        gUnitToggle = 3;
      }
      else
      {
        gUnitToggle = 2;
      }
    TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20)); // Restart Timer2 with CTC mode and a 256 prescaler

    currentState = IDLE_STATE; // Moves state machine to normal idle state
    }

    if (digitalRead(CAL_BUTTON) == HIGH) // Checks if the calibration button is still being pressed
    {
      gCalFlag = 1; // Moves the cakibration flag to 1
      
      TCCR2B &= ~((1 << CS22) | (1 << CS21) | (1 << CS20)); // Restart Timer2 with CTC mode and a 256 prescaler
    }
    gTimer2Counter = 0; // Resets Counter to Zero
  }
}

/**********************************************************************
*                        External Functions
**********************************************************************/

void checkButtons() // External funciton to check states of push buttons
{
  // Check On/Off State
  if (gOnOffFlag == 1) // If ON/OFF ISR has been triggered...
  {
   currentState = IDLE_STATE; // Move to normal idle state from off state if currently in off state
  }
  else // If not in off state, move to off state
  {
    currentState = OFF_STATE;
    gLockFlag = 1; // Ensures that when the thermometer is turned off and on again, it is not locked
  }

  // Check on Lock State
  if (gLockFlag == 1) // Normal state for the lock (unlocked) allowing for the tempurature to be read
  {
    TCCR1B |= (1<<CS12); // Starts Timer 1
    gLockUnlock = 1; // Changes the variable that tracks what symbol is printed on the OLED
    
  }
  else // This happens when the thermometer is locked
  {
    TCCR1B = 0; //Stops Timer 1 stopping the tempurature from being read
    gLockUnlock = 0; // Chnaged the variable that tracks what symbol is printed on the OLED
  }

  // Check Light Button
  if (digitalRead(LIGHT_BUTTON) == HIGH) // If the Light button is pressed toggle the LED on or off
  {
    gLEDToggle = !gLEDToggle;
    digitalWrite(LED, gLEDToggle);
  }

  // Check Unit Button
  if (digitalRead(UNIT_BUTTON) == HIGH) // If the Unit button is pressed....
  {
    TCCR2B |= (1<<CS21) | (1<<CS22); // Start Timer 2 for the three second press period of the Unit button
    currentState = UNITS_PRESSED_STATE; // Moves the state machine to the state where the units are in the process of changing
  }

  // Check Calibration Button
  if (digitalRead(CAL_BUTTON) == HIGH) //  If the Cal button is pressed...
  {
    TCCR2B |= (1<<CS21) | (1<<CS22); // Start Timer 2 for the three second press period of the Cal button
    currentState = CAL_PRESSED_STATE; // Moves the state machine to the state where the thermometer is in progress of going into calibration mode
  }
}
