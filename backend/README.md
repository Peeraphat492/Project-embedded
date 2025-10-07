# Smart Room Reservation Backend

Backend API สำหรับระบบจองห้องประชุมอัจฉริยะ (Smart Room Reservation System)

## คุณสมบัติ

- 🔐 **Authentication & Authorization** - ระบบล็อกอิน JWT
- 🏢 **Room Management** - จัดการข้อมูลห้องประชุม
- 📅 **Booking System** - ระบบจองห้องแบบเรียลไทม์
- ⏰ **Time Slot Management** - ตรวจสอบเวลาว่างของห้อง
- 🔧 **IoT Integration** - รองรับการเชื่อมต่อกับ ESP32
- 📊 **SQLite Database** - ฐานข้อมูลที่เบาและรวดเร็ว
- 🛡️ **Rate Limiting** - ป้องกันการใช้งานเกินขีดจำกัด

## การติดตั้ง

### ข้อกำหนดเบื้องต้น

- Node.js (เวอร์ชัน 16 หรือใหม่กว่า)
- npm หรือ yarn

### ขั้นตอนการติดตั้ง

1. **Clone หรือดาวน์โหลดโปรเจค**
   ```bash
   cd backend
   ```

2. **ติดตั้ง dependencies**
   ```bash
   npm install
   ```

3. **ตั้งค่า environment variables**
   ```bash
   cp .env.example .env
   # แก้ไขไฟล์ .env ตามต้องการ
   ```

4. **เริ่มต้นเซิร์ฟเวอร์**
   ```bash
   # โหมด development
   npm run dev
   
   # โหมด production
   npm start
   ```

5. **ทดสอบการเชื่อมต่อ**
   ```bash
   curl http://localhost:3000/api/health
   ```

## API Endpoints

### Authentication

#### POST `/api/auth/login`
ล็อกอินเข้าสู่ระบบ

**Request Body:**
```json
{
  "username": "admin",
  "password": "1234"
}
```

**Response:**
```json
{
  "success": true,
  "token": "jwt_token_here",
  "user": {
    "id": 1,
    "username": "admin"
  }
}
```

### Rooms

#### GET `/api/rooms`
ดึงข้อมูลห้องทั้งหมด

**Response:**
```json
[
  {
    "id": 1,
    "name": "Meeting Room1",
    "description": "ห้องประชุมขนาดกลาง",
    "capacity": 8,
    "image_url": "...",
    "status": "available"
  }
]
```

#### GET `/api/rooms/:id`
ดึงข้อมูลห้องตาม ID

#### GET `/api/rooms/:id/availability/:date`
ตรวจสอบเวลาว่างของห้องในวันที่กำหนด

**Parameters:**
- `id`: ID ของห้อง
- `date`: วันที่ในรูปแบบ YYYY-MM-DD

**Response:**
```json
[
  {
    "start": "08:00",
    "end": "09:00"
  },
  {
    "start": "10:00", 
    "end": "11:00"
  }
]
```

### Bookings

#### POST `/api/bookings`
สร้างการจองใหม่

**Headers:**
```
Authorization: Bearer jwt_token_here
```

**Request Body:**
```json
{
  "room_id": 1,
  "booking_date": "2024-01-15",
  "start_time": "09:00",
  "end_time": "10:00"
}
```

**Response:**
```json
{
  "success": true,
  "booking": {
    "id": 1,
    "access_code": "123456",
    "room_name": "Meeting Room1",
    "booking_date": "2024-01-15",
    "start_time": "09:00",
    "end_time": "10:00"
  }
}
```

#### GET `/api/bookings/my`
ดึงข้อมูลการจองของผู้ใช้ปัจจุบัน

**Headers:**
```
Authorization: Bearer jwt_token_here
```

#### DELETE `/api/bookings/:id`
ยกเลิกการจอง

### IoT Device Integration

#### GET `/api/device/settings`
ดึงการตั้งค่าปัจจุบันของอุปกรณ์

#### POST `/api/device/control`
ส่งคำสั่งควบคุมอุปกรณ์

**Request Body:**
```json
{
  "action": "turn_on",
  "timeRange": "09:00-11:00"
}
```

### Health Check

#### GET `/api/health`
ตรวจสอบสถานะของเซิร์ฟเวอร์

