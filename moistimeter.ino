#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>

#define SSID ""
#define PASSWORD ""
#define SERVER_URL "" 

ESP8266WebServer server(80);
unsigned long interval = 3000;
unsigned long previousMillis = 0;

WiFiClient wifiClient;

void setup() {
  Serial.begin(9600);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Start the mDNS responder
  if (MDNS.begin("moisture-sensor")) {  
    Serial.println("MDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder");
  }

  // Set up web server routes and start the server
  server.on("/", []() {
    server.send(200, "text/plain; charset=utf-8", "I am the Moisture Sensor 🪴💦");
  });

  server.on("/interval/", HTTP_POST, handleIntervalPost);
  server.begin();
}

int sensorValue;
float moisture;

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    sendSensorData();
  }
}

void sendSensorData() {
  sensorValue = analogRead(A0);
  delay(100);                                                    // Example sensor reading
  moisture = 100.00 - ((sensorValue / 1023.00) * 100.00);  // Convert to percentage (0-100)
  delay(100);
  // Create JSON object
  StaticJsonDocument<200>
    doc;
  JsonObject sensorData = doc.createNestedObject("sensorData");
  sensorData["sensor"] = "Moisture";
  sensorData["value"] = moisture;
  sensorData["unit"] = "Percentage";

  JsonObject battery = doc.createNestedObject("battery");
  battery["voltage"] = 3.7;  // Example value
  battery["level"] = 85;     // Example percentage

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  // Print the message to the serial monitor
  Serial.println("Sending data to server:");
  Serial.println(jsonBuffer);

  // Send data to server
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(wifiClient, SERVER_URL);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonBuffer);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Error in WiFi connection");
  }
}

void handleIntervalPost() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    Serial.println("Received body: " + body);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
      server.send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
      return;
    }
    const char* intervalStr = doc["interval"];
    setInterval(intervalStr);
    server.send(200, "application/json", "{\"message\":\"Interval set\"}");
  } else {
    server.send(400, "application/json", "{\"message\":\"Body not received\"}");
  }
}

void setInterval(const char* intervalStr) {
  if (strcmp(intervalStr, "low") == 0) {
    interval = 3000;  // 3 seconds
  } else if (strcmp(intervalStr, "medium") == 0) {
    interval = 5000;  // 5 seconds
  } else if (strcmp(intervalStr, "high") == 0) {
    interval = 8000;  // 8 seconds
  } else {
    Serial.println("Invalid interval value");
    return;
  }
  Serial.print("Interval set to: ");
  Serial.println(interval);
}
