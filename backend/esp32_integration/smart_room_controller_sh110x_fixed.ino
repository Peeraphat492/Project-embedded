/*
  Smart Room Reservation - Arduino Controller (SH110X OLED Version)
  ESP32 WiFi-enabled room access control system
  
  Features:
  - WiFi connectivity
  - HTTP communication with backend server
  - Keypad input for access codes
  - Relay control for door lock
  - LED status indicators
  - Motion sensor for occupancy detection
  - SH110X OLED Display
  
  Hardware connections:
  - Keypad: GPIO 12-19
  - Relay (Door Lock): GPIO 23
  - LEDs: GPIO 2 (status), GPIO 4 (error), GPIO 5 (success)
  - Motion Sensor: GPIO 21
  - OLED: GPIO 21 (SDA), GPIO 22 (SCL)
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Keypad.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// WiFi Configuration -  Phone WiFi Hotspot  
const char* ssid = "Peeraphat9";                
const char* password = "12345678";              // Updated password

// Server Configuration - Backend API Direct Access
const char* serverURL = "http://172.20.10.2:3000";     // Your Node.js backend server
int roomId = 0;  // Room ID will be auto-detected from backend API

// Backend API Endpoints ( backend )
// GET  /api/arduino/status/{roomId}     - Check room booking status
// POST /api/arduino/unlock/{roomId}     - Attempt to unlock room  
// POST /api/arduino/checkin/{roomId}    - Check in to room
// POST /api/arduino/checkout/{roomId}   - Check out from room
// GET  /api/bookings/all                - Auto-detect active bookings

// -----------------------------
//   ESP32
// -----------------------------

// --- PIR Sensor ---
#define PIR_SENSOR 4        //  SIG  GPIO4

// --- OLED Display (I2C) SH110X ---
#define OLED_SDA 21
#define OLED_SCL 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Initialize SH110X display
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- LED  ---
#define LED1 19             // LED Status (Blue) - 
#define LED2 18             // LED Error (Red) - 
#define LED3 16             // LED Success (Green) - 
#define LED4 13             // LED Available (Yellow) - 

// --- Relay/Door Control ---
#define RELAY_PIN 23        // Door lock relay

// --- Legacy definitions for compatibility ---
#define LED_STATUS LED1     // Blue LED - System status
#define LED_ERROR LED2      // Red LED - Error indicator  
#define LED_SUCCESS LED3    // Green LED - Success indicator
#define LED_AVAILABLE LED4  // Yellow LED - Available indicator
#define MOTION_SENSOR PIR_SENSOR  // PIR motion sensor

// Keypad Configuration ( Pin )
const byte ROWS = 4;
const byte COLS = 3;  //  4x3 keypad
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {14, 32, 27, 26};  // R1, R2, R3, R4
byte colPins[COLS] = {12, 25, 33};      // C1, C2, C3
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Global Variables
String accessCode = "";
bool isRoomOccupied = false;
bool doorUnlocked = false;
bool isBookingActive = false;  // Track if current time is within booking
bool userHasAccessed = false;  // Track if user has successfully entered room
String bookingStartTime = "";  // Store booking start time
String bookingEndTime = "";    // Store booking end time  
String currentTime = "";       // Store current time from server

// PIR Sensor & Display Control Variables
bool motionDetected = false;
bool displayActive = true;
bool displayDimmed = false;
bool ledsDimmed = false;  // Track if LED2,3,4 are dimmed
unsigned long lastMotionTime = 0;
unsigned long lastMotionDebounce = 0;  // Add debouncing for PIR
unsigned long lastDisplayToggle = 0;
const unsigned long MOTION_TIMEOUT = 15000;      // 15 seconds no motion = dim LED2,3,4
const unsigned long MOTION_DEBOUNCE_TIME = 300;   // 300ms debounce for faster response
const unsigned long SCREENSAVER_TIMEOUT = 60000; // 1 minute no motion = screensaver
const unsigned long DISPLAY_CHECK_INTERVAL = 1000; // Check display state every second

unsigned long lastStatusCheck = 0;
unsigned long lastMotionCheck = 0;
unsigned long unlockTime = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastScreenSaver = 0;
unsigned long lastLEDUpdate = 0;
const unsigned long STATUS_CHECK_INTERVAL = 30000;  // Check status every 30 seconds
const unsigned long MOTION_CHECK_INTERVAL = 500;    // Check motion every 500ms for faster response
const unsigned long UNLOCK_DURATION = 10000;        // Keep door unlocked for 10 seconds
const unsigned long DISPLAY_REFRESH_INTERVAL = 1000; // Refresh display every 1 second
const unsigned long LED_UPDATE_INTERVAL = 2000;     // Update booking LEDs every 2 seconds

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n🏠 Smart Room Controller Starting...");
  
  // Initialize I2C for OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  Serial.print("🔧 I2C initialized on SDA:");
  Serial.print(OLED_SDA);
  Serial.print(", SCL:");
  Serial.println(OLED_SCL);
  
  // Initialize SH110X OLED display with retry mechanism
  bool oledInitialized = false;
  Serial.println("🔍 Scanning I2C addresses...");
  
  // Scan I2C addresses
  for(byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if(Wire.endTransmission() == 0) {
      Serial.print("📡 Found I2C device at 0x");
      if(addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
    }
  }
  
  // Try different I2C addresses for SH110X
  byte addresses[] = {0x3C, 0x3D};
  for(int i = 0; i < 2; i++) {
    Serial.print("🔗 Trying OLED at address 0x");
    if(addresses[i] < 16) Serial.print("0");
    Serial.println(addresses[i], HEX);
    
    // Try to initialize with current address
    if(display.begin(addresses[i], true)) {  // Address, reset
      Serial.println("✅ SH110X OLED initialized successfully!");
      oledInitialized = true;
      break;
    } else {
      Serial.print("❌ Failed at address 0x");
      if(addresses[i] < 16) Serial.print("0");
      Serial.println(addresses[i], HEX);
    }
    
    delay(500);
  }
  
  if(oledInitialized) {
    Serial.println("⚙️ Configuring display settings...");
    
    // Configure display settings
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setTextWrap(false);
    
    // Test display immediately
    Serial.println("🧪 Testing display...");
    display.setCursor(0, 0);
    display.println("** SMART ROOM **");
    display.setCursor(0, 15);
    display.println("System Starting...");
    display.setCursor(0, 30);
    display.println("SH110X OLED Test");
    display.setCursor(0, 45);
    display.println("Please Wait...");
    display.display();
    
    delay(3000);  // Show test for 3 seconds
    
    showWelcomeScreen();
    lastDisplayUpdate = millis();
    lastScreenSaver = millis();
  } else {
    Serial.println("❌ OLED initialization completely failed");
    Serial.println("🔧 Check connections:");
    Serial.println("   VCC -> 3.3V");
    Serial.println("   GND -> GND");
    Serial.println("   SDA -> GPIO 21");
    Serial.println("   SCL -> GPIO 22");
    Serial.println("📍 Common SH110X addresses: 0x3C, 0x3D");
  }
  
  // Initialize hardware pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);
  pinMode(LED_SUCCESS, OUTPUT);
  pinMode(LED_AVAILABLE, OUTPUT);
  pinMode(MOTION_SENSOR, INPUT);
  
  // Initial state - door locked, all LEDs off (will be controlled by booking status)
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_STATUS, LOW);
  digitalWrite(LED_ERROR, LOW);
  digitalWrite(LED_SUCCESS, LOW);
  digitalWrite(LED_AVAILABLE, LOW);
  
  Serial.println("💡 All LEDs initialized OFF - LED1 will blink for standby, others on when unlocked");
  
  // Connect to WiFi
  connectToWiFi();
  
  // Auto-detect room ID from active bookings
  if (WiFi.status() == WL_CONNECTED) {
    autoDetectRoomId();
  }
  
  // Initial status check
  if (roomId > 0) {
    checkRoomStatus();
  } else {
    Serial.println("⚠️ Room ID not detected. Please check server for active bookings.");
  }
  
  Serial.println("🎉 Room Controller Ready!");
  Serial.println("📋 Commands:");
  Serial.println("   - Enter access code on keypad");
  Serial.println("   - Press * to clear code");
  Serial.println("   - Press # to submit code");
  Serial.println("💡 LED Behavior:");
  Serial.println("   - LED1 (Blue): Blinks when waiting for room access");
  Serial.println("   - LED2,3,4: Turn ON when user enters correct code");
  Serial.println("   - LED2,3,4: Turn OFF when booking time ends");
  Serial.println("   - ALL LEDs: Solid when door unlocked (10 seconds)");
  Serial.println("   - Display always on (no screensaver)");
  
  // Signal ready with brief LED test, then start standby mode
  blinkLED(LED1, 2);  // Brief signal ready
  digitalWrite(LED1, LOW);  // Turn off after signal
  
  // Initialize smart display control
  lastMotionTime = millis();  // Start with display active
  displayActive = true;
  displayDimmed = false;
  Serial.println("📺 Smart Display Control initialized");
  Serial.println("⏰ Display timeout: 30s dim, 60s screensaver");
  
  // Show ready screen
  showReadyScreen();
  
  // Show initial access code screen
  delay(2000);  // Show ready screen for 2 seconds
  showAccessCodeScreen();
}

void loop() {
  // Handle keypad input
  char key = keypad.getKey();
  if (key) {
    handleKeypadInput(key);
    lastScreenSaver = millis();  // Reset screensaver timer
    // Display always active - no screensaver check needed
  }
  
  // Periodic status checks
  if (millis() - lastStatusCheck > STATUS_CHECK_INTERVAL) {
    checkRoomStatus();
    lastStatusCheck = millis();
  }
  
  // Motion detection and display control
  if (millis() - lastMotionCheck > MOTION_CHECK_INTERVAL) {
    checkMotionSensor();
    lastMotionCheck = millis();
  }
  
  // Smart display control based on motion
  if (millis() - lastDisplayToggle > DISPLAY_CHECK_INTERVAL) {
    handleSmartDisplayControl();
    lastDisplayToggle = millis();
  }
  
  // Auto-lock door after unlock duration
  if (doorUnlocked && (millis() - unlockTime > UNLOCK_DURATION)) {
    lockDoor();
  }
  
  // Display management
  manageDisplay();
  
  // Status LED heartbeat - LED1 
  static unsigned long lastHeartbeat = 0;
  static unsigned long lastWiFiCheck = 0;
  
  if (millis() - lastHeartbeat > 1000) {  //  1 
    // LED1 
    if (isBookingActive && !userHasAccessed && !doorUnlocked) {
      digitalWrite(LED1, !digitalRead(LED1));  //  LED1 ()
      // LED1 blinking for booking standby (debug frequency reduced)
    } else if (!isBookingActive) {
      digitalWrite(LED1, LOW);  //  LED1 
      // Serial.println("LED1 OFF - no booking");  // Reduced frequency
    } else if (userHasAccessed) {
      digitalWrite(LED1, LOW);  //  user 
      // LED1 OFF - user has accessed (debug removed for cleaner output)
    }
    
    lastHeartbeat = millis();
  }
  
  // Update booking LEDs based on user access status
  if (millis() - lastLEDUpdate > LED_UPDATE_INTERVAL) {
    if (userHasAccessed && isBookingActive) {
      // User has accessed - check motion dimming state before forcing LEDs
      if (!ledsDimmed) {
        setLED234(true);  //  helper function
        // LED2,3,4 maintained ON (debug removed for cleaner output)
      } else {
        Serial.println("LED2,3,4 kept dimmed (no motion)");
      }
      // NOTE: Removed checkBookingTimeEnd() - let server handle booking end detection
    } else if (!userHasAccessed) {
      // User hasn't accessed yet - normal LED behavior
      updateBookingLEDs();
    } else if (!isBookingActive) {
      // Booking no longer active from server - turn off LEDs and reset
      Serial.println("⏰ Booking ended by server - turning off LED2,3,4");
      setLED234(false);  //  helper function
      userHasAccessed = false;
      resetLEDDimming();  // Reset LED dimming state when booking ends
    }
    lastLEDUpdate = millis();
  }
  
  // Simple WiFi check (no auto-reconnect)
  if (millis() - lastWiFiCheck > 30000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("📶❌ WiFi disconnected");
    }
    lastWiFiCheck = millis();
  }
  
  delay(100);
}

void connectToWiFi() {
  Serial.println("📶 Connecting to WiFi...");
  Serial.print("📡 SSID: ");
  Serial.println(ssid);
  
  // Check if already connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ WiFi already connected!");
    Serial.print("🌐 IP: ");
    Serial.println(WiFi.localIP());
    return;
  }
  
  // Reset WiFi and connect
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  
  Serial.println("🔄 Starting WiFi connection...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
    
    if (attempts % 5 == 0) {
      Serial.print(" (");
      Serial.print(WiFi.status());
      Serial.print(") ");
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
    Serial.print("🌐 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi failed!");
  }
}

void handleKeypadInput(char key) {
  Serial.print("  Key pressed: ");
  Serial.println(key);
  
  // Wake up display immediately when keypad is pressed
  lastMotionTime = millis();  // Treat keypad input as motion
  if (!displayActive || displayDimmed) {
    Serial.println("  Keypad input - Activating display");
    activateDisplay();
  }
  
  // Visual feedback with LED - LED1 
  if (isBookingActive && !doorUnlocked) {
    blinkLED(LED1, 1);
  }
  
  // Reset display activity
  lastDisplayUpdate = millis();
  
  // Immediate visual feedback on display
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Key: ");
  display.print(key);
  display.display();
  delay(300);  // Show briefly
  
  switch (key) {
    case '*':
      // Clear access code
      accessCode = "";
      Serial.println("  Access code cleared");
      if (isBookingActive && !doorUnlocked) {
        blinkLED(LED1, 1);  // LED1  clear
      }
      refreshDisplay();
      showAccessCodeScreen();  // Update display
      break;
      
    case '#':
      // Submit access code
      if (accessCode.length() >= 4) {
        Serial.print("Submitting access code: ");
        Serial.println(accessCode);
        refreshDisplay();
        showSubmittingScreen();  // Show submitting message
        attemptUnlock(accessCode);
      } else {
        Serial.println("Access code too short (minimum 4 digits)");
        if (isBookingActive && !doorUnlocked) {
          blinkLED(LED1, 2);  // LED1  error
        }
        refreshDisplay();
        showErrorScreen("Code too short");
      }
      break;
      
    default:
      // Add digit to access code
      if (accessCode.length() < 10) {  // Limit access code length
        accessCode += key;
        Serial.print("Access code: ");
        // Print asterisks for security
        for (int i = 0; i < accessCode.length(); i++) {
          Serial.print("*");
        }
        Serial.println();
        Serial.print("Updating display with code: ");
        Serial.println(accessCode);
        refreshDisplay();
        showAccessCodeScreen();  // Update display with new digit
      } else {
        Serial.println("Access code too long");
        if (isBookingActive && !doorUnlocked) {
          blinkLED(LED1, 1);  // LED1  error
        }
        refreshDisplay();
        showErrorScreen("Code too long");
      }
      break;
  }
}

void checkRoomStatus() {
  // Skip if WiFi not connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - skipping status check");
    return;
  }
  
  // If no room ID detected, try to auto-detect again
  if (roomId == 0) {
    Serial.println("No room ID set, attempting auto-detection...");
    autoDetectRoomId();
    if (roomId == 0) {
      Serial.println("Still no active booking found");
      return;
    }
  }
  
  HTTPClient http;
  String url = String(serverURL) + "/api/arduino/status/" + String(roomId);
  
  Serial.println(" === BACKEND API CALL ===");
  Serial.print("Endpoint: ");
  Serial.println(url);
  Serial.println("Direct backend access (no database credentials needed)");
  
  Serial.println("=== CONNECTION DEBUG ===");
  Serial.print("Full URL: ");
  Serial.println(url);
  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  Serial.print("Arduino IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS IP: ");
  Serial.println(WiFi.dnsIP());
  Serial.print("Room ID: ");
  Serial.println(roomId);
  Serial.println("========================");
  
  http.setTimeout(10000);  // 10 second timeout
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  Serial.println("Sending HTTP GET request...");
  int httpResponseCode = http.GET();
  
  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Response: ");
    Serial.println(response);
    
    // Parse JSON response
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.print("JSON Parse Error: ");
      Serial.println(error.c_str());
      return;
    }
    
    bool isBooked = doc["isBooked"];
    String roomStatus = doc["roomStatus"];
    
    // Get booking time information if available
    if (doc.containsKey("currentBooking") && !doc["currentBooking"].isNull()) {
      JsonObject booking = doc["currentBooking"];
      bookingStartTime = booking["start_time"].as<String>();
      bookingEndTime = booking["end_time"].as<String>();
      Serial.print("Booking: ");
      Serial.print(bookingStartTime);
      Serial.print(" - ");
      Serial.println(bookingEndTime);
    } else {
      bookingStartTime = "";
      bookingEndTime = "";
    }
    
    // Get current server time if available
    if (doc.containsKey("timestamp")) {
      currentTime = doc["timestamp"].as<String>();
    }
    
    isRoomOccupied = (roomStatus == "occupied");
    isBookingActive = isBooked;  // Update booking status
    
    Serial.print("Room Status: ");
    Serial.print(roomStatus);
    Serial.print(", Booked: ");
    Serial.println(isBooked ? "Yes" : "No");
    Serial.print("LED Status: ");
    Serial.println(isBookingActive ? "LED1 Blinking + Booking LEDs" : "All LEDs Off (No Booking)");
    
    // LED behavior based on booking status
    if (!doorUnlocked && !userHasAccessed) {
      updateBookingLEDs();  // Only update if user hasn't accessed yet
    }
    
    // Reset access flag if no booking
    if (!isBookingActive) {
      userHasAccessed = false;
      Serial.println("No booking - reset access flag");
      // Turn off all LEDs when no booking
      digitalWrite(LED1, LOW);
      setLED234(false);  //  helper function
    }
    
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpResponseCode);
    
    // Print detailed error
    if (httpResponseCode == -1) {
      Serial.println("Connection failed - server not reachable");
      Serial.println("Check server is running and port is correct");
    } else if (httpResponseCode == -11) {
      Serial.println("Timeout - server took too long to respond");
    } else {
      Serial.println("Check server URL and network connectivity");
    }
    
    blinkLED(LED2, 2);
  }
  
  http.end();
}

void attemptUnlock(String code) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    blinkLED(LED_ERROR, 3);
    return;
  }
  
  HTTPClient http;
  String url = String(serverURL) + "/api/arduino/unlock/" + String(roomId);
  
  Serial.println(" === BACKEND UNLOCK REQUEST ===");
  Serial.print("API Endpoint: ");
  Serial.println(url);
  Serial.print("Access Code: ");
  Serial.println(code);
  Serial.println("Sending to backend for validation...");
  
  http.setTimeout(15000);  // 15 second timeout for unlock requests
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP32-SmartRoom-Controller");
  
  // Create JSON payload for backend
  DynamicJsonDocument doc(512);
  doc["accessCode"] = code;
  doc["deviceId"] = "esp32_room_" + String(roomId);
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.print("JSON Payload: ");
  Serial.println(jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Unlock response: ");
    Serial.println(response);
    
    // Parse JSON response
    DynamicJsonDocument responseDoc(1024);
    deserializeJson(responseDoc, response);
    
    bool success = responseDoc["success"];
    bool unlocked = responseDoc["unlocked"];
    
    if (success && unlocked) {
      Serial.println("🔓 Access granted!");
      
      // CRITICAL: Set userHasAccessed FIRST before any LED operations
      userHasAccessed = true;
      Serial.println("userHasAccessed set to TRUE - LED2,3,4 MUST stay on!");
      
      unlockDoor();
      checkIn();
      
      // Success - Turn on all LEDs
      turnOnUnlockLEDs();
      showWelcomeMessage();  // Show WELCOME message
      
    } else {
      Serial.println("🔒 Access denied!");
      if (isBookingActive && !doorUnlocked) {
        blinkLED(LED1, 3);  // LED1  access denied
      }
      showFailScreen();  // Show FAIL message
    }
    
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpResponseCode);
    blinkLED(LED_ERROR, 3);
  }
  
  // Clear access code after attempt
  accessCode = "";
  http.end();
}

void unlockDoor() {
  Serial.println("Unlocking door...");
  digitalWrite(RELAY_PIN, HIGH);  // Activate relay
  
  doorUnlocked = true;
  unlockTime = millis();
  
  Serial.println("🚪 Door unlocked!");
}

void turnOnUnlockLEDs() {
  Serial.println("🔥 === DOOR UNLOCKED - ALL LEDS ON ===");
  // Serial.println("userHasAccessed: " + String(userHasAccessed));  // Removed for cleaner output
  // Serial.println("isBookingActive: " + String(isBookingActive));  // Removed for cleaner output
  
  // When door is unlocked, all LEDs turn on solid
  setAllLEDs(true);  //  helper function 
  
  // Ensure userHasAccessed flag is set (redundant safety check)
  if (!userHasAccessed) {
    userHasAccessed = true;
    Serial.println("userHasAccessed set to TRUE in turnOnUnlockLEDs()");
  }
  
  Serial.println("💡 ALL LEDs ON - Door unlocked mode!");
  Serial.println("LED2,3,4 should stay on even after door locks!");
}

void lockDoor() {
  Serial.println("🔒 === DOOR LOCKED ===");
  digitalWrite(RELAY_PIN, LOW);   // Deactivate relay
  
  doorUnlocked = false;
  
  Serial.println("🔒 Door locked!");
  
  // ALWAYS keep LED2-4 on if user has accessed, regardless of other conditions
  if (userHasAccessed && isBookingActive) {
    Serial.println("Door locked but user accessed - checking motion dimming");
    if (!ledsDimmed) {
      setLED234(true);  //  helper function
      Serial.println("LED2,3,4 kept ON (motion active)");
    } else {
      Serial.println("LED2,3,4 kept dimmed (no motion)");
    }
  } else {
    Serial.println("Door locked - returning to standby mode");
    setLED234(false);  //  helper function
  }
}

void checkIn() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected for check-in");
    return;
  }
  
  HTTPClient http;
  String url = String(serverURL) + "/api/arduino/checkin/" + String(roomId);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST("{}");
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Check-in response: ");
    Serial.println(response);
  } else {
    Serial.print("Check-in HTTP Error: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

void checkOut() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected for check-out");
    return;
  }
  
  HTTPClient http;
  String url = String(serverURL) + "/api/arduino/checkout/" + String(roomId);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST("{}");
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Check-out response: ");
    Serial.println(response);
    
    // Lock door after checkout
    lockDoor();
    
    // Turn off all LEDs after checkout
    digitalWrite(LED1, LOW);
    setLED234(false);  //  helper function
    
  } else {
    Serial.print("Check-out HTTP Error: ");
    Serial.println(httpResponseCode);
    if (isBookingActive && !doorUnlocked) {
      blinkLED(LED1, 2);  // LED1  error
    }
  }
  
  http.end();
}

void checkMotionSensor() {
  bool currentMotion = digitalRead(MOTION_SENSOR);
  unsigned long currentTime = millis();
  
  // Motion detection for LED2-4 control with reduced debouncing for faster response
  if (currentMotion) {
    // Only process motion if enough time has passed since last detection (faster debouncing)
    if (!motionDetected && (currentTime - lastMotionDebounce > MOTION_DEBOUNCE_TIME)) {
      // Motion detected - brighten LED2,3,4 (debug message removed for cleaner output)
      motionDetected = true;
      lastMotionDebounce = currentTime;
      
      // Brighten LEDs immediately when motion is detected
      brightenLEDs();
    }
    lastMotionTime = currentTime;  // Update last motion time for LED control
  } else {
    // Reduce motion end delay for faster response
    if (motionDetected && (currentTime - lastMotionTime > 200)) {  // Reduced to 200ms
      motionDetected = false;
      // Motion ended - starting timeout timer (debug message removed for cleaner output)
    }
  }
  
  // Check if we should dim LEDs due to no motion
  unsigned long timeSinceMotion = currentTime - lastMotionTime;
  if (timeSinceMotion > MOTION_TIMEOUT && userHasAccessed && isBookingActive && !ledsDimmed) {
    Serial.println("Motion timeout reached - dimming LEDs");
    dimLEDs();
  }
}

// LED Helper Functions - 
void setLED234(bool state) {
  digitalWrite(LED2, state ? HIGH : LOW);
  digitalWrite(LED3, state ? HIGH : LOW);
  digitalWrite(LED4, state ? HIGH : LOW);
}

void setAllLEDs(bool state) {
  digitalWrite(LED1, state ? HIGH : LOW);
  setLED234(state);
}

void blinkLED(int pin, int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(200);
    digitalWrite(pin, LOW);
    delay(200);
  }
}

// LED2,3,4 Brightness Control Functions
void dimLEDs() {
  if (userHasAccessed && isBookingActive && !ledsDimmed) {
    Serial.println("Dimming LED2,3,4 - no motion for 15 seconds");
    // Turn off LEDs to simulate dimming (since ESP32 digitalWrite doesn't support PWM directly)
    setLED234(false);  //  helper function
    ledsDimmed = true;
    Serial.println("LED2,3,4 dimmed (turned off)");
  }
}

void brightenLEDs() {
  if (userHasAccessed && isBookingActive && ledsDimmed) {
    Serial.println("🚶‍♂️ Brightening LED2,3,4 - motion detected");
    setLED234(true);  //  helper function
    ledsDimmed = false;
    Serial.println("LED2,3,4 brightened (turned on)");
  }
}

// Reset LED dimming state when user access ends
void resetLEDDimming() {
  ledsDimmed = false;
  Serial.println("LED dimming state reset");
}

// Smart Display Control Functions
void handleSmartDisplayControl() {
  unsigned long timeSinceMotion = millis() - lastMotionTime;
  
  // Check for screensaver (1 minute no motion)
  if (timeSinceMotion > SCREENSAVER_TIMEOUT && displayActive) {
    Serial.println("No motion for 1 minute - Activating screensaver");
    showScreensaver();
    displayActive = false;
    displayDimmed = false;
  }
  // Check for dimming (30 seconds no motion)
  else if (timeSinceMotion > MOTION_TIMEOUT && displayActive && !displayDimmed) {
    Serial.println("No motion for 30 seconds - Dimming display");
    dimDisplay();
    displayDimmed = true;
  }
  // Keep display active if there was recent motion
  else if (timeSinceMotion <= MOTION_TIMEOUT && (!displayActive || displayDimmed)) {
    Serial.println("Recent motion - Keeping display active");
    activateDisplay();
  }
}

void activateDisplay() {
  displayActive = true;
  displayDimmed = false;
  
  // SH110X doesn't have ssd1306_command - just redraw the screen to activate
  Serial.println("Activating display by redrawing screen");
  
  // Redraw current screen based on state
  if (accessCode.length() > 0) {
    showAccessCodeScreen();
  } else if (doorUnlocked) {
    showWelcomeMessage();
  } else {
    showReadyScreen();
  }
  
  Serial.println("Display activated");
}

void dimDisplay() {
  // Reduce display content to minimal info
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println("Smart Room");
  display.setCursor(0, 35);
  if (isBookingActive) {
    display.println("Status: BOOKED");
  } else {
    display.println("Status: Available");
  }
  display.setCursor(0, 50);
  display.println("Touch keypad to wake");
  display.display();
  
  Serial.println("Display dimmed");
}

void showScreensaver() {
  static int screensaverFrame = 0;
  
  display.clearDisplay();
  
  // Animated border effect
  int borderOffset = screensaverFrame % 8;
  for (int i = 0; i < 128; i += 8) {
    if ((i + borderOffset) % 16 < 8) {
      display.drawPixel(i, 0, SH110X_WHITE);
      display.drawPixel(i, 63, SH110X_WHITE);
    }
  }
  for (int i = 0; i < 64; i += 8) {
    if ((i + borderOffset) % 16 < 8) {
      display.drawPixel(0, i, SH110X_WHITE);
      display.drawPixel(127, i, SH110X_WHITE);
    }
  }
  
  // Floating logo with gentle movement
  int logoX = 40 + sin(screensaverFrame * 0.05) * 15;
  int logoY = 20 + cos(screensaverFrame * 0.03) * 8;
  
  display.setTextSize(2);
  display.setCursor(logoX, logoY);
  display.println("SMART");
  display.setCursor(logoX + 5, logoY + 16);
  display.println("ROOM");
  
  // Pulsing status indicator
  int pulseRadius = 2 + sin(screensaverFrame * 0.2) * 1;
  display.fillCircle(64, 50, pulseRadius, SH110X_WHITE);
  
  // Wake instruction
  display.setTextSize(1);
  display.setCursor(25, 56);
  display.println("Touch to wake up");
  
  display.display();
  
  screensaverFrame++;
  if (screensaverFrame > 200) screensaverFrame = 0;
  
  Serial.println("🌙 Enhanced screensaver active");
}

void updateBookingLEDs() {
  Serial.println(" === updateBookingLEDs() called ===");
  // Serial.println("isBookingActive: " + String(isBookingActive));  // Removed for cleaner output
  // Serial.println("userHasAccessed: " + String(userHasAccessed));  // Removed for cleaner output
  // Serial.println("doorUnlocked: " + String(doorUnlocked));  // Removed for cleaner output
  
  // If user has accessed, DO NOT interfere with LED2-4 
  if (userHasAccessed) {
    // User has accessed - LED2,3,4 control handled elsewhere (debug removed)
    return;
  }
  
  // Normal LED behavior for users who haven't accessed
  if (!isBookingActive || doorUnlocked) {
    // No booking or door is unlocked - turn off booking LEDs
    Serial.println("No booking or door unlocked - LED2,3,4 OFF");
    setLED234(false);  //  helper function
    return;
  }
  
  // Keep LED2-4 off until user accesses
  Serial.println("Waiting for user access - LED2,3,4 OFF");
  setLED234(false);  //  helper function
}

void autoDetectRoomId() {
  Serial.println(" === BACKEND AUTO-DETECTION ===");
  Serial.println("Querying backend API for active bookings...");
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected for room detection");
    return;
  }
  
  String url = String(serverURL) + "/api/bookings/all";
  
  Serial.print("Backend API: ");
  Serial.println(url);
  Serial.println("Backend handles database queries, returns processed data");
  
  HTTPClient http;
  http.setTimeout(10000);  // 10 second timeout
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.GET();
  
  Serial.print("HTTP Response Code: ");
  Serial.println(httpResponseCode);
  

  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Bookings response (first 300 chars): ");
    Serial.println(response.substring(0, 300));
    
    // Parse JSON response
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
      Serial.print("JSON Parse Error: ");
      Serial.println(error.c_str());
      http.end();
      return;
    }
    
    // Look for active booking
    if (doc.is<JsonArray>()) {
      JsonArray bookings = doc.as<JsonArray>();
      
      Serial.print("Found ");
      Serial.print(bookings.size());
      Serial.println("bookings total");
      
      for (JsonObject booking : bookings) {
        String status = booking["status"];
        
        Serial.print("Booking status: ");
        Serial.println(status);
        
        if (status == "active") {
          roomId = booking["room_id"];
          String roomName = booking["room_name"];
          String accessCode = booking["access_code"];
          
          Serial.println("FOUND ACTIVE BOOKING!");
          Serial.print("Room ID: ");
          Serial.println(roomId);
          Serial.print("Room Name: ");
          Serial.println(roomName);
          Serial.print("Access Code: ");
          Serial.println(accessCode);
          
          break;
        }
      }
    } else {
      Serial.println("Response is not an array");
    }
    
    if (roomId > 0) {
      Serial.println("Room ID auto-detected successfully!");
    } else {
      Serial.println("No active booking found. Arduino will wait for booking.");
    }
    
  } else {
    Serial.print("HTTP Error detecting room: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode == -1) {
      Serial.println("Connection failed - check network connectivity");
      Serial.println("DETAILED DIAGNOSIS:");
      Serial.print("    Trying to reach: ");
      Serial.println(url);
      Serial.print("    Arduino is on network: ");
      Serial.println(WiFi.localIP());
      Serial.print("    Arduino gateway: ");
      Serial.println(WiFi.gatewayIP());
      Serial.println("    POSSIBLE SOLUTIONS:");
      Serial.println("   1. Server may have changed ports");
      Serial.println("   2. Ensure computer and Arduino on same WiFi");
      Serial.println("   3. Disable Windows Firewall");
      Serial.println("   4. Use phone WiFi hotspot for both devices");
    }
  }
  
  http.end();
}

void showWiFiConnecting() {
  static int connectFrame = 0;
  
  display.clearDisplay();
  
  // WiFi icon with animation
  display.drawRect(40, 10, 48, 30, SH110X_WHITE);
  display.drawRect(42, 12, 44, 26, SH110X_WHITE);
  
  // Animated WiFi signal bars
  int numBars = (connectFrame / 10) % 4 + 1;
  for (int i = 0; i < numBars; i++) {
    int barHeight = (i + 1) * 4;
    display.fillRect(50 + (i * 8), 30 - barHeight, 6, barHeight, SH110X_WHITE);
  }
  
  // Connection text
  display.setTextSize(1);
  display.setCursor(30, 45);
  display.println("Connecting to WiFi");
  
  // Animated dots
  display.setCursor(45, 55);
  for (int i = 0; i < (connectFrame / 15) % 4; i++) {
    display.print(".");
  }
  
  display.display();
  connectFrame++;
  if (connectFrame > 120) connectFrame = 0;
}

// Display management functions
void refreshDisplay() {
  // Force display refresh to prevent corruption
  display.clearDisplay();
  display.display();
  delay(10);
}

void manageDisplay() {
  // Check if display needs periodic refresh
  if (millis() - lastDisplayUpdate > DISPLAY_REFRESH_INTERVAL) {
    // Only refresh if display is active (not in screensaver mode)
    if (displayActive) {
      preventDisplayCorruption();
    }
    lastDisplayUpdate = millis();
  }
}

void preventDisplayCorruption() {
  // Prevent display corruption by refreshing occasionally
  static int refreshCounter = 0;
  refreshCounter++;
  
  if (refreshCounter > 60) {  // Every 60 seconds
    Serial.println("Refreshing display to prevent corruption");
    refreshDisplay();
    
    // Redraw current screen based on state
    if (accessCode.length() > 0) {
      showAccessCodeScreen();
    } else {
      showReadyScreen();
    }
    
    refreshCounter = 0;
  }
}

void resetDisplay() {
  // Complete display reset
  Serial.println("Resetting OLED display");
  
  // Re-initialize display
  if (display.begin(0x3C, true)) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setTextWrap(false);
    display.display();
    
    Serial.println("Display reset successful");
    showReadyScreen();
  } else {
    Serial.println("Display reset failed");
  }
}

// OLED Display Functions
void showWelcomeScreen() {
  refreshDisplay();
  
  // Draw decorative border
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.drawRect(2, 2, 124, 60, SH110X_WHITE);
  
  // Title with better spacing
  display.setTextSize(2);
  display.setCursor(18, 8);
  display.println("SMART");
  display.setCursor(25, 28);
  display.println("ROOM");
  
  // Subtitle with decorative elements
  display.setTextSize(1);
  display.setCursor(8, 48);
  display.println(">> Controller <<");
  
  // Add decorative dots
  display.fillCircle(10, 52, 1, SH110X_WHITE);
  display.fillCircle(118, 52, 1, SH110X_WHITE);
  
  display.display();
  Serial.println("🎨 Enhanced welcome screen displayed");
}

void showReadyScreen() {
  refreshDisplay();
  
  // Header with border
  display.drawRect(0, 0, 128, 12, SH110X_WHITE);
  display.fillRect(1, 1, 126, 10, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(15, 2);
  display.println("ROOM CONTROLLER");
  
  // Reset text color for content
  display.setTextColor(SH110X_WHITE);
  
  // WiFi status with icon
  display.setCursor(5, 16);
  display.println("WiFi: Connected");
  display.drawRect(110, 15, 8, 6, SH110X_WHITE);
  display.fillRect(112, 17, 4, 2, SH110X_WHITE);
  
  // IP address
  display.setCursor(5, 26);
  display.print("IP: ");
  display.println(WiFi.localIP());
  
  // Status section with visual indicator
  display.drawLine(0, 36, 128, 36, SH110X_WHITE);
  display.setCursor(5, 40);
  if (isBookingActive) {
    display.println("Status: BOOKED");
    display.fillCircle(115, 42, 3, SH110X_WHITE);  // Solid circle for booked
  } else {
    display.println("Status: Available");
    display.drawCircle(115, 42, 3, SH110X_WHITE);  // Empty circle for available
  }
  
  // Instructions at bottom
  display.drawLine(0, 48, 128, 48, SH110X_WHITE);
  display.setCursor(6, 52);
  display.println("# = Submit  * = Clear");
  
  display.display();
}

void showAccessCodeScreen() {
  Serial.println("Showing access code screen...");
  Serial.print("Current code: '");
  Serial.print(accessCode);
  Serial.print("' (length: ");
  Serial.print(accessCode.length());
  Serial.println(")");
  
  refreshDisplay();
  
  // Header with key icon
  display.setTextSize(1);
  display.setCursor(5, 2);
  display.println("ACCESS CODE");
  // Draw key icon
  display.drawRect(100, 0, 6, 8, SH110X_WHITE);
  display.drawRect(106, 2, 4, 4, SH110X_WHITE);
  display.fillRect(108, 3, 2, 2, SH110X_WHITE);
  
  // Code input area with border
  display.drawRect(5, 15, 118, 28, SH110X_WHITE);
  display.drawRect(6, 16, 116, 26, SH110X_WHITE);
  
  // Display code with boxes
  display.setTextSize(2);
  int startX = 10;
  for (int i = 0; i < 6; i++) {
    int boxX = startX + (i * 18);
    display.drawRect(boxX, 20, 16, 20, SH110X_WHITE);  // ขยายกรอบให้ใหญ่ขึ้น
    
    if (i < accessCode.length()) {
      display.setCursor(boxX + 3, 24);  // ปรับตำแหน่งตัวเลขให้อยู่กลางกรอบ
      display.print(accessCode[i]);
    } else if (i == accessCode.length()) {
      // Show blinking cursor
      display.fillRect(boxX + 2, 35, 12, 2, SH110X_WHITE);  // ปรับตำแหน่ง cursor
    }
  }
  
  // Instructions
  display.setTextSize(1);
  display.setCursor(8, 46);
  display.println("Enter 6-digit code");
  display.setCursor(6, 56);
  display.println("# = Submit  * = Clear");
  
  display.display();
  
  Serial.println("Access code screen updated");
}

void showSubmittingScreen() {
  static int loadingFrame = 0;
  
  display.clearDisplay();
  
  // Loading spinner
  int centerX = 64, centerY = 25;
  int radius = 12;
  for (int i = 0; i < 8; i++) {
    int angle = (i * 45 + loadingFrame * 10) % 360;
    int x = centerX + cos(angle * PI / 180) * radius;
    int y = centerY + sin(angle * PI / 180) * radius;
    int dotSize = (i == 0) ? 3 : (i < 3) ? 2 : 1;
    display.fillCircle(x, y, dotSize, SH110X_WHITE);
  }
  
  // Loading text
  display.setTextSize(1);
  display.setCursor(25, 45);
  display.println("Verifying Code");
  
  // Progress bar
  int progress = (loadingFrame / 3) % 100;
  display.drawRect(20, 55, 88, 6, SH110X_WHITE);
  display.fillRect(21, 56, (progress * 86) / 100, 4, SH110X_WHITE);
  
  display.display();
  loadingFrame++;
  if (loadingFrame > 300) loadingFrame = 0;
}

void showWelcomeMessage() {
  refreshDisplay();
  
  // Success border
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.drawRect(2, 2, 124, 60, SH110X_WHITE);
  
  // Success icon (checkmark)
  display.drawLine(20, 28, 25, 33, SH110X_WHITE);
  display.drawLine(25, 33, 35, 20, SH110X_WHITE);
  display.drawLine(21, 28, 26, 33, SH110X_WHITE);
  display.drawLine(26, 33, 36, 20, SH110X_WHITE);
  
  // Success message
  display.setTextSize(2);
  display.setCursor(45, 10);
  display.println("ACCESS");
  display.setCursor(45, 28);
  display.println("GRANTED");
  
  // Status with icon
  display.setTextSize(1);
  display.setCursor(30, 48);
  display.println("Door Unlocked");
  
  // Add decorative elements
  display.fillCircle(10, 50, 2, SH110X_WHITE);
  display.fillCircle(118, 50, 2, SH110X_WHITE);
  
  display.display();
  
  delay(3000);  // Show for 3 seconds
  lastScreenSaver = millis();  // Reset screensaver timer
  accessCode = "";  // Clear access code
  showReadyScreen();  // Return to ready screen
}

void showFailScreen() {
  refreshDisplay();
  
  // Error border (double line for emphasis)
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.drawRect(2, 2, 124, 60, SH110X_WHITE);
  display.drawRect(4, 4, 120, 56, SH110X_WHITE);
  
  // Error icon (X mark)
  display.drawLine(15, 20, 30, 35, SH110X_WHITE);
  display.drawLine(30, 20, 15, 35, SH110X_WHITE);
  display.drawLine(16, 20, 31, 35, SH110X_WHITE);
  display.drawLine(31, 20, 16, 35, SH110X_WHITE);
  
  // Error message
  display.setTextSize(2);
  display.setCursor(40, 10);
  display.println("ACCESS");
  display.setCursor(48, 28);
  display.println("DENIED");
  
  // Status message
  display.setTextSize(1);
  display.setCursor(30, 48);
  display.println("Invalid Code");
  display.setCursor(35, 56);
  display.println("Try Again");
  
  display.display();
  
  delay(3000);  // Show for 3 seconds
  lastScreenSaver = millis();  // Reset screensaver timer
  accessCode = "";  // Clear the code
  showReadyScreen();  // Return to ready screen
}

void showErrorScreen(String message) {
  refreshDisplay();
  display.setTextSize(2);
  display.setCursor(15, 10);
  display.println("ERROR");
  
  display.setTextSize(1);
  display.setCursor(5, 35);
  display.println(message);
  display.display();
  
  delay(2000);  // Show for 2 seconds
  lastScreenSaver = millis();  // Reset screensaver timer
  showAccessCodeScreen();  // Return to access code screen
}



