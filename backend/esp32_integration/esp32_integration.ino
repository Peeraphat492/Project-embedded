/*
 * ESP32 Integration Example for Smart Room Reservation System
 * 
 * This example shows how to connect ESP32 to the backend API
 * for IoT device control and room automation
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Backend API Configuration
const char* serverURL = "http://YOUR_SERVER_IP:3000/api";  // Replace with your server IP
const char* deviceEndpoint = "/device/settings";
const char* controlEndpoint = "/device/control";

// Pin Configuration
const int relayPin = 2;      // Relay control pin
const int ledPin = LED_BUILTIN;  // Status LED

// Timing Configuration
unsigned long lastCheck = 0;
const unsigned long checkInterval = 30000;  // Check every 30 seconds

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(relayPin, LOW);
  digitalWrite(ledPin, LOW);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Initial status
  digitalWrite(ledPin, HIGH);
  Serial.println("ESP32 Smart Room Controller Ready");
}

void loop() {
  // Check server periodically
  if (millis() - lastCheck >= checkInterval) {
    checkBookingStatus();
    lastCheck = millis();
  }
  
  delay(1000);
}

void checkBookingStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverURL) + String(deviceEndpoint);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server Response: " + response);
      
      // Parse JSON response
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      
      // Get current time and relay status
      String currentTime = doc["currentTime"];
      String wakeTime = doc["wakeTime"];
      String stopTime = doc["stopTime"];
      bool relayStatus = doc["relaystatus"];
      
      Serial.println("Current Time: " + currentTime);
      Serial.println("Wake Time: " + wakeTime);
      Serial.println("Stop Time: " + stopTime);
      Serial.println("Relay Status: " + String(relayStatus));
      
      // Control relay based on time
      controlRelay(currentTime, wakeTime, stopTime);
      
    } else {
      Serial.println("Error connecting to server: " + String(httpResponseCode));
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void controlRelay(String currentTime, String wakeTime, String stopTime) {
  // Simple time comparison (you may want to implement more robust time handling)
  int currentHour = currentTime.substring(0, 2).toInt();
  int currentMinute = currentTime.substring(3, 5).toInt();
  
  int wakeHour = wakeTime.substring(0, 2).toInt();
  int wakeMinute = wakeTime.substring(3, 5).toInt();
  
  int stopHour = stopTime.substring(0, 2).toInt();
  int stopMinute = stopTime.substring(3, 5).toInt();
  
  int currentTotalMinutes = currentHour * 60 + currentMinute;
  int wakeTotalMinutes = wakeHour * 60 + wakeMinute;
  int stopTotalMinutes = stopHour * 60 + stopMinute;
  
  bool shouldActivate = (currentTotalMinutes >= wakeTotalMinutes && currentTotalMinutes < stopTotalMinutes);
  
  if (shouldActivate) {
    digitalWrite(relayPin, HIGH);
    Serial.println("Relay ON - Room Active");
    sendDeviceStatus("turn_on", wakeTime + "-" + stopTime);
  } else {
    digitalWrite(relayPin, LOW);
    Serial.println("Relay OFF - Room Inactive");
    sendDeviceStatus("turn_off", "");
  }
}

void sendDeviceStatus(String action, String timeRange) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverURL) + String(controlEndpoint);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    // Create JSON payload
    DynamicJsonDocument doc(256);
    doc["action"] = action;
    doc["timeRange"] = timeRange;
    doc["deviceId"] = "ESP32_Room_Controller";
    doc["timestamp"] = WiFi.getTime();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    int httpResponseCode = http.POST(jsonString);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Status sent successfully: " + response);
    } else {
      Serial.println("Error sending status: " + String(httpResponseCode));
    }
    
    http.end();
  }
}

// Handle manual control via Serial
void serialEvent() {
  if (Serial.available()) {
    String command = Serial.readString();
    command.trim();
    
    if (command == "ON") {
      digitalWrite(relayPin, HIGH);
      Serial.println("Manual ON");
      sendDeviceStatus("manual_on", "");
    } else if (command == "OFF") {
      digitalWrite(relayPin, LOW);
      Serial.println("Manual OFF");
      sendDeviceStatus("manual_off", "");
    } else if (command == "STATUS") {
      Serial.println("Current Status: " + String(digitalRead(relayPin) ? "ON" : "OFF"));
      Serial.println("WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
      Serial.println("IP: " + WiFi.localIP().toString());
    } else if (command == "RESET") {
      ESP.restart();
    }
  }
}

/*
 * Installation Instructions:
 * 
 * 1. Install required libraries:
 *    - WiFi (built-in)
 *    - HTTPClient (built-in)
 *    - ArduinoJson (install via Library Manager)
 * 
 * 2. Update configuration:
 *    - Set your WiFi credentials
 *    - Set your server IP address
 *    - Adjust pin numbers if needed
 * 
 * 3. Hardware connections:
 *    - Connect relay module to pin 2
 *    - Connect status LED if needed
 * 
 * 4. Upload to ESP32
 * 
 * 5. Monitor serial output for debugging
 * 
 * Serial Commands:
 *    - "ON" - Turn relay on manually
 *    - "OFF" - Turn relay off manually
 *    - "STATUS" - Show current status
 *    - "RESET" - Restart ESP32
 */