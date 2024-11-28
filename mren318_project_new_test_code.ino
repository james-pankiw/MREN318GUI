#include <WiFiS3.h>
#include <ArduCAM.h>
#include <Wire.h>
#include <SPI.h>
#include "memorysaver.h"
#include <TimeLib.h>
#include <Arduino.h>


// WiFi credentials
const char ssid[] = "Desmos";          // Your WiFi SSID
const char pass[] = "papavasiliou"; // Your WiFi password

// Pins and ArduCAM configuration
#define CS_PIN 7 // Chip Select pin for ArduCAM
ArduCAM myCAM(OV2640, CS_PIN);

WiFiServer server(80);

// Variables to store received data from the Web UI
int receivedValue = 0; // Variable to store received integer from the web UI
int mass = 100;   // in grams, default 100.
int feeding_frequency = 3;    // How many times per day do you want your pet to eat? Default = 3
int feeding_interval = 8;    // Time between each feed, default 8
time_t first_feed = now();
time_t current_time = now();
time_t next_feed_time;
//int food_interval = 0;

// Stepper Motor / LED + button
#define stepPin 2  // Pin connected to the STEP pin of the driver
#define dirPin 3   // Pin connected to the DIR pin of the driver
#define button 4
#define LED 5
const int stepsPerRev = 200;

// US Sensor
#define US_TRIGPIN 9
#define US_ECHOPIN 8
long duration;
int distance;

// FlexiForce Sensor
// http://adam-meyer.com/arduino/Flexiforce
#define FLEXIFORCE_PIN A0
float initial_mass = 0.0;
float current_mass = 0.0;

// Other 
bool testMode = true;   // True only for the demo to show test, false otherwise.


void setup() {
  Serial.begin(115200);
  pinMode(CS_PIN, OUTPUT);

  // Initialize the camera
  Wire.begin();
  SPI.begin();
  myCAM.write_reg(0x07, 0x80); // Reset the camera
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  // Initialize WiFi connection
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  // Stepper Motor / LED + Button
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(dirPin, HIGH);   // LOW = clockwise, HIGH = counterclockwise
  digitalWrite(LED, LOW);   // Initialize LED to be off.

  // US Sensor
  pinMode(US_TRIGPIN, OUTPUT);
  pinMode(US_ECHOPIN, INPUT);

  // FlexiForce Sensor
  initial_mass = readMassBowl();
  current_mass = initial_mass;


}


void loop() {
  // Listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\n');
    if (request.indexOf("POST") >= 0) {
      handlePostRequest(client, request);
    } else if (request.indexOf("GET /capture") >= 0) {
      captureImage(client);
    } else {
      sendWebUI(client);
    }
    client.stop();
  }

  static uint8_t lastBtnState = LOW;
  uint8_t state = digitalRead(button);
  if (state != lastBtnState) {
    lastBtnState = state;
  }
  delay(100);
  if (state == HIGH || next_feed_time == now()) {
    if (testMode == true) {
      demo();
    } else if (testMode == false) {
      feed();
    }
  } 
  delay(100);

  // if eating time is now or button is pressed
    // if (testMode == true)
      // demo();
    // else if (testMode == false)
      // feed();

/*
  food_interval++;
  if (food_interval >= feeding_frequency) {
    food_interval = 0;
  }*/
  

}


void handlePostRequest(WiFiClient &client, String &request) {
  if (request.indexOf("mass=") >= 0) {
    int value = extractValue(request, "mass=");
    if (value >= 10 && value <= 250) {
      mass = value;
      Serial.print("Updated mass: ");
      Serial.println(mass);
    }
  }

  if (request.indexOf("feeding_frequency=") >= 0) {
    int value = extractValue(request, "feeding_frequency=");
    if (value > 0 && value <= 5) {
      feeding_frequency = value;
      Serial.print("Updated feeding frequency: ");
      Serial.println(feeding_frequency);
    }
  }

  if (request.indexOf("feeding_interval=") >= 0) {
    int value = extractValue(request, "feeding_interval=");
    if (value > 0 && value <= 12) {
      feeding_interval = value;
      Serial.print("Updated feeding interval: ");
      Serial.println(feeding_interval);
    }
  }

  if (request.indexOf("first_feed=") >= 0) {
    String timeStr = extractStringValue(request, "first_feed=");
    time_t newFeedTime = parseTime(timeStr);
    if (newFeedTime >= now()) {
      first_feed = newFeedTime;
      Serial.print("Updated first feed time: ");
      Serial.println(first_feed);
    }
  }

  // Recalculate next feeding time
  calculateNextFeedingTime();

  // Respond to the client
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("Update successful");
}


