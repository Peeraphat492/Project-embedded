# การติดตั้งและเริ่มต้น Backend

## ขั้นตอนการติดตั้ง Node.js

### 1. ดาวน์โหลดและติดตั้ง Node.js

1. ไปที่ https://nodejs.org/
2. ดาวน์โหลด **LTS Version** (แนะนำ)
3. รันไฟล์ installer และติดตั้งตามขั้นตอน
4. เลือก "Add to PATH" ในขั้นตอนการติดตั้ง

### 2. ตรวจสอบการติดตั้ง

เปิด Command Prompt หรือ PowerShell ใหม่ และรันคำสั่ง:

```cmd
node --version
npm --version
```

หากแสดงเลขเวอร์ชัน แสดงว่าติดตั้งสำเร็จแล้ว

### 3. ติดตั้ง Dependencies

```cmd
cd "z:\\Desktop\\Project embedded\\backend"
npm install
```

### 4. เริ่มต้นเซิร์ฟเวอร์

#### วิธีที่ 1: ใช้ไฟล์ .bat
```cmd
start.bat
```

#### วิธีที่ 2: ใช้ npm command
```cmd
npm start
```

#### วิธีที่ 3: Development mode (auto-reload)
```cmd
npm run dev
```

## การทดสอบ Backend

หลังจากเริ่มเซิร์ฟเวอร์แล้ว:

1. เปิดเบราว์เซอร์ไปที่ http://localhost:3000/api/health
2. ควรเห็นข้อความ:
   ```json
   {
     "status": "OK",
     "timestamp": "...",
     "database": "Connected"
   }
   ```

## การใช้งานกับ Frontend

1. เริ่มต้น backend server
2. เปิดไฟล์ HTML ใน ProjectA folder
3. ระบบจะเชื่อมต่อกับ backend โดยอัตโนมัติ

## URLs สำคัญ

- **Backend API**: http://localhost:3000/api
- **Health Check**: http://localhost:3000/api/health
- **Database**: ./database.db (สร้างอัตโนมัติ)

## Default Login

- **Username**: admin
- **Password**: 1234

## การแก้ไขปัญหา

### ปัญหา: npm command not found
- ติดตั้ง Node.js ใหม่และรีสตาร์ท terminal
- ตรวจสอบว่า Node.js อยู่ใน PATH

### ปัญหา: Port 3000 ถูกใช้แล้ว
- เปลี่ยน PORT ในไฟล์ .env
- หรือหยุดกระบวนการที่ใช้ port 3000

### ปัญหา: Database locked
- ปิดเซิร์ฟเวอร์ทั้งหมด
- ลบไฟล์ database.db และเริ่มใหม่

## โครงสร้างไฟล์

```
backend/
├── server.js          # เซิร์ฟเวอร์หลัก
├── package.json       # การตั้งค่าโปรเจค
├── .env              # การตั้งค่าสภาพแวดล้อม
├── database.db       # ฐานข้อมูล SQLite (สร้างอัตโนมัติ)
├── start.bat         # ไฟล์เริ่มต้นสำหรับ Windows
├── README.md         # เอกสารการใช้งาน
└── public/
    └── api.js        # ไลบรารี่สำหรับ frontend
```