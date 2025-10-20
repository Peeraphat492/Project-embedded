# Arduino Integration Guide
## Smart Room Reservation System

การเชื่อมต่อระบบ Smart Room Reservation กับ Arduino ESP32 สำหรับควบคุมการเข้าถึงห้อง

## 📋 รายการอุปกรณ์ที่ต้องใช้

### Hardware Components:
- **ESP32 Development Board** (1 ชิ้น)
- **4x4 Matrix Keypad** (1 ชิ้น) - สำหรับใส่ access code
- **Relay Module 5V** (1 ชิ้น) - ควบคุมกลอนประตู
- **LED RGB หรือ LED 3 สี** (3 ดวง) - แสดงสถานะ
- **PIR Motion Sensor** (1 ชิ้น) - ตรวจจับการเคลื่อนไหว
- **Buzzer** (1 ชิ้น) - เสียงแจ้งเตือน
- **Breadboard หรือ PCB** สำหรับเชื่อมต่อ
- **Jumper Wires** สำหรับเชื่อมต่อ
- **Power Supply 5V** สำหรับ Arduino และ Relay

### Software Requirements:
- **Arduino IDE** พร้อม ESP32 Board Package
- **Libraries ที่ต้องติดตั้ง:**
  - WiFi (Built-in)
  - HTTPClient (Built-in)
  - ArduinoJson (ติดตั้งผ่าน Library Manager)
  - Keypad (ติดตั้งผ่าน Library Manager)

## 🔌 การเชื่อมต่อ Hardware

### ESP32 Pin Connections:

```
Keypad (4x4 Matrix):
- Row 1 → GPIO 19
- Row 2 → GPIO 18  
- Row 3 → GPIO 17
- Row 4 → GPIO 16
- Col 1 → GPIO 15
- Col 2 → GPIO 14
- Col 3 → GPIO 13
- Col 4 → GPIO 12

Relay Module:
- VCC → 5V
- GND → GND
- IN  → GPIO 23

LED Indicators:
- Status LED (Blue)  → GPIO 2
- Error LED (Red)    → GPIO 4
- Success LED (Green)→ GPIO 5

Motion Sensor (PIR):
- VCC → 5V
- GND → GND
- OUT → GPIO 21

Buzzer:
- Positive → GPIO 22
- Negative → GND
```

### 📐 Wiring Diagram:

```
ESP32          Keypad (4x4)
GPIO 19    ←→  Row 1
GPIO 18    ←→  Row 2
GPIO 17    ←→  Row 3
GPIO 16    ←→  Row 4
GPIO 15    ←→  Col 1
GPIO 14    ←→  Col 2
GPIO 13    ←→  Col 3
GPIO 12    ←→  Col 4

ESP32          Other Components
GPIO 23    ←→  Relay IN
GPIO 2     ←→  Status LED
GPIO 4     ←→  Error LED
GPIO 5     ←→  Success LED
GPIO 21    ←→  PIR Sensor OUT
GPIO 22    ←→  Buzzer
5V         ←→  Relay VCC, PIR VCC
GND        ←→  All GND connections
```

## ⚙️ การติดตั้งและตั้งค่า

### 1. ติดตั้ง Arduino IDE และ Libraries:

```bash
# ใน Arduino IDE:
# 1. ไปที่ File → Preferences
# 2. เพิ่ม URL ในช่อง "Additional Board Manager URLs":
https://dl.espressif.com/dl/package_esp32_index.json

# 3. ไปที่ Tools → Board → Board Manager
# 4. ค้นหาและติดตั้ง "ESP32"

# 5. ติดตั้ง Libraries:
# Tools → Manage Libraries
# - ค้นหาและติดตั้ง "ArduinoJson"
# - ค้นหาและติดตั้ง "Keypad"
```

### 2. แก้ไขการตั้งค่าใน Arduino Code:

แก้ไขไฟล์ `smart_room_controller.ino`:

