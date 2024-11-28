// MREN 318 - Project
// Automated Pet Feeder
// Group 4
// Author: Dimos Papavasiliou



/********** IMPORTED LIBRARIES **********/
#include <ArduCAM.h>
#include <Wire.h>
#include <TimeLib.h>    // https://docs.arduino.cc/libraries/time/
#include <Arduino.h>
#include <WiFiS3.h>



/********** PIN ASSIGNMENTS **********/
// Wifi & Website
// N/A

// OV2640 ArduCAM
#define CS 7

// Stepper Motor
#define stepPin 2  // Pin connected to the STEP pin of the driver
#define dirPin 3   // Pin connected to the DIR pin of the driver

// FlexiForce Sensor
// http://adam-meyer.com/arduino/Flexiforce
#define FLEXIFORCE_PIN A0

// US / IR Sensor
#define US_TRIGPIN 9
#define US_ECHOPIN 8

// Other
#define button 4
#define LED 5



/********** VARIABLES **********/
// Wifi & Website
// https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples/
// https://arduinogetstarted.com/tutorials/arduino-uno-r4-wifi-controls-led-via-web
#define SSID "Desmos"
#define PASSWORD "papavasiliou"


// OV2640 ArduCAM
#define OV2640_MINI_2MP_PLUS
ArduCAM myCam(OV2640, CS);
const int chunkSize = 512;

// Stepper Motor
const int stepsPerRev = 200;

// FlexiForce Sensor
float initial_mass = 0.0;
float current_mass = 0.0;

// US / IR Sensor
long duration;
int distance;

// Other
bool testMode = true;     // True only for the demo to show test, false otherwise.

// Web UI Variables:
int mass;   // in grams, default 100.
bool schedule_mode;   // 0 for time interval, 1 for set feeding time. default 0.
//time_t set_feed[3];
int feeding_frequency;    // How many times per day do you want your pet to eat?
time_t first_feed_of_day; // What time would you like the first feed to be?







/********** SETUP **********/
void setup() {
  Serial.begin(115200);
  Wire.begin();
  SPI.begin();


  /* ===== SET PIN MODES ===== */
  // Wifi & Website
  // ...


  // OV2640 ArduCAM
  // ...


  // Stepper Motor
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  // FlexiForce Sensor
  // N/A

  // US / IR Sensor
  pinMode(US_TRIGPIN, OUTPUT);
  pinMode(US_ECHOPIN, INPUT);
  
  // Other
  pinMode(button, INPUT_PULLUP);
  pinMode(LED, OUTPUT);


  /* ===== ADDITIONAL SETUP ===== */
  // Wifi & Website
  // ...


  // OV2640 ArduCAM
  // ...


  // Stepper Motor
  digitalWrite(dirPin, HIGH);   // LOW = clockwise, HIGH = counterclockwise

  // FlexiForce Sensor
  current_mass = readMassBowl();
  initial_mass = current_mass;

  // US / IR Sensor
  // N/A

  // Other
  digitalWrite(LED, LOW);   // Initialize LED to be off.

}






/********** LOOP **********/
void loop() {
  food_interval = 0;

  int flexiForceReading = analog(FLEXIFORCE_PIN);

  // Check for website response (James) (if user wants input any changes to the default settings)


  // Look at time right now to see if 



  // FEED

  // If correct time (whether by interval or by date-time) then feed
    // if (interval != 0) then set_time = 0
      // schedule_mode(int mode)    0 for interval, 1 for set_time
  // stop when 
  // Take photo of animal
  // Send final mass to website


  if (testMode == true) {
    demo();
  } else if (testMode == false)) {
    feed();
  }




  if (food_interval >= feeding_frequency) {
    food_interval = 0;
  }
  
  delay(250);   // slows down the output for easier readings

}






/********** ADDITIONAL METHODS **********/

// Wifi & Website
// ...



// OV2640 ArduCAM
// ...


// Stepper Motor
float grams2rotations(int grams) {
  float revs = grams / 25;    // ~25 grams per 1 complete revolution of the motor.
  return revs;
} // End of 'grams2rotations' method.

void motorControl(float rev_ratio) {
  int stepCount = (int) (rev_ratio * stepsPerRev);
  for (int x = 0; x < stepCount; x++) {
    digitalWrite(stepPin, HIGH);
    delay(5);
  }
  current_step = stepCount % stepsPerRev;
  digitalWrite(stepPin, LOW);
  delay(1000);
} // End of 'motorControl' method.

void feedPet(int grams) {
  motorControl(grams2rotations(grams));
} // End of 'feedPet' method.

void mimicMotorControl() {
  // Made to represent a visual representation of the motor being "on" if the motor was working.
  // Uses an LED as a visual indicator to replace the motor for the purposes of the demo.

} // ...

// Weight / Force Sensor
float readMassBowl() {
  int flexiForceReading = analog(FLEXIFORCE_PIN);
  float m = ((float)(flexiForceReading)) / 9.81;
  return m;
} // End of 'readMassBowl' method.

// US / IR Sensor
void readUS() {
  // https://howtomechatronics.com/tutorials/arduino/ultrasonic-sensor-hc-sr04/
  digitalWrite(US_TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(US_TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(US_TRIGPIN, LOW);
  duration = pulseIn(US_ECHOPIN, HIGH);
  distance = duration * 0.034 / 2;
} // End of 'readUS' method.



// Other
void simulateMotor(int grams) {
  // Converts grams of food per feed to number of seconds of runtime for the motor / LED indicator
  // From 'motorControl' method, we can deduce that the motor will run for 5 millisecond per step.
  // There are 200 steps in one full revolution
  float revs = grams2rotations();
  int led_time = ((int) (revs * stepsPerRev)) * 5;
  digitalWrite(LED, HIGH);
  delay(led_time);
  digitalWrite(LED, LOW);
  delay(1000);
} // End of 'simulateMotor' method.


void demo() {
  // wait for button to be pressed
  // if pressed, then run demo
  
  while (digitalRead(button) == HIGH) {
    // Do nothing, i.e., wait for button state to change
  }
  delay(20);

  // Turn on LED for certain number of seconds
  simulateMotor(mass);


} // End of 'demo' method.



// Web UI
int readGrams() {
  // read the mass value from the website.
  // James
} // End of 'readGrams' method.

int readFeedFreq() {
  // read the feeding frequency value from the website.
  // i.e., how many times a day do you want to feed your pet?
  // James
}

time_t readFirstFeed() {
  // read the first time in the day that a feed can occur.
  // Uses this info and the feeding frequency to determine a feeding schedule
  // James
}

int read



int mass;   // in grams, default 100.
bool schedule_mode;   // 0 for time interval, 1 for set feeding time. default 0.
//time_t set_feed[3];
int feeding_frequency;    // How many times per day do you want your pet to eat?
time_t first_feed_of_day; // What time would you like the first feed to be?

time_t time_since_last_feed = now();
time_t current_time;




/*
UI:
  - Number of feeds per day (feeding_frequency) int
    - must be 0 < feeding_frequency <= 5

  - First feed time (first_feed_of_day) time_t

  - Grams per feed (mass) int
  
  - Add recommendation note under where user inputs these values
    - "Note: The avergae healthy household pet should eat 3 times a day."


*/



/********** END OF CODE **********/