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


#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include "ota_setup.h"  // Assuming ota_setup.h contains WiFi and OTA functions

// Replace with your network credentials
const char* ssid = "Galaxy A21sA57C";  // Replace with your mobile hotspot SSID
const char* password = "abcd12345";    // Replace with your mobile hotspot password

// Data wire is connected to GPIO4 on the ESP32
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWire(ONE_WIRE_BUS);

// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);  // Start serial communication at 115200 baud
  
  // Connect to Wi-Fi
  setupWiFi(ssid, password);  // Connect to Wi-Fi using function in ota_setup.h

  // Set up OTA functionality
  setupOTA();  // Initialize OTA handling
  
  // Start the DS18B20 sensor
  sensors.begin();
}

void loop() {
  // Handle OTA requests
  ArduinoOTA.handle();  

  // Read temperature and print it to the Serial Monitor
  sensors.requestTemperatures();  // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0);  // Get the temperature in Celsius

  if (tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println(" °C");
  } else {
    Serial.println("Error: Sensor not connected or not found!");
  }

  delay(1000);  // Wait for 1 second before next reading
}

