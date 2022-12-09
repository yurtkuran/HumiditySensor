// #include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "Adafruit_HTU31D.h"
#include "MovingAverageFloat.h"           // https://github.com/pilotak/MovingAverageFloat
#include "WiFiCredentials.h"


// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Server time variables
unsigned long currentTime = millis();                 // Current time
unsigned long previousTime = 0;                       // Previous time
const long timeoutTime = 2000;                        // Define timeout time in milliseconds (example: 2000ms = 2s)

// ntp settings
const char* ntpServer          = "pool.ntp.org";
const long  utcOffsetInSeconds = -5*3600;
const int   daylightOffset_sec = 3600;

// init NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds);

// init sensor
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

float tempF(float c ) {
  return 1.8*c + 32;
}

String getLocalTime() {
  timeClient.update();
  return timeClient.getFormattedTime();
}

void printSensorValues(float avgTemp, float avgHumidity, float relativeHumidity) {
  Serial.println("Time:        " + getLocalTime());
  Serial.printf("Avg Temp [F]: %.1f\n", tempF(avgTemp)); 
  Serial.printf("Avg Humidity: %.1f\n", avgHumidity); 
  Serial.printf("Humidity:     %.1f\n\n", relativeHumidity); 
}

void readSensor() {
  htu.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  avgHumidity.add(humidity.relative_humidity); 
  avgTemp.add(temp.temperature);
}

void toggleHeater() {
  heaterEnabled = !heaterEnabled;
  if (!htu.enableHeater(heaterEnabled)) {
    Serial.println("Emable heater failed");
  }
  heaterTimestamp = millis();
}

void setup() {
  // init serial port
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // wait untill serial port opens
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to "+(String)ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print local IP address and start web server
  Serial.println("\n\nWiFi connected - IP address: "+WiFi.localIP().toString())+"/n";

  // Launch server
  server.begin();
  
  //init time
  timeClient.begin();

  Serial.println("Adafruit HTU31D\n");

  // init sensor
  if (!htu.begin(0x40)) {
    Serial.println("Couldn't find sensor!");
    while (1);
  }
}

void loop() {
  WiFiClient client = server.available();                                       // Listen for incoming clients

  if (client) {  // If a new client connects,
    // Serial.println("Client - connected");                                       // print a message out in the serial port
    String currentLine = "";                                                    // make a String to hold incoming data from the client

    currentTime = millis();
    previousTime = currentTime;

    while (client.connected() && currentTime - previousTime <= timeoutTime) {   // loop while the client's connected
      currentTime = millis();

      if (client.available()) {                                                 // if there's bytes to read from the client,
        char c = client.read();                                                 // read a byte, then
        // Serial.write(c);                                                        // print it out the serial monitor
        header += c;

        if (c == '\n') {                                                        // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {

            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // process get reqests here
            if (header.indexOf("GET /humidity") >= 0) {
              // readSensor();
              // printSensorValues(avgTemp.get(), avgHumidity.get(), humidity.relative_humidity);
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");

            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>");
            client.println("html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center; }");
            client.println("body { color: rgb(47, 62, 78); }");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer; }");
            client.println(".button2 {background-color: #555555; }");
            client.println("h3 {font-size: 0.7rem; color: #555555; }");
            client.println("</style></head>");

            // HTML body
            client.println("<body><h3>Hello World</h3>");

            if (header.indexOf("GET /humidity") >= 0) {
              readSensor();
              printSensorValues(avgTemp.get(), avgHumidity.get(), humidity.relative_humidity);

              htu.getEvent(&humidity, &temp);                                   // populate temp and humidity objects with fresh data
              client.println("<p>"+getLocalTime()+"</p>");
              client.printf("<p>Temp [F]: %.1f", tempF(temp.temperature),"</p>");
              client.printf("<p>Humidity: %.1f", humidity.relative_humidity,"</p>");
              // client.printf("Humidity:     %.1f\n\n", humidity.relative_humidity);
            }

            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();

            // Break out of the while loop
            break;

          } else {  // if you got a newline, then clear currentLine
            currentLine = "";

          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";

    // Close the connection
    client.stop();
    // Serial.println("Client - disconnected.\n");
  }

}
  
  
  
  
  
  // // read sensor
  // readSensor();

  // // update display
  // if ((millis() - displayTimestamp) > period) {
  //   printSensorValues(avgTemp.get(), avgHumidity.get(), humidity.relative_humidity);
  //   displayTimestamp = millis();
  // }

  // // toggle the heater every 5 seconds
  // if ((millis() - heaterTimestamp) > 5000) {
  //   toggleHeater();
  // }

  // delay(delayMs);