## ฐานข้อมูล

ระบบใช้ SQLite เป็นฐานข้อมูล โดยมี 3 ตารางหลัก:

### Users
- `id` - Primary key
- `username` - ชื่อผู้ใช้ (unique)
- `password` - รหัสผ่าน (hashed)
- `email` - อีเมล
- `created_at` - วันที่สร้าง

### Rooms
- `id` - Primary key
- `name` - ชื่อห้อง
- `description` - รายละเอียด
- `capacity` - ความจุ
- `image_url` - รูปภาพ
- `amenities` - สิ่งอำนวยความสะดวก
- `status` - สถานะ (available/maintenance)

### Bookings
- `id` - Primary key
- `user_id` - Foreign key ไปยัง users
- `room_id` - Foreign key ไปยัง rooms
- `booking_date` - วันที่จอง
- `start_time` - เวลาเริ่มต้น
- `end_time` - เวลาสิ้นสุด
- `access_code` - รหัสเข้าใช้ห้อง (6 หลัก)
- `status` - สถานะ (active/cancelled)

## การพัฒนา

### Scripts ที่มีให้ใช้

```bash
npm start        # เริ่มเซิร์ฟเวอร์ production
npm run dev      # เริ่มเซิร์ฟเวอร์ development (auto-reload)
npm test         # รันการทดสอบ
```

### โครงสร้างโฟลเดอร์

```
backend/
├── server.js           # ไฟล์หลักของเซิร์ฟเวอร์
├── package.json        # ข้อมูลโปรเจคและ dependencies
├── .env               # การตั้งค่า environment variables
├── database.db        # ไฟล์ฐานข้อมูล SQLite (สร้างอัตโนมัติ)
└── README.md          # เอกสารนี้
```

### การเชื่อมต่อกับ Frontend

เพื่อให้ frontend เชื่อมต่อกับ backend ได้ ให้แก้ไขไฟล์ JavaScript ใน frontend:

```javascript
const API_BASE_URL = 'http://localhost:3000/api';

// ตอนล็อกอิน
fetch(`${API_BASE_URL}/auth/login`, {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json'
  },
  body: JSON.stringify({
    username: 'admin',
    password: '1234'
  })
})
.then(response => response.json())
.then(data => {
  if (data.success) {
    localStorage.setItem('token', data.token);
    // ไปหน้าถัดไป
  }
});
```

## การใช้งานกับ ESP32

Backend นี้รองรับการเชื่อมต่อกับ ESP32 สำหรับควบคุมอุปกรณ์ IoT:

```cpp
// ตัวอย่างโค้ด ESP32
#include <WiFi.h>
#include <HTTPClient.h>

const char* serverURL = "http://your-server-ip:3000/api";

void checkBookingStatus() {
  HTTPClient http;
  http.begin(serverURL + String("/device/settings"));
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    // ประมวลผลข้อมูลการจอง
  }
  
  http.end();
}
```

## ความปลอดภัย

- 🔐 Password hashing ด้วย bcryptjs
- 🎫 JWT authentication
- 🚦 Rate limiting
- 🛡️ CORS protection
- 🔒 Environment variables สำหรับข้อมูลสำคัญ

## การ Deploy

### Local Development
```bash
npm run dev
```

### Production
```bash
npm start
```

### Docker (Optional)
```dockerfile
FROM node:16-alpine
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
EXPOSE 3000
CMD ["npm", "start"]
```

## การแก้ไขปัญหา

### ปัญหาที่พบบ่อย

1. **ฐานข้อมูลล็อก**
   ```bash
   rm database.db
   npm start  # ฐานข้อมูลใหม่จะถูกสร้างอัตโนมัติ
   ```

2. **Port ถูกใช้งานแล้ว**
   ```bash
   # เปลี่ยน PORT ในไฟล์ .env
   PORT=3001
   ```

3. **CORS Error**
   ```bash
   # เพิ่ม origin ใน ALLOWED_ORIGINS ในไฟล์ .env
   ```

## การสนับสนุน

หากพบปัญหาหรือต้องการความช่วยเหลือ สามารถ:
- สร้าง Issue ในโปรเจค
- ตรวจสอบ logs ในคอนโซล
- ใช้ health check endpoint เพื่อทดสอบการเชื่อมต่อ

## License

MIT License