```cpp
// WiFi Configuration - แก้ไขตาม WiFi ของคุณ
const char* ssid = "YOUR_WIFI_SSID";        // ชื่อ WiFi
const char* password = "YOUR_WIFI_PASSWORD"; // รหัสผ่าน WiFi

// Server Configuration - แก้ไข IP ของ Server
const char* serverURL = "http://192.168.1.100:3000";  // IP ของเครื่อง Server
const int roomId = 1;  // เลขห้องที่ Arduino ควบคุม (1-6)
```

### 3. Upload Code to ESP32:

```bash
# 1. เชื่อมต่อ ESP32 กับคอมพิวเตอร์ผ่าน USB
# 2. ใน Arduino IDE:
#    - เลือก Board: ESP32 Dev Module
#    - เลือก Port: COM port ที่ ESP32 เชื่อมต่อ
#    - กด Upload
```

## 🚀 การใช้งาน

### การใช้งานผ่าน Keypad:

**Basic Operations:**
- `1-9, 0` → ใส่รหัสเข้าถึง (access code)
- `*` → ลบรหัสที่ใส่
- `#` → ยืนยันรหัสเข้าถึง
- `A` → ตรวจสอบสถานะห้อง
- `B` → ปลดล็อคแบบ Manual (สำหรับ Admin)
- `C` → ล็อคประตู
- `D` → Check-out จากห้อง

**การใช้งานปกติ:**
1. ผู้ใช้จองห้องผ่านเว็บไซต์
2. ได้รับ Access Code (เช่น 123456)
3. ไปที่ห้องและกดรหัสใน Keypad: `1` `2` `3` `4` `5` `6`
4. กด `#` เพื่อยืนยัน
5. ประตูจะปลดล็อคและมีเสียงบีบ
6. เข้าไปในห้องและประตูจะล็อคอัตโนมัติหลังจาก 10 วินาที

### การทำงานของ LED:

- **LED สีน้ำเงิน (Status):** กะพริบเมื่อระบบทำงานปกติ
- **LED สีเขียว (Success):** สว่างเมื่อปลดล็อคสำเร็จ
- **LED สีแดง (Error):** กะพริบเมื่อมีข้อผิดพลาด

### การทำงานของ Motion Sensor:

- ตรวจจับการเคลื่อนไหวในห้อง
- อัปเดตสถานะเป็น "Occupied" เมื่อมีคนในห้อง
- Auto check-out หากไม่มีการเคลื่อนไหวเป็นเวลา 5 นาที

## 🔗 API Integration

Arduino เชื่อมต่อกับ Backend Server ผ่าน HTTP REST API:

### API Endpoints ที่ Arduino ใช้:

1. **GET /api/arduino/status/{roomId}**
   - ตรวจสอบสถานะห้องปัจจุบัน
   - ได้ข้อมูล: การจอง, สถานะห้อง, รหัสเข้าถึง

2. **POST /api/arduino/unlock/{roomId}**
   - ปลดล็อคห้องด้วย access code
   - ส่งข้อมูล: access code, user ID

3. **POST /api/arduino/checkin/{roomId}**
   - บันทึกการเข้าใช้ห้อง
   - อัปเดตสถานะเป็น "occupied"

4. **POST /api/arduino/checkout/{roomId}**
   - บันทึกการออกจากห้อง
   - อัปเดตสถานะเป็น "available"

### ตัวอย่าง API Response:

```json
// GET /api/arduino/status/1
{
  "roomId": 1,
  "isBooked": true,
  "roomStatus": "available",
  "currentBooking": {
    "bookingId": 123,
    "userId": 1,
    "startTime": "09:00",
    "endTime": "10:00",
    "accessCode": "123456"
  },
  "timestamp": "2025-10-17T10:30:00Z"
}
```

## 🌐 Web Integration

### การใช้งานผ่าน Web Interface:

**หน้า Rooms (rooms.html):**
- แสดงสถานะ Arduino ของแต่ละห้อง
- LED แสดงสถานะ: 🟢 Online, 🔴 Offline, 🟡 Unknown

**หน้า Summary (summary.html):**
- ปุ่มควบคุม Arduino
- ช่องใส่ Access Code (auto-fill จากการจอง)
- ปุ่ม Unlock, Check-in, Check-out
- แสดงสถานะ Real-time

### การใช้ Arduino Controller Library:

```javascript
// สร้าง Arduino Controller
const arduino = new ArduinoController();
arduino.setRoomId(1);

// ตรวจสอบสถานะ
arduino.getRoomStatus().then(status => {
  console.log('Room status:', status);
});

// ปลดล็อคห้อง
arduino.unlockRoom('123456').then(result => {
  console.log('Unlock successful:', result);
});

// เริ่ม Status Polling
arduino.startStatusPolling(30000); // ทุก 30 วินาที
```

## 🔧 Troubleshooting

### ปัญหาที่พบบ่อย:

**1. ESP32 เชื่อมต่อ WiFi ไม่ได้:**
- ตรวจสอบชื่อ WiFi และรหัสผ่าน
- ตรวจสอบสัญญาณ WiFi
- ลองรีสตาร์ท ESP32

**2. ไม่สามารถเชื่อมต่อ Server:**
- ตรวจสอบ IP Address ของ Server
- ตรวจสอบว่า Server เปิดอยู่บน Port 3000
- ตรวจสอบ Firewall

**3. Keypad ไม่ทำงาน:**
- ตรวจสอบการเชื่อมต่อสาย
- ตรวจสอบ Pin Configuration
- ทดสอบด้วย Multimeter

**4. Relay ไม่ทำงาน:**
- ตรวจสอบไฟเลี้ยง 5V
- ตรวจสอบสัญญาณ Control จาก GPIO 23
- ตรวจสอบการเชื่อมต่อ Load

### คำสั่ง Debug ใน Serial Monitor:

```bash
# เปิด Serial Monitor ใน Arduino IDE
# Baud Rate: 115200
# จะแสดงข้อมูล Debug เช่น:

🚀 Smart Room Controller Starting...
🌐 Connecting to WiFi...
✅ WiFi Connected!
📱 IP address: 192.168.1.150
⌨️  Key pressed: 1
🔢 Access code: *
🔐 Submitting access code: 123456
📡 Checking status: http://192.168.1.100:3000/api/arduino/status/1
✅ Access granted!
🔓 Unlocking door...
```

## 📖 การขยายระบบ

### ฟีเจอร์เพิ่มเติมที่สามารถพัฒนาได้:

1. **RFID Card Access** - ใช้บัตร RFID แทน Keypad
2. **Fingerprint Scanner** - ใช้ลายนิ้วมือเพื่อเข้าถึง
3. **Camera Integration** - บันทึกวิดีโอเมื่อมีการเข้าถึง
4. **Temperature/Humidity Monitoring** - ตรวจสอบสภาพแวดล้อมในห้อง
5. **Voice Announcement** - แจ้งเสียงเมื่อมีการใช้งาน
6. **Mobile App Control** - ควบคุมผ่าน Mobile App
7. **Emergency Override** - ระบบฉุกเฉินสำหรับ Admin

### การปรับแต่งเพิ่มเติม:

```cpp
// ปรับเวลาการทำงาน
const unsigned long UNLOCK_DURATION = 15000;  // เปลี่ยนจาก 10 เป็น 15 วินาที
const unsigned long STATUS_CHECK_INTERVAL = 60000;  // เช็คสถานะทุก 1 นาที

// เพิ่มห้องใหม่
const int roomId = 7;  // ห้องใหม่

// ปรับแต่งเสียง Buzzer
void customBeep() {
  for(int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}
```

## 🛡️ Security Considerations

### ความปลอดภัย:

1. **Access Code Encryption** - รหัสผ่านถูกส่งผ่าน HTTPS
2. **WiFi Security** - ใช้ WPA2/WPA3 สำหรับ WiFi
3. **Physical Security** - ติดตั้ง Arduino ในตู้ล็อคได้
4. **Code Obfuscation** - ซ่อนรหัสผ่านใน Source Code
5. **Regular Updates** - อัปเดต Firmware เป็นประจำ

---

## 📞 Support

หากมีปัญหาการใช้งาน สามารถติดต่อได้ที่:
- GitHub Issues: [Project Repository]
- Email: support@smartroom.local
- Documentation: [Online Docs]

**Happy Coding! 🚀**