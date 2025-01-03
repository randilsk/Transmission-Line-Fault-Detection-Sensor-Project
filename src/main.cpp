// #include <Arduino.h>
// #include "ota_setup.h"

// // Replace with your network credentials
// const char* ssid = "Galaxy A21sA57C";  // Replace with your mobile hotspot SSID
// const char* password = "abcd12345";  // Replace with your mobile hotspot password

// void setup() {
//   Serial.begin(115200);  // Start serial communication at 115200 baud

//   // Connect to Wi-Fi
//   setupWiFi(ssid, password);  // Call the function to connect to Wi-Fi

//   // Set up OTA functionality
//   setupOTA();  // Initialize OTA handling
// }

// void loop() {
//   ArduinoOTA.handle();  // Handle OTA requests in the loop
// }

// version 1

// #include <Arduino.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>
// #include <WiFi.h>
// #include <ArduinoOTA.h>
// #include "ota_setup.h"  // Assuming ota_setup.h contains WiFi and OTA functions

// // Replace with your network credentials
// const char* ssid = "Galaxy A21sA57C";  // Replace with your mobile hotspot SSID
// const char* password = "abcd12345";    // Replace with your mobile hotspot password

// // Data wire is connected to GPIO4 on the ESP32
// #define ONE_WIRE_BUS 4

// // Setup a oneWire instance to communicate with any OneWire device
// OneWire oneWire(ONE_WIRE_BUS);

// // Pass oneWire reference to DallasTemperature library
// DallasTemperature sensors(&oneWire);

// void setup() {
//   Serial.begin(115200);  // Start serial communication at 115200 baud
  
//   // Connect to Wi-Fi
//   setupWiFi(ssid, password);  // Connect to Wi-Fi using function in ota_setup.h

//   // Set up OTA functionality
//   setupOTA();  // Initialize OTA handling
  
//   // Start the DS18B20 sensor
//   sensors.begin();
// }

// void loop() {
//   // Handle OTA requests
//   ArduinoOTA.handle();  

//   // Read temperature and print it to the Serial Monitor
//   sensors.requestTemperatures();  // Send the command to get temperatures
//   float tempC = sensors.getTempCByIndex(0);  // Get the temperature in Celsius

//   if (tempC != DEVICE_DISCONNECTED_C) {
//     Serial.print("Temperature: ");
//     Serial.print(tempC);
//     Serial.println(" °C");
//   } else {
//     Serial.println("Error: Sensor not connected or not found!");
//   }

//   delay(1000);  // Wait for 1 second before next reading
// }

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "ota_setup.h"  // Ensure your OTA setup file is included

// Replace with your network credentials
const char* ssid = "Galaxy A21sA57C";
const char* password = "abcd12345";

// Data wire is connected to GPIO4 on the ESP32
#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// WiFi server on port 80
WiFiServer server(80);

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  setupWiFi(ssid, password);

  // Start the DS18B20 sensor
  sensors.begin();

  // Start the web server
  server.begin();

  // Set up OTA
  setupOTA();

  Serial.println("Server started, waiting for clients...");
}

void loop() {
  // Handle OTA updates
  ArduinoOTA.handle();

  // Check for client connection
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");

    // Wait for the client's request
    while (client.connected() && !client.available()) {
      delay(1);
    }

    // Read the request
    String request = client.readStringUntil('\r');
    client.flush();

    // Read temperature
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);

    // Generate HTML response
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Temperature Monitor</title>
        <meta http-equiv="refresh" content="1">
        <style>
          body { font-family: Arial; text-align: center; }
          .temp { font-size: 2em; color: #007BFF; }
        </style>
      </head>
      <body>
        <h1>Real-Time Temperature Sensor</h1>
        <div class="temp">Temperature: )rawliteral";
    html += String(tempC) + " &deg;C</div>";
    html += R"rawliteral(
      </body>
      </html>
    )rawliteral";

    // Send response
    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    client.print(html);

    // Close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}
