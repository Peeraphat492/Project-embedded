const express = require('express');
const app = express();

// Enable CORS for all origins
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept, Authorization');
  next();
});

app.use(express.json());

// Simple test endpoint
app.get('/test', (req, res) => {
  console.log(`📍 Test request from: ${req.ip}`);
  res.json({
    success: true,
    message: 'Arduino connection test successful!',
    timestamp: new Date().toISOString(),
    clientIP: req.ip,
    serverIP: '0.0.0.0'
  });
});

// Arduino status endpoint
app.get('/api/arduino/status/:roomId', (req, res) => {
  const roomId = req.params.roomId;
  console.log(`📍 Arduino status request for room ${roomId} from: ${req.ip}`);
  
  res.json({
    roomId: parseInt(roomId),
    isBooked: true,
    roomStatus: "available",
    currentBooking: {
      bookingId: 10,
      userId: 1,
      startTime: "10:00",
      endTime: "11:00",
      accessCode: "661176"
    },
    timestamp: new Date().toISOString()
  });
});

// Listen on all interfaces
const PORT = 3002;
app.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Test server running on port ${PORT}`);
  console.log(`🌍 Server accessible on ALL network interfaces:`);
  console.log(`   - http://localhost:${PORT}/test`);
  console.log(`   - http://10.211.55.3:${PORT}/test`);
  console.log(`   - http://100.86.13.13:${PORT}/test`);
  console.log(`   - http://[ANY-IP]:${PORT}/test`);
  console.log(`\n📱 Arduino test URLs:`);
  console.log(`   - http://10.211.55.3:${PORT}/api/arduino/status/6`);
  console.log(`   - http://100.86.13.13:${PORT}/api/arduino/status/6`);
});