// #include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "Adafruit_HTU31D.h"
#include "MovingAverageFloat.h"           // https://github.com/pilotak/MovingAverageFloat

// wifi credentials
const char* ssid               = "WirelessAcceleration";
const char* password           = "nr6mLtenanpugXuN";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// ntp settings
const char* ntpServer          = "pool.ntp.org";
const long  utcOffsetInSeconds = -5*3600;
const int   daylightOffset_sec = 3600;

// init NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds);

Adafruit_HTU31D htu = Adafruit_HTU31D();
uint32_t heaterTimestamp;
uint32_t displayTimestamp;
bool heaterEnabled = false;
sensors_event_t humidity, temp;

// setup moving averages
const long points   = 20;
const long delayMs  = 250; // milliseconds
const long period = points*delayMs;
MovingAverageFloat <points> avgHumidity;
MovingAverageFloat <points> avgTemp;

const bool loopSensorReading = true;

float tempF(float c ) {
  return 1.8*c + 32;
}

void printLocalTime() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
}

void printSensorValues(float avgTemp, float avgHumidity, float relativeHumidity) {
  printLocalTime();
  printf("Avg Temp [F]: %.1f\n", tempF(avgTemp)); 
  printf("Avg Humidity: %.1f\n", avgHumidity); 
  printf("Humidity:     %.1f\n\n", relativeHumidity); 
}

void readSensor() {
  htu.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  avgHumidity.add(humidity.relative_humidity); 
  avgTemp.add(temp.temperature);
}

void setup() {
  // init serial port
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // wait untill serial port opens
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.print("\n\nWiFi connected - IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  
  //init time
  timeClient.begin();


  Serial.println("Adafruit HTU31D test\n");

  // init sensor
  if (!htu.begin(0x40)) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
  heaterTimestamp  = millis();
  displayTimestamp = millis();

  Serial.printf("Period between sensor measurements: %.1f seconds\n\n", (float)period/1000);
}

void toggleHeater() {
  heaterEnabled = !heaterEnabled;
  if (!htu.enableHeater(heaterEnabled)) {
    Serial.println("Emable heater failed");
  }
  heaterTimestamp = millis();
}

void loop() {

  // read sensor
  readSensor();

  // update display
  if ((millis() - displayTimestamp) > period) {
    printSensorValues(avgTemp.get(), avgHumidity.get(), humidity.relative_humidity);
    displayTimestamp = millis();
  }

  // toggle the heater every 5 seconds
  if ((millis() - heaterTimestamp) > 5000) {
    toggleHeater();
  }

  delay(delayMs);
}