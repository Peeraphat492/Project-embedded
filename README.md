# 🏠 Smart Room Reservation System

ระบบจองห้องประชุมอัจฉริยะ - พร้อมใช้งานทันที!

## 🚀 Quick Start (สำหรับเพื่อนร่วมงาน)

### 1. Clone โปรเจค
```bash
git clone https://github.com/Peeraphat492/Project-embedded.git
cd Project-embedded
```

### 2. Install Dependencies
```bash
npm install
```

### 3. เริ่มใช้งาน
```bash
npm start
```

จบแล้ว! 🎉 เปิด browser ไปที่ `http://localhost:3000`

---

## 📱 การใช้งาน

### เว็บไซต์
- **หน้าหลัก**: `http://localhost:3000`
- **Smart Room (ProjectA)**: เปิดไฟล์ `ProjectA/login.html`
- **API Testing**: `http://localhost:3000/api/health`

### GitHub Pages (Live Demo)
🌐 [https://peeraphat492.github.io/Project-embedded/ProjectA/login.html](https://peeraphat492.github.io/Project-embedded/ProjectA/login.html)

---

## 📁 โครงสร้างโปรเจค

```
Project-embedded/
├── 📁 ProjectA/          # Smart Room Reservation UI
│   ├── login.html        # หน้า Login
│   ├── rooms.html        # เลือกห้อง  
│   ├── time.html         # เลือกเวลา
│   ├── summary.html      # สรุปการจอง
│   └── manual.html       # คู่มือใช้งาน
├── 📁 backend/           # Server & API
│   ├── server.js         # Main server
│   ├── package.json      # Dependencies
│   └── public/           # Additional frontend
├── 📄 package.json       # Root package (สำหรับ npm start)
└── 📄 README.md          # คู่มือนี้
```

---

## 🛠️ Commands ที่ใช้บ่อย

| Command | คำอธิบาย |
|---------|----------|
| `npm start` | เริ่ม server (port 3000) |
| `npm run dev` | เริ่ม server แบบ development |
| `npm install` | ติดตั้ง dependencies |

---

## 🎯 Features

- ✅ ระบบ Login/Authentication
- ✅ เลือกห้องประชุม  
- ✅ จองเวลาใช้งาน
- ✅ สร้าง Access Code
- ✅ Database SQLite
- ✅ REST API
- ✅ รองรับ GitHub Pages

---

## 🚨 หากมีปัญหา

1. **npm start ไม่ได้**: ลองรัน `npm install` อีกครั้ง
2. **Port 3000 ถูกใช้**: ปิด server เก่าหรือเปลี่ยน port
3. **Database error**: ลบไฟล์ `backend/database.db` แล้วรัน server ใหม่

---

**Happy Coding! 🎉**