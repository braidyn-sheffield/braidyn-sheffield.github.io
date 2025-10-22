#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DallasTemperature.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "images.h"

#define CAL_BUTTON 5
#define ON_OFF_BUTTON 2
#define ONE_WIRE_BUS 8
#define TEMP_ANALOG A0
#define UNIT_BUTTON 4
#define LOCK_BUTTON 3
#define LIGHT_BUTTON 6
#define LED 9

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
// arrays to hold device address
DeviceAddress insideThermometer;

volatile int gOnOffFlag = 0; // Stores state of on off button

void setup() 
{
  Serial.begin(9600);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(500); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  pinMode(CAL_BUTTON, INPUT);
  pinMode(ON_OFF_BUTTON, INPUT);
  pinMode(TEMP_ANALOG, INPUT);
  pinMode(UNIT_BUTTON, INPUT);
  pinMode(LOCK_BUTTON, INPUT);
  pinMode(LIGHT_BUTTON, INPUT);
  pinMode(LED, OUTPUT);

  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) 
  {
    sensors.setResolution(insideThermometer, 9);
  } 

  attachInterrupt(digitalPinToInterrupt(ON_OFF_BUTTON), onOffToggle, FALLING);
  attachInterrupt(digitalPinToInterrupt(LOCK_BUTTON), onOffToggle, FALLING);

}

void loop() 
{
  if (gOnOffFlag == 1)
  {

    display.clearDisplay();

    display.drawBitmap(0,0, images[1], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
    display.drawBitmap(5,2, images[2], SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

    display.setTextColor(SSD1306_WHITE);
    display.setRotation(2);

    display.setTextSize(1); // Draw 2X-scale test
    display.setCursor(10, 0);
    display.print("Tempurature");

    display.setTextSize(2); // Draw 2X-scale test
    display.setCursor(10, 10);
    sensors.requestTemperatures();
    
    display.print(sensors.getTempC(insideThermometer));

    display.display();
    delay(100);
  }
  else
  {
    display.clearDisplay();  // Clear the display buffer
    display.display();       // Update the display to show the cleared buffer
    delay(100);
  }

}

void onOffToggle() // ISR that will toggle the On/Off Flag
{
gOnOffFlag = !gOnOffFlag;
}

void LockISR() // ISR that will toggle the On/Off Flag
{
gOnOffFlag = !gOnOffFlag;
}

