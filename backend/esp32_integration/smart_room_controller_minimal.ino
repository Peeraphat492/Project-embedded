/*
  Smart Room Reservation - Arduino Controller (Minimal Version)
  ESP32 WiFi-enabled room access control system
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// WiFi Configuration
const char* ssid = "Peeraphat9";
const char* password = "12345678";

// Server Configuration
const char* serverURL = "http://172.20.10.2:3000";
int roomId = 0;

// Hardware Pins
#define PIR_SENSOR 4      // HC-SR501 PIR Motion Sensor
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define LED1 19  // Status LED (Blue)
#define LED2 18  // LED (Red)
#define LED3 16  // LED (Green)
#define LED4 13  // LED (Yellow)
#define RELAY_PIN 23  // Door lock relay

// HC-SR501 Configuration Variables
bool pirCalibrated = false;
unsigned long pirCalibrationStart = 0;
const unsigned long PIR_CALIBRATION_TIME = 60000;  // 60 seconds calibration

// Initialize OLED
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Keypad Configuration
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {14, 32, 27, 26};
byte colPins[COLS] = {12, 25, 33};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Global Variables
String accessCode = "";
bool isBookingActive = false;
bool userHasAccessed = false;
bool doorUnlocked = false;
bool motionDetected = false;
bool ledsOn = false;
String currentBookingId = "";

// Timing Variables
unsigned long lastStatusCheck = 0;
unsigned long lastMotionCheck = 0;
unsigned long unlockTime = 0;
unsigned long lastMotionTime = 0;
unsigned long currentCheckInterval = 15000;  // Start with 15 seconds
int rateLimitCount = 0;  // Count rate limit occurrences

// Constants
const unsigned long STATUS_CHECK_INTERVAL = 15000;  // Check every 15 seconds (increased)
const unsigned long STATUS_CHECK_INTERVAL_SLOW = 60000;  // Slow down to 60 seconds when rate limited
const unsigned long MOTION_CHECK_INTERVAL = 500;   // Check motion every 500ms
const unsigned long UNLOCK_DURATION = 10000;       // Door unlock duration
const unsigned long MOTION_TIMEOUT = 15000;        // LED timeout after no motion (15 seconds)

// HC-SR501 specific constants
const unsigned long HC_SR501_TRIGGER_TIME = 3000;   // HC-SR501 minimum trigger time
const unsigned long HC_SR501_BLOCK_TIME = 3000;     // HC-SR501 block time after trigger

// Function declarations
void showWiFiStatusScreen(bool isConnected, String ssid = "");
void showSystemStatusScreen();
void showCalibrationScreen(int remainingSeconds);

void setup() {
  Serial.begin(115200);
  Serial.println("\n🚀 Smart Room Controller - Minimal Version");
  Serial.println("📝 Features: HC-SR501 PIR + LED Control + Access System");
  
  // Initialize pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(PIR_SENSOR, INPUT);
  
  // Initial state
  digitalWrite(RELAY_PIN, LOW);
  setAllLEDs(false);
  
  Serial.println("💡 Hardware initialized - All LEDs OFF");
  Serial.println("🔧 HC-SR501 PIR Sensor detected - starting calibration...");
  
  // Start PIR calibration
  pirCalibrationStart = millis();
  pirCalibrated = false;
  
  // Initialize I2C and OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(display.begin(0x3C, true)) {
    Serial.println("📺 OLED initialized successfully");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    showWelcomeScreen();
    delay(2000);
  } else {
    Serial.println("❌ OLED initialization failed");
  }
  
  // Connect to WiFi
  connectToWiFi();
  
  // Auto-detect room
  if (WiFi.status() == WL_CONNECTED) {
    autoDetectRoomId();
    if (roomId > 0) {
      checkRoomStatus();
    }
  }
  
  Serial.println("✅ System Ready!");
  Serial.println("� HC-SR501 PIR Sensor Guide:");
  Serial.println("   🔧 Adjust Sensitivity: Turn left potentiometer");
  Serial.println("   ⏰ Adjust Time Delay: Turn right potentiometer");
  Serial.println("   🔄 Trigger Mode: Set jumper to 'H' for repeatable trigger");
  Serial.println("���💡 LED Behavior:");
  Serial.println("   - LED1: Blinks when waiting for access");
  Serial.println("   - LED2-4: Motion controlled after user access");
  Serial.println("   - All LEDs: ON when door unlocked");
  
  showAccessCodeScreen();
}

void loop() {
  // HC-SR501 Calibration Process
  if (!pirCalibrated) {
    unsigned long calibrationTime = millis() - pirCalibrationStart;
    if (calibrationTime < PIR_CALIBRATION_TIME) {
      // Show calibration progress on OLED
      int remainingSeconds = (PIR_CALIBRATION_TIME - calibrationTime) / 1000;
      static unsigned long lastCalibScreen = 0;
      if (millis() - lastCalibScreen > 1000) {
        showCalibrationScreen(remainingSeconds);
        lastCalibScreen = millis();
      }
      
      // Blink LED1 during calibration
      static unsigned long lastCalibBlink = 0;
      if (millis() - lastCalibBlink > 200) {
        digitalWrite(LED1, !digitalRead(LED1));
        lastCalibBlink = millis();
        
        // Print calibration progress every 10 seconds
        static unsigned long lastCalibPrint = 0;
        if (millis() - lastCalibPrint > 10000) {
          Serial.print("🔧 HC-SR501 Calibrating... ");
          Serial.print(remainingSeconds);
          Serial.println(" seconds remaining");
          lastCalibPrint = millis();
        }
      }
      return; // Don't process anything else during calibration
    } else {
      pirCalibrated = true;
      digitalWrite(LED1, LOW);
      Serial.println("✅ HC-SR501 Calibration Complete!");
      Serial.println("🎯 PIR Sensor ready for motion detection");
      showWelcomeScreen(); // Show welcome screen after calibration
    }
  }
  
  // Handle keypad input
  char key = keypad.getKey();
  if (key) {
    handleKeypadInput(key);
  }
  
  // Check room status periodically
  if (millis() - lastStatusCheck > currentCheckInterval) {
    if (roomId > 0) {
      checkRoomStatus();
    }
    autoDetectRoomId();  // Always check for new bookings
    lastStatusCheck = millis();
  }
  
  // Check motion sensor (only after calibration)
  if (pirCalibrated && millis() - lastMotionCheck > MOTION_CHECK_INTERVAL) {
    checkMotionSensor();
    lastMotionCheck = millis();
  }
  
  // Auto-lock door
  if (doorUnlocked && (millis() - unlockTime > UNLOCK_DURATION)) {
    lockDoor();
  }
  
  // LED1 heartbeat (only when not calibrating)
  if (pirCalibrated) {
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 1000) {
      if (isBookingActive && !userHasAccessed && !doorUnlocked) {
        digitalWrite(LED1, !digitalRead(LED1));  // Blink
      } else {
        digitalWrite(LED1, LOW);
      }
      lastHeartbeat = millis();
    }
  }
  
  delay(100);
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    showWiFiStatusScreen(false);
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.println(WiFi.localIP());
    showWiFiStatusScreen(true, ssid);
    delay(2000); // Show connected status for 2 seconds
  } else {
    Serial.println("\nWiFi failed!");
    showWiFiStatusScreen(false);
    delay(2000);
  }
}

void handleKeypadInput(char key) {
  Serial.print("Key: ");
  Serial.println(key);
  
  switch (key) {
    case '*':
      if (accessCode.length() > 0) {
        accessCode = "";
        Serial.println("Code cleared");
        showAccessCodeScreen();
      } else {
        Serial.println("Showing system status");
        showSystemStatusScreen();
        delay(3000); // Show status for 3 seconds
        showAccessCodeScreen();
      }
      break;
      
    case '#':
      if (accessCode.length() >= 4) {
        Serial.println("Submitting code...");
        attemptUnlock(accessCode);
      } else {
        Serial.println("Code too short");
      }
      break;
      
    default:
      if (accessCode.length() < 10) {
        accessCode += key;
        showAccessCodeScreen();
      }
      break;
  }
}

void checkRoomStatus() {
  if (WiFi.status() != WL_CONNECTED || roomId == 0) return;
  
  HTTPClient http;
  String url = String(serverURL) + "/api/arduino/status/" + String(roomId);
  
  http.setTimeout(5000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    
    DynamicJsonDocument doc(1024);
    if (deserializeJson(doc, response) == DeserializationError::Ok) {
      isBookingActive = doc["isBooked"];
      
      if (!isBookingActive) {
        resetSystemState();
      }
    }
  }
  
  http.end();
}

void attemptUnlock(String code) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("ERROR: WiFi not connected");
    return;
  }
  
  if (roomId == 0) {
    Serial.println("ERROR: No room ID detected");
    return;
  }
  
  Serial.println("=== ATTEMPTING UNLOCK ===");
  Serial.print("Room ID: ");
  Serial.println(roomId);
  Serial.print("Access Code: ");
  Serial.println(code);
  
  HTTPClient http;
  String url = String(serverURL) + "/api/arduino/unlock/" + String(roomId);
  
  Serial.print("URL: ");
  Serial.println(url);
  
  http.setTimeout(8000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["accessCode"] = code;
  doc["deviceId"] = "esp32_room_" + String(roomId);
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.print("Payload: ");
  Serial.println(jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    
    DynamicJsonDocument responseDoc(1024);
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (error) {
      Serial.print("JSON Parse Error: ");
      Serial.println(error.c_str());
      http.end();
      return;
    }
    
    bool success = responseDoc["success"];
    bool unlocked = responseDoc["unlocked"];
    
    Serial.print("Success: ");
    Serial.println(success ? "true" : "false");
    Serial.print("Unlocked: ");
    Serial.println(unlocked ? "true" : "false");
    
    if (success && unlocked) {
      Serial.println("✓ ACCESS GRANTED!");
      userHasAccessed = true;
      unlockDoor();
      setAllLEDs(true);
      showWelcomeMessage();
    } else {
      Serial.println("✗ ACCESS DENIED!");
      showFailScreen();
    }
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == -1) {
      Serial.println("Connection failed - check server");
    } else if (httpResponseCode == -11) {
      Serial.println("Timeout - server too slow");
    }
  }
  
  accessCode = "";
  http.end();
}

void unlockDoor() {
  Serial.println("=== UNLOCKING DOOR ===");
  Serial.print("Relay Pin: ");
  Serial.println(RELAY_PIN);
  
  digitalWrite(RELAY_PIN, HIGH);
  doorUnlocked = true;
  unlockTime = millis();
  
  Serial.println("✓ Door relay activated (HIGH)");
  Serial.println("✓ Door unlocked for 10 seconds");
  
  // Visual confirmation
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED1, HIGH);
    delay(200);
    digitalWrite(LED1, LOW);
    delay(200);
  }
}

void lockDoor() {
  Serial.println("Door locked");
  digitalWrite(RELAY_PIN, LOW);
  doorUnlocked = false;
  
  if (userHasAccessed && isBookingActive) {
    setLED234(true);  // Keep LEDs on for motion control
    lastMotionTime = millis();
  }
}

void checkMotionSensor() {
  bool currentMotion = digitalRead(PIR_SENSOR);
  unsigned long currentTime = millis();
  
  // Motion detection optimized for HC-SR501
  static unsigned long lastTriggerTime = 0;
  static bool lastMotionState = false;
  static unsigned long triggerCount = 0;  // Count triggers for debugging
  
  // Track motion changes
  if (currentMotion != lastMotionState) {
    if (currentMotion) {
      lastTriggerTime = currentTime;
      triggerCount++;
    }
  }
  
  if (userHasAccessed && isBookingActive && !doorUnlocked && pirCalibrated) {
    // HC-SR501 specific motion handling with improved state management
    if (currentMotion) {
      // Any HIGH signal from HC-SR501 = motion detected
      if (!motionDetected) {
        motionDetected = true;
      }
      
      // Update motion timer on every HIGH signal
      lastMotionTime = currentTime;
      
      // Turn on LEDs if not already on
      if (!ledsOn) {
        setLED234(true);
        ledsOn = true;
      }
    }
    else {
      // HC-SR501 is LOW - check for timeout
      if (motionDetected) {
        unsigned long timeSinceMotion = currentTime - lastMotionTime;
        if (timeSinceMotion > MOTION_TIMEOUT) {
          // Timeout reached - turn off everything
          motionDetected = false;
          setLED234(false);
          ledsOn = false;
          Serial.print("🔇 ❌ Motion timeout reached (Trigger #");
          Serial.print(triggerCount);
          Serial.println(") - LEDs OFF");
        } else {
          Serial.print("⏳ No motion for ");
          Serial.print(timeSinceMotion / 1000);
          Serial.print("s / ");
          Serial.print(MOTION_TIMEOUT / 1000);
          Serial.print("s timeout (Trigger #");
          Serial.print(triggerCount);
          Serial.println(")");
        }
      }
    }
    
    lastMotionState = currentMotion;
  } else {
    // Force reset when conditions not met
    if (motionDetected || ledsOn) {
      motionDetected = false;
      ledsOn = false;
      setLED234(false);
    }
  }
}

void autoDetectRoomId() {
  if (WiFi.status() != WL_CONNECTED) return;
  
  Serial.print("🔍 Checking for new bookings (interval: ");
  Serial.print(currentCheckInterval / 1000);
  Serial.println("s)...");
  
  HTTPClient http;
  String url = String(serverURL) + "/api/bookings/all";
  
  http.setTimeout(5000);
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    
    // Check for rate limiting
    if (response.indexOf("Too many requests") >= 0) {
      rateLimitCount++;
      unsigned long backoffTime = STATUS_CHECK_INTERVAL_SLOW * rateLimitCount; // Exponential backoff
      if (backoffTime > 300000) backoffTime = 300000; // Max 5 minutes
      
      currentCheckInterval = backoffTime;
      
      Serial.print("⚠️ Rate limited (");
      Serial.print(rateLimitCount);
      Serial.print(" times) - backing off to ");
      Serial.print(backoffTime / 1000);
      Serial.println(" seconds");
      
      http.end();
      return;
    } else {
      // Reset on successful response
      if (rateLimitCount > 0) {
        Serial.println("✅ Rate limit cleared - returning to normal speed");
        rateLimitCount = 0;
      }
      currentCheckInterval = STATUS_CHECK_INTERVAL;  // Reset to normal speed
    }
    
    Serial.print("📥 Raw response: ");
    Serial.println(response.substring(0, 200));  // Show first 200 chars
    
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, response) == DeserializationError::Ok && doc.is<JsonArray>()) {
      JsonArray bookings = doc.as<JsonArray>();
      
      Serial.print("📊 Found ");
      Serial.print(bookings.size());
      Serial.println(" total bookings");
      
      bool foundActive = false;
      
      for (JsonObject booking : bookings) {
        String status = booking["status"].as<String>();
        
        Serial.print("📋 Booking status: ");
        Serial.println(status);
        
        if (status == "active") {
          foundActive = true;
          int newRoomId = booking["room_id"];
          String newBookingId = booking["id"].as<String>();
          
          Serial.print("✅ Active booking - Room: ");
          Serial.print(newRoomId);
          Serial.print(", ID: ");
          Serial.println(newBookingId);
          
          if (newRoomId != roomId || newBookingId != currentBookingId) {
            Serial.println("🎯 === NEW BOOKING DETECTED ===");
            Serial.print("Previous Room ID: ");
            Serial.print(roomId);
            Serial.print(" → New Room ID: ");
            Serial.println(newRoomId);
            Serial.print("Previous Booking ID: ");
            Serial.print(currentBookingId);
            Serial.print(" → New Booking ID: ");
            Serial.println(newBookingId);
            
            roomId = newRoomId;
            currentBookingId = newBookingId;
            
            resetSystemState();
            isBookingActive = true;
            
            Serial.println("✅ System state reset for new booking");
            Serial.println("📺 Updating display...");
            showAccessCodeScreen();
          } else {
            Serial.println("📋 Same booking - no changes needed");
          }
          break;
        }
      }
      
      if (!foundActive) {
        Serial.println("❌ No active bookings found");
        if (roomId > 0) {
          Serial.println("🔄 Resetting room ID - no active bookings");
          roomId = 0;
          currentBookingId = "";
          resetSystemState();
        }
      }
    } else {
      Serial.println("❌ Failed to parse bookings JSON");
      Serial.print("Response: ");
      Serial.println(response.substring(0, 100));
    }
  } else {
    Serial.print("❌ HTTP Error: ");
    Serial.println(httpResponseCode);
    if (httpResponseCode == 429) {
      Serial.println("⚠️ Too Many Requests - reducing check frequency");
    }
  }
  
  http.end();
}

// Helper Functions
void setLED234(bool state) {
  digitalWrite(LED2, state ? HIGH : LOW);
  digitalWrite(LED3, state ? HIGH : LOW);
  digitalWrite(LED4, state ? HIGH : LOW);
}

void setAllLEDs(bool state) {
  digitalWrite(LED1, state ? HIGH : LOW);
  setLED234(state);
}

void resetSystemState() {
  userHasAccessed = false;
  doorUnlocked = false;
  motionDetected = false;
  ledsOn = false;
  setAllLEDs(false);
  digitalWrite(RELAY_PIN, LOW);
}

// Display Functions
void showWelcomeScreen() {
  display.clearDisplay();
  
  // Decorative border with corners
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
  display.drawRect(1, 1, SCREEN_WIDTH-2, SCREEN_HEIGHT-2, SH110X_WHITE);
  
  // Corner decorations
  display.fillRect(0, 0, 8, 8, SH110X_WHITE);
  display.fillRect(SCREEN_WIDTH-8, 0, 8, 8, SH110X_WHITE);
  display.fillRect(0, SCREEN_HEIGHT-8, 8, 8, SH110X_WHITE);
  display.fillRect(SCREEN_WIDTH-8, SCREEN_HEIGHT-8, 8, 8, SH110X_WHITE);
  
  // Clear corner centers
  display.fillRect(2, 2, 4, 4, SH110X_BLACK);
  display.fillRect(SCREEN_WIDTH-6, 2, 4, 4, SH110X_BLACK);
  display.fillRect(2, SCREEN_HEIGHT-6, 4, 4, SH110X_BLACK);
  display.fillRect(SCREEN_WIDTH-6, SCREEN_HEIGHT-6, 4, 4, SH110X_BLACK);
  
  // ASCII Art Style Title
  display.setTextSize(1);
  
  // "SMART" in stylized form
  display.setCursor(20, 10);
  display.println("*** SMART ***");
  
  // Decorative line
  display.drawLine(15, 22, 113, 22, SH110X_WHITE);
  display.drawLine(15, 23, 113, 23, SH110X_WHITE);
  
  // "ROOM" in larger, bold style
  display.setTextSize(2);
  display.setCursor(25, 28);
  display.println("ROOM");
  
  // Shadow effect for ROOM
  display.setTextSize(2);
  display.setCursor(26, 29);
  display.setTextColor(SH110X_BLACK);
  display.println("ROOM");
  display.setTextColor(SH110X_WHITE);
  
  // Subtitle with decorative elements
  display.drawLine(10, 48, 118, 48, SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(25, 52);
  display.println("CONTROLLER v1.0");
  display.drawLine(10, 60, 118, 60, SH110X_WHITE);
  
  display.display();
}

void showAccessCodeScreen() {
  Serial.println("📺 === UPDATING DISPLAY ===");
  Serial.print("Room ID: ");
  Serial.println(roomId);
  Serial.print("Booking Active: ");
  Serial.println(isBookingActive ? "YES" : "NO");
  Serial.print("Current Booking ID: ");
  Serial.println(currentBookingId);
  
  display.clearDisplay();
  
  // Decorative header
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SH110X_WHITE);
  display.fillRect(2, 2, SCREEN_WIDTH-4, 12, SH110X_BLACK);
  
  // Title with decorative styling
  display.setTextSize(1);
  display.setCursor(20, 5);
  display.setTextColor(SH110X_WHITE);
  display.println(">> ACCESS CODE <<");
  display.setTextColor(SH110X_WHITE);
  
  // Room status with icon
  display.setCursor(2, 18);
  display.print("Room ");
  display.print(roomId > 0 ? String(roomId) : "--");
  
  // Status indicator
  int statusX = 80;
  if (isBookingActive) {
    display.fillCircle(statusX, 21, 3, SH110X_WHITE);
    display.setCursor(statusX + 8, 18);
    display.println("ACTIVE");
  } else {
    display.drawCircle(statusX, 21, 3, SH110X_WHITE);
    display.setCursor(statusX + 8, 18);
    display.println("WAIT");
  }
  
  // Clean code input boxes
  display.setTextSize(2);
  for (int i = 0; i < 6; i++) {
    int x = 8 + (i * 19);
    int y = 32;
    
    // Simple box design
    display.fillRect(x, y, 16, 18, SH110X_BLACK);  // Clear the area first
    display.drawRect(x, y, 16, 18, SH110X_WHITE);   // Draw border
    
    // Display digit
    if (i < accessCode.length()) {
      display.setCursor(x + 4, y + 3);
      display.print(accessCode[i]);
    } else {
      // Show cursor for current position
      if (i == accessCode.length()) {
        display.drawLine(x + 8, y + 4, x + 8, y + 14, SH110X_WHITE);
      }
    }
  }
  
  // Bottom instructions with separator
  display.drawLine(0, 54, SCREEN_WIDTH, 54, SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(8, 57);
  display.print("# Enter");
  display.setCursor(70, 57);
  display.print("* Clear");
  
  display.display();
}

void showWelcomeMessage() {
  display.clearDisplay();
  
  // Simple success frame
  display.drawRect(5, 5, SCREEN_WIDTH-10, SCREEN_HEIGHT-10, SH110X_WHITE);
  display.drawRect(7, 7, SCREEN_WIDTH-14, SCREEN_HEIGHT-14, SH110X_WHITE);
  
  // Checkmark icon
  display.drawLine(25, 30, 30, 35, SH110X_WHITE);
  display.drawLine(30, 35, 40, 25, SH110X_WHITE);
  display.drawLine(26, 30, 31, 35, SH110X_WHITE);
  display.drawLine(31, 35, 41, 25, SH110X_WHITE);
  
  // Clear text areas first
  display.fillRect(50, 20, 70, 30, SH110X_BLACK);
  display.fillRect(20, 45, 90, 15, SH110X_BLACK);
  
  // Success text
  display.setTextSize(1);
  display.setCursor(50, 25);
  display.println("ACCESS");
  display.setCursor(50, 35);
  display.println("GRANTED");
  
  // Welcome message
  display.setCursor(30, 50);
  display.println("WELCOME!");
  
  display.display();
  delay(3000);
  showAccessCodeScreen();
}

void showFailScreen() {
  display.clearDisplay();
  
  // Error frame with double border
  display.drawRect(8, 8, SCREEN_WIDTH-16, SCREEN_HEIGHT-16, SH110X_WHITE);
  display.drawRect(10, 10, SCREEN_WIDTH-20, SCREEN_HEIGHT-20, SH110X_WHITE);
  
  // X mark icon
  display.drawLine(25, 25, 35, 35, SH110X_WHITE);
  display.drawLine(35, 25, 25, 35, SH110X_WHITE);
  display.drawLine(26, 25, 36, 35, SH110X_WHITE);
  display.drawLine(35, 26, 25, 36, SH110X_WHITE);
  
  // Error text
  display.setTextSize(1);
  display.setCursor(50, 25);
  display.println("ACCESS");
  display.setCursor(52, 35);
  display.println("DENIED");
  
  // Try again message
  display.setCursor(25, 50);
  display.println("TRY AGAIN");
  
  display.display();
  delay(3000);
  showAccessCodeScreen();
}

// Simple PIR calibration screen
void showCalibrationScreen(int remainingSeconds) {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(20, 15);
  display.println("PIR Calibrating");
  
  display.setCursor(45, 30);
  display.print(remainingSeconds);
  display.println(" sec");
  
  // Simple progress bar
  int progress = map(60 - remainingSeconds, 0, 60, 0, 100);
  display.drawRect(20, 45, 88, 6, SH110X_WHITE);
  display.fillRect(22, 47, map(progress, 0, 100, 0, 84), 2, SH110X_WHITE);
  
  display.display();
}

// Simplified WiFi status screen
void showWiFiStatusScreen(bool isConnected, String ssid) {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(35, 15);
  display.println("WiFi");
  
  if (isConnected) {
    display.setCursor(25, 30);
    display.println("Connected");
    display.setCursor(30, 45);
    display.println(ssid);
  } else {
    display.setCursor(25, 30);
    display.println("Connecting");
    display.setCursor(45, 45);
    display.println("...");
  }
  
  display.display();
}

// Simple system status screen
void showSystemStatusScreen() {
  display.clearDisplay();
  
  // Header
  display.setTextSize(1);
  display.setCursor(30, 5);
  display.println("STATUS");
  display.drawLine(0, 15, SCREEN_WIDTH, 15, SH110X_WHITE);
  
  // PIR Status
  display.setCursor(5, 20);
  display.print("PIR: ");
  display.println(pirCalibrated ? "OK" : "CAL");
  
  // WiFi Status  
  display.setCursor(5, 30);
  display.print("WiFi: ");
  display.println(WiFi.status() == WL_CONNECTED ? "OK" : "NO");
  
  // Motion Status
  display.setCursor(5, 40);
  display.print("Motion: ");
  display.println(motionDetected ? "YES" : "NO");
  
  // Door Status
  display.setCursor(5, 50);
  display.print("Door: ");
  display.println(digitalRead(RELAY_PIN) ? "OPEN" : "LOCK");
  
  display.display();
}

