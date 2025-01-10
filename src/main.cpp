#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>

// Replace with your network credentials
const char* ssid = "Galaxy A21sA57C";
const char* password = "abcd12345";

// Data wire is connected to GPIO4 on the ESP32
#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// WiFi server on port 80
WiFiServer server(80);

void setupWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // Print the ESP32 IP address
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  setupWiFi(ssid, password);

  // Start the DS18B20 sensor
  sensors.begin();

  // Start the web server
  server.begin();

  Serial.println("Server started, waiting for clients...");
}

void loop() {
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

    // Serve temperature as JSON
    if (request.indexOf("/temperature") != -1) {
      sensors.requestTemperatures();
      float tempC = sensors.getTempCByIndex(0);
      String jsonResponse = "{\"temperature\":" + String(tempC) + "}";

      client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
      client.print(jsonResponse);
    } else {
      // Serve the main HTML page
      String html = R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
          <title>Temperature Monitor</title>
          <style>
            body {
              font-family: Arial, sans-serif;
              background-color: #f4f4f9;
              color: #333;
              display: flex;
              justify-content: center;
              align-items: center;
              height: 100vh;
              margin: 0;
            }
            .container {
              text-align: center;
              background: #ffffff;
              padding: 20px;
              border-radius: 10px;
              box-shadow: 0 4px 8px rgba(0, 0, 0, 0.2);
              width: 80%;
              max-width: 400px;
            }
            h1 {
              font-size: 1.8em;
              color: #007BFF;
              margin-bottom: 20px;
            }
            .temp {
              font-size: 2em;
              color: #e63946;
              margin: 10px 0;
            }
            footer {
              margin-top: 20px;
              font-size: 0.8em;
              color: #888;
            }
          </style>
          <script>
            function updateTemperature() {
              fetch('/temperature')
                .then(response => response.json())
                .then(data => {
                  document.getElementById('temperature').textContent = data.temperature + ' °C';
                })
                .catch(error => console.error('Error:', error));
            }
            setInterval(updateTemperature, 1000);
          </script>
        </head>
        <body>
          <div class="container">
            <h1>Real-Time Temperature Sensor</h1>
            <div class="temp" id="temperature">Loading...</div>
            <footer>&copy; 2025 by DEIE</footer>
          </div>
        </body>
        </html>
      )rawliteral";

      client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      client.print(html);
    }

    // Close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}
