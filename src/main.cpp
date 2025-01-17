#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// Wi-Fi credentials
const char* ssid = "iPhone";
const char* password = "00022222334";

// DS18B20 sensor setup
#define ONE_WIRE_BUS 4  // Pin where the DS18B20 is connected
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Voltage sensor setup
#define ANALOG_IN_PIN  32           // ESP32 pin GPIO32 (ADC1_CHANNEL_4) connected to voltage sensor
#define REF_VOLTAGE    3.3
#define ADC_RESOLUTION 4096.0
#define R1             30000.0 // Resistor values in voltage sensor (in ohms)
#define R2             7500.0  // Resistor values in voltage sensor (in ohms)
#define NUM_SAMPLES    10

// Current sensor setup
const int currentPin = 34;  // Pin connected to ACS712 sensor
const int numSamples = 500;  // Number of samples to take for both offset calibration and RMS calculation
float offsetVoltage = 2.5;    // Initial guess for no-current offset (for 5V VCC)
float sensitivity = 0.1;      // Sensitivity in V/A (100mV/A for the 20A version)
float voltageSum = 0.0;       // For offset calibration

WiFiServer server(80);  // Web server on port 80

// Variables for averaging voltage readings
float voltageBuffer[NUM_SAMPLES] = {0.0};
int bufferIndex = 0;

// Flame sensor and buzzer setup
#define FLAME_SENSOR_PIN 27 // Flame sensor pin (GPIO 27)
#define BUZZER_PIN 19       // Buzzer pin (GPIO 19)

// Function to calculate the average voltage
float getAverageVoltage() {
  float sum = 0.0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    sum += voltageBuffer[i];
  }
  return sum / NUM_SAMPLES;
}

// Function to setup Wi-Fi
void setupWiFi(const char* ssid, const char* password) {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // Print the ESP32 IP address
}

void setup() {
  Serial.begin(115200);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Connect to Wi-Fi
  setupWiFi(ssid, password);

  // Start DS18B20 sensor
  sensors.begin();

  // Configure ADC pin
  pinMode(ANALOG_IN_PIN, INPUT);
  analogSetAttenuation(ADC_11db);  // Set attenuation for ADC input

  // Initialize voltage buffer
  for (int i = 0; i < NUM_SAMPLES; i++) {
    voltageBuffer[i] = 0.0;
  }

  // Initialize Flame sensor and Buzzer pins
  pinMode(FLAME_SENSOR_PIN, INPUT);  // Flame sensor pin as input
  pinMode(BUZZER_PIN, OUTPUT);       // Buzzer pin as output

  // Start the web server
  server.begin();
  Serial.println("Server started, waiting for clients...");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client connected");

    // Wait for the client's request
    while (client.connected() && !client.available()) {
      delay(1);
    }

    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("/temperature") != -1) {
      // Serve JSON temperature data
      sensors.requestTemperatures();
      float tempC = sensors.getTempCByIndex(0);
      String jsonResponse = "{\"temperature\":" + String(tempC) + "}";
      
      // Add CORS headers
      client.print("HTTP/1.1 200 OK\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print("Access-Control-Allow-Origin: *\r\n");
      client.print("\r\n");
      client.print(jsonResponse);
    } else if (request.indexOf("/voltage") != -1) {
      // Read ADC value and calculate voltage
      int adc_value = analogRead(ANALOG_IN_PIN);
      delay(1000);
      float voltage_adc = ((float)adc_value * REF_VOLTAGE) / ADC_RESOLUTION;
      float voltage_in = voltage_adc * (R1 + R2) / R2;

      // Add voltage to buffer for averaging
      voltageBuffer[bufferIndex] = voltage_in;
      bufferIndex = (bufferIndex + 1) % NUM_SAMPLES;

      // Get the average voltage
      float averageVoltage = getAverageVoltage();

      String jsonResponse = "{\"voltage\":" + String(voltage_in, 2) + "}";

      // Add CORS headers
      client.print("HTTP/1.1 200 OK\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print("Access-Control-Allow-Origin: *\r\n");
      client.print("\r\n");
      client.print(jsonResponse);
    } else if (request.indexOf("/current") != -1) {
      // Offset Calibration
      voltageSum = 0.0;
      for (int i = 0; i < numSamples; i++) {
        int sensorValue = analogRead(currentPin);
        float voltage = sensorValue * (5.0 / 8000.0); // Convert ADC value to voltage
        voltageSum += voltage;
        delay(1);
      }
      offsetVoltage = voltageSum / numSamples;  // Calculate the average offset voltage

      // RMS Current Calculation
      float sumSquared = 0.0;
      for (int i = 0; i < numSamples; i++) {
        int sensorValue = analogRead(currentPin);
        float voltage = sensorValue * (5.0 / 8000.0);  // Convert ADC value to voltage
        float voltageDifference = voltage - offsetVoltage; // Difference from offset
        sumSquared += voltageDifference * voltageDifference;  // Sum of squares
        delay(1);
      }

      float rmsVoltage = sqrt(sumSquared / numSamples);
      float rmsCurrent = rmsVoltage / sensitivity;

      String jsonResponse = "{\"current\":" + String(rmsCurrent, 2) + "}";

      // Add CORS headers
      client.print("HTTP/1.1 200 OK\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print("Access-Control-Allow-Origin: *\r\n");
      client.print("\r\n");
      client.print(jsonResponse);
    } else if (request.indexOf("/flame") != -1) {
      // Get flame sensor status
      int flameStatus = digitalRead(FLAME_SENSOR_PIN);
      if (flameStatus == HIGH) {
    digitalWrite(BUZZER_PIN, HIGH);  // Turn on the buzzer when flame is detected
  } else {
    digitalWrite(BUZZER_PIN, LOW);   // Turn off the buzzer when no flame is detected
  }

      String jsonResponse = "{\"flame\":" + String(flameStatus) + "}";

      // Add CORS headers
      client.print("HTTP/1.1 200 OK\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print("Access-Control-Allow-Origin: *\r\n");
      client.print("\r\n");
      client.print(jsonResponse);
    } else {
      // Serve HTML page
      File file = SPIFFS.open("/index.html", "r");
      if (!file) {
        client.print("HTTP/1.1 500 Internal Server Error\r\n\r\n");
        Serial.println("Failed to open index.html");
      } else {
        client.print("HTTP/1.1 200 OK\r\n");
        client.print("Content-Type: text/html\r\n");
        client.print("Access-Control-Allow-Origin: *\r\n");
        client.print("\r\n");
        while (file.available()) {
          client.write(file.read());
        }
        file.close();
      }
    }

    // Close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}