void captureImage(WiFiClient &client) {
  myCAM.clear_fifo_flag();
  myCAM.start_capture();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  uint32_t length = myCAM.read_fifo_length();
  if (length >= MAX_FIFO_SIZE || length == 0) {
    Serial.println("Image capture failed");
    return;
  }

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: image/jpeg");
  client.println("Connection: close");
  client.println();
  while (length--) {
    uint8_t byte = SPI.transfer(0x00);
    client.write(byte);
  }
  myCAM.CS_HIGH();
}


void sendWebUI(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head><title>Feeding System</title></head>");
  client.println("<body>");
  client.println("<h1>Feeding System Configuration</h1>");
  client.println("<form method='POST'>");

  // Mass
  client.println("<label for='mass'>Mass (10-250):</label>");
  client.print("<input type='number' id='mass' name='mass' value='");
  client.print(mass);
  client.println("' min='10' max='250'><br><br>");

  // Feeding Frequency
  client.println("<label for='feeding_frequency'>Feeding Frequency (1-5):</label>");
  client.print("<input type='number' id='feeding_frequency' name='feeding_frequency' value='");
  client.print(feeding_frequency);
  client.println("' min='1' max='5'><br><br>");

  // Feeding Interval
  client.println("<label for='feeding_interval'>Feeding Interval (1-12 hours):</label>");
  client.print("<input type='number' id='feeding_interval' name='feeding_interval' value='");
  client.print(feeding_interval);
  client.println("' min='1' max='12'><br><br>");

  // First Feed
  client.println("<label for='first_feed'>First Feed Time (yyyy-mm-dd hh:mm):</label>");
  client.println("<input type='text' id='first_feed' name='first_feed' placeholder='YYYY-MM-DD HH:MM'><br><br>");

  client.println("<input type='submit' value='Update'>");
  client.println("</form>");
  client.println("</body>");
  client.println("</html>");
}


int extractValue(String &request, const char *key) {
  int index = request.indexOf(key);
  if (index >= 0) {
    int endIndex = request.indexOf('&', index);
    if (endIndex == -1) endIndex = request.length();
    return request.substring(index + strlen(key), endIndex).toInt();
  }
  return -1;
}


String extractStringValue(String &request, const char *key) {
  int index = request.indexOf(key);
  if (index >= 0) {
    int endIndex = request.indexOf('&', index);
    if (endIndex == -1) endIndex = request.length();
    return request.substring(index + strlen(key), endIndex);
  }
  return "";
}


time_t parseTime(String &timeStr) {
  int year = timeStr.substring(0, 4).toInt();
  int month = timeStr.substring(5, 7).toInt();
  int day = timeStr.substring(8, 10).toInt();
  int hour = timeStr.substring(11, 13).toInt();
  int minute = timeStr.substring(14, 16).toInt();

  tmElements_t tm;
  tm.Year = year - 1970;
  tm.Month = month;
  tm.Day = day;
  tm.Hour = hour;
  tm.Minute = minute;
  tm.Second = 0;

  return makeTime(tm);
}


void calculateNextFeedingTime() {
  previous_feed_time = now();
  next_feed_time = previous_feed_time + (feeding_interval * SECS_PER_HOUR);
  Serial.print("Previous feed time: ");
  Serial.println(previous_feed_time);
  Serial.print("Next feed time: ");
  Serial.println(next_feed_time);
}


float grams2rotations(int grams) {
  float revs = grams / 25;    // ~25 grams per 1 complete revolution of the motor.
  return revs;
} // End of 'grams2rotations' method.


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


void motorControl(int grams) {
  float rev_ratio = grams2rotations(grams)
  int stepCount = (int) (rev_ratio * stepsPerRev);
  for (int x = 0; x < stepCount; x++) {
    digitalWrite(stepPin, HIGH);
    delay(5);
  }
  current_step = stepCount % stepsPerRev;
  digitalWrite(stepPin, LOW);
  delay(1000);
} // End of 'motorControl' method.


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


float readMassBowl() {
  int flexiForceReading = analog(FLEXIFORCE_PIN);
  float m = ((float)(flexiForceReading)) / 9.81;
  return m;
} // End of 'readMassBowl' method.


void demo() {
  simulateMotor(mass);
} // End of 'demo' method.


void feed() {
  motorControl(mass);
} // End of 'feed' method.




