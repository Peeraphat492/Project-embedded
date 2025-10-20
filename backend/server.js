const express = require('express');
const cors = require('cors');
const sqlite3 = require('sqlite3').verbose();
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const rateLimit = require('express-rate-limit');
const path = require('path');
const net = require('net');
require('dotenv').config();

// Set timezone to Thailand (UTC+7)
process.env.TZ = 'Asia/Bangkok';

const app = express();
let PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'your-secret-key-here';

// Function to check if port is available
function isPortAvailable(port) {
  return new Promise((resolve) => {
    const server = net.createServer();
    
    server.listen(port, () => {
      server.once('close', () => {
        resolve(true);
      });
      server.close();
    });
    
    server.on('error', () => {
      resolve(false);
    });
  });
}

// Function to find available port
async function findAvailablePort(startPort = 3000, maxPort = 3010) {
  for (let port = startPort; port <= maxPort; port++) {
    if (await isPortAvailable(port)) {
      return port;
    }
  }
  throw new Error(`No available ports found between ${startPort} and ${maxPort}`);
}

// Function to kill process using port
function killProcessOnPort(port) {
  return new Promise((resolve) => {
    const { exec } = require('child_process');
    
    // Windows command to find and kill process
    exec(`netstat -ano | findstr :${port}`, (error, stdout) => {
      if (error || !stdout) {
        resolve(false);
        return;
      }
      
      const lines = stdout.split('\n');
      const pidMatch = lines[0].match(/\s+(\d+)\s*$/);
      
      if (pidMatch) {
        const pid = pidMatch[1];
        exec(`taskkill /PID ${pid} /F`, (killError) => {
          if (killError) {
            console.log(`⚠️  Could not kill process ${pid}:`, killError.message);
            resolve(false);
          } else {
            console.log(`✅ Killed process ${pid} using port ${port}`);
            resolve(true);
          }
        });
      } else {
        resolve(false);
      }
    });
  });
}

// Middleware
app.use(cors({
  origin: ['http://localhost:3000', 'http://localhost:3001', 'http://localhost:3002', 'https://peeraphat492.github.io', 'https://*.vercel.app'],
  credentials: true
}));
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// Rate limiting - Increased for Arduino devices
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 300 // limit each IP to 300 requests per windowMs (increased for multiple Arduino)
});
app.use('/api/', limiter);

// Request logging middleware
app.use((req, res, next) => {
  console.log(`📡 ${req.method} ${req.url} from ${req.ip}`);
  next();
});

// Database initialization with enhanced error handling
const dbPath = process.env.NODE_ENV === 'production' ? ':memory:' : path.join(__dirname, 'database.db');
console.log(`🗃️  Database path: ${dbPath}`);

let db;
let dbInitialized = false;

function initializeDBConnection() {
  return new Promise((resolve, reject) => {
    db = new sqlite3.Database(dbPath, (err) => {
      if (err) {
        console.error('❌ Error opening database:', err);
        reject(err);
      } else {
        console.log(`✅ Connected to SQLite database: ${dbPath}`);
        
        // Set database pragmas for better performance and reliability
        db.serialize(() => {
          db.run("PRAGMA foreign_keys = ON");
          db.run("PRAGMA journal_mode = WAL");
          db.run("PRAGMA synchronous = NORMAL");
          db.run("PRAGMA cache_size = 10000");
          db.run("PRAGMA temp_store = memory");
        });
        
        initializeDatabase()
          .then(() => {
            dbInitialized = true;
            resolve(db);
          })
          .catch(reject);
      }
    });
  });
}

// Enhanced database initialization
function initializeDatabase() {
  return new Promise((resolve, reject) => {
    console.log('🔧 Initializing database tables...');
    
    db.serialize(() => {
      // Users table
      db.run(`CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT UNIQUE NOT NULL,
        password TEXT NOT NULL,
        email TEXT,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
      )`, (err) => {
        if (err) {
          console.error('❌ Error creating users table:', err);
          reject(err);
          return;
        }
        console.log('✅ Users table ready');
      });

      // Rooms table
      db.run(`CREATE TABLE IF NOT EXISTS rooms (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        status TEXT DEFAULT 'available',
        image_url TEXT,
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP
      )`, (err) => {
        if (err) {
          console.error('❌ Error creating rooms table:', err);
          reject(err);
          return;
        }
        console.log('✅ Rooms table ready');
      });

      // Bookings table  
      db.run(`CREATE TABLE IF NOT EXISTS bookings (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER NOT NULL,
        room_id INTEGER NOT NULL,
        date TEXT NOT NULL,
        start_time TEXT NOT NULL,
        end_time TEXT NOT NULL,
        access_code TEXT NOT NULL,
        status TEXT DEFAULT 'active',
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (user_id) REFERENCES users (id),
        FOREIGN KEY (room_id) REFERENCES rooms (id)
      )`, (err) => {
        if (err) {
          console.error('❌ Error creating bookings table:', err);
          reject(err);
          return;
        }
        console.log('✅ Bookings table ready');
        
        // Initialize default data after all tables are created
        initializeDefaultData()
          .then(() => {
            console.log('🎉 Database initialization completed successfully');
            resolve();
          })
          .catch(reject);
      });
    });
  });
}

// Initialize default data
function initializeDefaultData() {
  return new Promise((resolve, reject) => {
    // Create default admin user
    const defaultPassword = '1234';
    bcrypt.hash(defaultPassword, 10, (err, hashedPassword) => {
      if (err) {
        console.error('❌ Error hashing password:', err);
        reject(err);
        return;
      }

      db.get('SELECT COUNT(*) as count FROM users', (err, row) => {
        if (err) {
          console.error('❌ Error checking users:', err);
          reject(err);
          return;
        }

        if (row.count === 0) {
          db.run(`INSERT INTO users (username, password) VALUES (?, ?)`,
            ['admin', hashedPassword], (err) => {
            if (err) {
              console.error('❌ Error creating admin user:', err);
              reject(err);
              return;
            }
            console.log('✅ Default admin user created (admin/1234)');
          });
        }
      });
    });

    // Create default rooms
    const defaultRooms = [
      { 
        name: 'Game Room', 
        image_url: 'https://via.placeholder.com/300x200/4CAF50/white?text=Game+Room'
      },
      { 
        name: 'Meeting Room1', 
        image_url: 'https://via.placeholder.com/300x200/2196F3/white?text=Meeting+Room+1'
      },
      { 
        name: 'Meeting Room2', 
        image_url: 'https://via.placeholder.com/300x200/FF9800/white?text=Meeting+Room+2'
      },
      { 
        name: 'Cooking Room', 
        image_url: 'https://via.placeholder.com/300x200/E91E63/white?text=Cooking+Room'
      },
      { 
        name: 'Facial Sauna Room', 
        image_url: 'https://via.placeholder.com/300x200/9C27B0/white?text=Sauna+Room'
      },
      { 
        name: 'Karaoke Room', 
        image_url: 'https://via.placeholder.com/300x200/FF5722/white?text=Karaoke+Room'
      }
    ];

    db.get('SELECT COUNT(*) as count FROM rooms', (err, row) => {
      if (err) {
        console.error('❌ Error checking rooms:', err);
        reject(err);
        return;
      }

      if (row.count === 0) {
        let insertedCount = 0;
        defaultRooms.forEach((room) => {
          db.run(`INSERT INTO rooms (name, image_url) VALUES (?, ?)`,
            [room.name, room.image_url], (err) => {
            if (err) {
              console.error('❌ Error inserting room:', err);
              reject(err);
              return;
            }
            
            insertedCount++;
            if (insertedCount === defaultRooms.length) {
              console.log('✅ Default rooms inserted successfully');
              resolve();
            }
          });
        });
      } else {
        console.log(`📊 Found ${row.count} existing rooms in database`);
        resolve();
      }
    });
  });
}

// Middleware for authentication
function authenticateToken(req, res, next) {
  const authHeader = req.headers['authorization'];
  const token = authHeader && authHeader.split(' ')[1];

  if (!token) {
    return res.status(401).json({ error: 'Access token required' });
  }

  jwt.verify(token, JWT_SECRET, (err, user) => {
    if (err) {
      return res.status(403).json({ error: 'Invalid token' });
    }
    req.user = user;
    next();
  });
}

// API Routes

// Health check
app.get('/api/health', (req, res) => {
  console.log('🏥 Health check requested from:', req.ip);
  res.json({ 
    status: 'OK', 
    timestamp: new Date().toISOString(),
    database: dbInitialized ? 'Connected' : 'Not Ready',
    port: PORT
  });
});

// Admin endpoints
app.delete('/api/admin/clear-bookings', (req, res) => {
  console.log('🗑️ Admin: Clear all bookings requested from:', req.ip);
  
  db.run('DELETE FROM bookings', [], function(err) {
    if (err) {
      console.error('❌ Error clearing bookings:', err);
      return res.status(500).json({ error: 'Failed to clear bookings' });
    }
    
    console.log(`✅ Cleared ${this.changes} bookings`);
    res.json({ 
      success: true, 
      message: `Successfully cleared ${this.changes} bookings`,
      deletedCount: this.changes
    });
  });
});

app.delete('/api/admin/reset-database', (req, res) => {
  console.log('🔄 Admin: Reset database requested from:', req.ip);
  
  // Clear all tables
  const tables = ['bookings', 'users'];
  let completed = 0;
  let totalDeleted = 0;
  
  tables.forEach(table => {
    db.run(`DELETE FROM ${table}`, [], function(err) {
      if (err) {
        console.error(`❌ Error clearing ${table}:`, err);
        return res.status(500).json({ error: `Failed to clear ${table}` });
      }
      
      totalDeleted += this.changes;
      completed++;
      
      if (completed === tables.length) {
        console.log(`✅ Database reset complete. Deleted ${totalDeleted} records`);
        res.json({ 
          success: true, 
          message: `Database reset successfully. Deleted ${totalDeleted} records`,
          deletedCount: totalDeleted
        });
      }
    });
  });
});

// Authentication endpoints
app.post('/api/auth/login', async (req, res) => {
  try {
    const { username, password } = req.body;

    if (!username || !password) {
      return res.status(400).json({ error: 'Username and password required' });
    }

    db.get('SELECT * FROM users WHERE username = ?', [username], async (err, user) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }

      if (!user || !await bcrypt.compare(password, user.password)) {
        return res.status(401).json({ error: 'Invalid credentials' });
      }

      const token = jwt.sign(
        { id: user.id, username: user.username },
        JWT_SECRET,
        { expiresIn: '24h' }
      );

      res.json({
        success: true,
        token,
        user: {
          id: user.id,
          username: user.username
        }
      });
    });
  } catch (error) {
    res.status(500).json({ error: 'Server error' });
  }
});

// Rooms endpoints
app.get('/api/rooms', (req, res) => {
  console.log('📝 API Request: GET /api/rooms');
  
  db.all('SELECT * FROM rooms ORDER BY id', (err, rooms) => {
    if (err) {
      console.error('❌ Database error:', err);
      return res.status(500).json({ error: 'Database error' });
    }
    
    console.log(`✅ Found ${rooms.length} rooms`);
    res.json(rooms);
  });
});

// Bookings endpoints
app.get('/api/bookings/all', (req, res) => {
  console.log('📝 API Request: GET /api/bookings/all');
  
  db.all(`SELECT b.*, r.name as room_name, u.username 
          FROM bookings b 
          LEFT JOIN rooms r ON b.room_id = r.id 
          LEFT JOIN users u ON b.user_id = u.id 
          ORDER BY b.created_at DESC`, (err, bookings) => {
    if (err) {
      console.error('❌ Database error:', err);
      return res.status(500).json({ error: 'Database error' });
    }
    
    console.log(`✅ Found ${bookings.length} bookings`);
    res.json(bookings);
  });
});

app.post('/api/bookings', authenticateToken, (req, res) => {
  console.log('📝 API Request: POST /api/bookings');
  console.log('📋 Request body:', req.body);
  
  const { room_id, date, start_time, end_time } = req.body;
  const user_id = req.user.id;
  
  if (!room_id || !date || !start_time || !end_time) {
    return res.status(400).json({ error: 'Missing required fields' });
  }
  
  // Generate access code
  const access_code = Math.floor(100000 + Math.random() * 900000).toString();
  
  db.run(`INSERT INTO bookings (user_id, room_id, booking_date, start_time, end_time, access_code) 
          VALUES (?, ?, ?, ?, ?, ?)`,
    [user_id, room_id, date, start_time, end_time, access_code],
    function(err) {
      if (err) {
        console.error('❌ Database error:', err);
        return res.status(500).json({ error: 'Database error' });
      }
      
      // Get the complete booking info
      db.get(`SELECT b.*, r.name as room_name, u.username 
              FROM bookings b 
              LEFT JOIN rooms r ON b.room_id = r.id 
              LEFT JOIN users u ON b.user_id = u.id 
              WHERE b.id = ?`, [this.lastID], (err, booking) => {
        if (err) {
          console.error('❌ Error fetching booking:', err);
          return res.status(500).json({ error: 'Database error' });
        }
        
        console.log('✅ Booking created successfully:', booking);
        res.status(201).json(booking);
      });
    }
  );
});

// Room Availability API
app.get('/api/rooms/:roomId/availability/:date', (req, res) => {
  console.log('📅 Room Availability Request:', req.params);
  
  const { roomId, date } = req.params;
  
  // Get all bookings for this room on this date
  db.all(`
    SELECT start_time, end_time, status
    FROM bookings 
    WHERE room_id = ? AND booking_date = ? AND status = 'active'
    ORDER BY start_time
  `, [roomId, date], (err, bookings) => {
    if (err) {
      console.error('❌ Availability Error:', err);
      return res.status(500).json({ error: 'Database error' });
    }
    
    // Generate all possible time slots (24 hours)
    const allSlots = [];
    for (let hour = 0; hour < 24; hour++) {
      const startTime = hour.toString().padStart(2, '0') + ':00';
      const endTime = ((hour + 1) % 24).toString().padStart(2, '0') + ':00';
      allSlots.push({ start: startTime, end: endTime });
    }
    
    // Filter out booked slots
    const availableSlots = allSlots.filter(slot => {
      return !bookings.some(booking => 
        slot.start >= booking.start_time && slot.start < booking.end_time
      );
    });
    
    console.log('✅ Available slots:', availableSlots.length, 'out of', allSlots.length);
    res.json(availableSlots);
  });
});

// Arduino Integration API
app.get('/api/arduino/status/:roomId', (req, res) => {
  console.log('📱 Arduino Status Request:', req.params.roomId);
  
  const roomId = req.params.roomId;
  
  // Check current booking status for the room
  const now = new Date();
  const currentDate = now.toLocaleDateString('sv-SE'); // YYYY-MM-DD format
  const currentTime = now.toLocaleTimeString('en-GB', { hour12: false }).slice(0, 5); // HH:MM format
  
  console.log(`🕐 Current Bangkok Time: ${currentDate} ${currentTime}`);
  
  db.get(`
    SELECT b.*, r.name as room_name, r.status as room_status
    FROM bookings b
    LEFT JOIN rooms r ON b.room_id = r.id
    WHERE b.room_id = ? AND b.status = 'active'
    AND b.booking_date = ?
    AND ? BETWEEN b.start_time AND b.end_time
    ORDER BY b.created_at DESC
    LIMIT 1
  `, [roomId, currentDate, currentTime], (err, booking) => {
    if (err) {
      console.error('❌ Arduino Status Error:', err);
      return res.status(500).json({ 
        error: 'Database error',
        isBooked: false,
        roomStatus: 'available'
      });
    }
    
    const response = {
      roomId: parseInt(roomId),
      isBooked: booking ? true : false,
      roomStatus: booking ? 'occupied' : 'available',
      currentBooking: booking ? {
        bookingId: booking.id,
        userId: booking.user_id,
        startTime: booking.start_time,
        endTime: booking.end_time,
        accessCode: booking.access_code
      } : null,
      timestamp: new Date().toISOString()
    };
    
    console.log('✅ Arduino Status Response:', response);
    res.json(response);
  });
});

app.post('/api/arduino/unlock/:roomId', (req, res) => {
  console.log('🔓 Arduino Unlock Request:', req.params.roomId, req.body);
  
  const roomId = req.params.roomId;
  const { access_code, accessCode, userId } = req.body;
  
  // Support both access_code (Arduino) and accessCode (web) formats
  const finalAccessCode = access_code || accessCode;
  
  console.log(`🔍 Using access code: ${finalAccessCode}`);
  
  // Use JavaScript time instead of SQLite localtime
  const now = new Date();
  const currentDate = now.toLocaleDateString('sv-SE'); // YYYY-MM-DD format
  const currentTime = now.toLocaleTimeString('en-GB', { hour12: false }).slice(0, 5); // HH:MM format
  
  console.log(`🕐 Current Bangkok Time: ${currentDate} ${currentTime}`);
  
  // Verify access code and current booking
  db.get(`
    SELECT b.*, r.name as room_name
    FROM bookings b
    LEFT JOIN rooms r ON b.room_id = r.id
    WHERE b.room_id = ? AND b.access_code = ? AND b.status = 'active'
    AND b.booking_date = ?
    AND ? BETWEEN b.start_time AND b.end_time
  `, [roomId, finalAccessCode, currentDate, currentTime], (err, booking) => {
    if (err) {
      console.error('❌ Arduino Unlock Error:', err);
      return res.status(500).json({ 
        success: false,
        error: 'Database error',
        unlocked: false
      });
    }
    
    if (!booking) {
      console.log('❌ Invalid access code or no active booking');
      return res.status(403).json({ 
        success: false,
        error: 'Invalid access code or no active booking',
        unlocked: false
      });
    }
    
    // Log successful unlock
    const unlockLog = {
      roomId: parseInt(roomId),
      bookingId: booking.id,
      userId: booking.user_id,
      accessCode: finalAccessCode,
      unlockTime: new Date().toISOString(),
      roomName: booking.room_name
    };
    
    console.log('✅ Arduino Unlock Success:', unlockLog);
    
    // Update room status to occupied
    db.run('UPDATE rooms SET status = ? WHERE id = ?', ['occupied', roomId], (err) => {
      if (err) {
        console.error('❌ Error updating room status:', err);
      }
    });
    
    res.json({
      success: true,
      unlocked: true,
      booking: {
        id: booking.id,
        roomName: booking.room_name,
        startTime: booking.start_time,
        endTime: booking.end_time,
        userId: booking.user_id
      },
      unlockTime: unlockLog.unlockTime,
      message: `Room ${booking.room_name} unlocked successfully`
    });
  });
});

app.post('/api/arduino/checkin/:roomId', (req, res) => {
  console.log('📍 Arduino Check-in Request:', req.params.roomId);
  
  const roomId = req.params.roomId;
  
  // Update room status to occupied
  db.run('UPDATE rooms SET status = ? WHERE id = ?', ['occupied', roomId], (err) => {
    if (err) {
      console.error('❌ Arduino Check-in Error:', err);
      return res.status(500).json({ 
        success: false,
        error: 'Database error'
      });
    }
    
    console.log(`✅ Room ${roomId} checked in successfully`);
    res.json({
      success: true,
      roomId: parseInt(roomId),
      status: 'occupied',
      checkinTime: new Date().toISOString(),
      message: 'Check-in successful'
    });
  });
});

app.post('/api/arduino/checkout/:roomId', (req, res) => {
  console.log('📤 Arduino Check-out Request:', req.params.roomId);
  
  const roomId = req.params.roomId;
  
  // Update room status to available
  db.run('UPDATE rooms SET status = ? WHERE id = ?', ['available', roomId], (err) => {
    if (err) {
      console.error('❌ Arduino Check-out Error:', err);
      return res.status(500).json({ 
        success: false,
        error: 'Database error'
      });
    }
    
    console.log(`✅ Room ${roomId} checked out successfully`);
    res.json({
      success: true,
      roomId: parseInt(roomId),
      status: 'available',
      checkoutTime: new Date().toISOString(),
      message: 'Check-out successful'
    });
  });
});

// Manual LED Control Endpoints - All LEDs together
app.post('/api/arduino/led/:roomId', (req, res) => {
  console.log('💡 Manual LED Control Request:', req.params.roomId, req.body);
  
  const roomId = req.params.roomId;
  const { action } = req.body; // action can be 'on', 'off', or 'toggle'
  
  // Validate input
  if (!roomId) {
    return res.status(400).json({ 
      success: false,
      error: 'Room ID is required'
    });
  }
  
  // Store LED states (in a real application, you might want to store this in database)
  global.manualLEDStates = global.manualLEDStates || {};
  global.manualLEDStates[roomId] = global.manualLEDStates[roomId] || { led2: false, led3: false, led4: false };
  
  // Apply changes based on action - ALL LEDs together
  if (action === 'on') {
    global.manualLEDStates[roomId].led2 = true;
    global.manualLEDStates[roomId].led3 = true;
    global.manualLEDStates[roomId].led4 = true;
  } else if (action === 'off') {
    global.manualLEDStates[roomId].led2 = false;
    global.manualLEDStates[roomId].led3 = false;
    global.manualLEDStates[roomId].led4 = false;
  } else if (action === 'toggle') {
    // Toggle all LEDs together
    const currentState = global.manualLEDStates[roomId].led2 || global.manualLEDStates[roomId].led3 || global.manualLEDStates[roomId].led4;
    const newState = !currentState;
    global.manualLEDStates[roomId].led2 = newState;
    global.manualLEDStates[roomId].led3 = newState;
    global.manualLEDStates[roomId].led4 = newState;
  }
  
  console.log(`💡 LED States for Room ${roomId}:`, global.manualLEDStates[roomId]);
  
  res.json({
    success: true,
    roomId: parseInt(roomId),
    ledStates: global.manualLEDStates[roomId],
    timestamp: new Date().toISOString(),
    message: 'Manual LED control updated - All LEDs together'
  });
});

app.get('/api/arduino/led/:roomId', (req, res) => {
  console.log('💡 Get LED Status Request:', req.params.roomId);
  
  const roomId = req.params.roomId;
  
  // Initialize if not exists
  global.manualLEDStates = global.manualLEDStates || {};
  global.manualLEDStates[roomId] = global.manualLEDStates[roomId] || { led2: false, led3: false, led4: false };
  
  res.json({
    success: true,
    roomId: parseInt(roomId),
    ledStates: global.manualLEDStates[roomId],
    timestamp: new Date().toISOString()
  });
});

// Catch-all for undefined API routes
app.use('/api/*', (req, res) => {
  res.status(404).json({ error: 'API endpoint not found' });
});

// Enhanced server startup with port management
async function startServer() {
  try {
    console.log('🔍 Checking server startup...');
    
    // Initialize database first
    await initializeDBConnection();
    
    // Check if port is available
    const isAvailable = await isPortAvailable(PORT);
    
    if (!isAvailable) {
      console.log(`⚠️  Port ${PORT} is in use, attempting to free it...`);
      const killed = await killProcessOnPort(PORT);
      
      if (killed) {
        // Wait a moment for port to be freed
        await new Promise(resolve => setTimeout(resolve, 2000));
      } else {
        // Find alternative port
        console.log(`🔍 Finding alternative port...`);
        const availablePort = await findAvailablePort(3001, 3010);
        console.log(`✅ Using alternative port: ${availablePort}`);
        PORT = availablePort;
      }
    }
    
    // Start server
    const server = app.listen(PORT, '0.0.0.0', () => {
      console.log(`🚀 Server running on port ${PORT}`);
      console.log(`📱 API available at http://localhost:${PORT}/api`);
      console.log(`🌐 External access: http://[YOUR-IP]:${PORT}`);
      console.log(`❤️  Health check: http://localhost:${PORT}/api/health`);
    });
    
    // Handle server errors
    server.on('error', async (err) => {
      if (err.code === 'EADDRINUSE') {
        console.log(`❌ Port ${PORT} still in use, trying alternative...`);
        try {
          const newPort = await findAvailablePort(PORT + 1, PORT + 10);
          console.log(`🔄 Retrying with port ${newPort}...`);
          PORT = newPort;
          setTimeout(() => startServer(), 1000);
        } catch (error) {
          console.error('💥 Could not find available port:', error.message);
          process.exit(1);
        }
      } else {
        console.error('💥 Server error:', err);
        process.exit(1);
      }
    });
    
    return server;
    
  } catch (error) {
    console.error('💥 Failed to start server:', error.message);
    process.exit(1);
  }
}

// Enhanced error handling
process.on('uncaughtException', (err) => {
  console.error('🚨 Uncaught Exception:', err);
  
  // Log error details but don't crash
  if (err.code === 'EADDRINUSE') {
    console.log('🔄 Port conflict detected, server will attempt restart...');
    return;
  }
  
  // For other critical errors, graceful shutdown
  console.log('💥 Critical error detected, initiating graceful shutdown...');
  gracefulShutdown('UNCAUGHT_EXCEPTION');
});

// Enhanced promise rejection handling  
process.on('unhandledRejection', (reason, promise) => {
  console.error('🚨 Unhandled Rejection at:', promise, 'reason:', reason);
  
  // Don't crash on unhandled rejections, just log them
  if (reason && reason.code === 'EADDRINUSE') {
    console.log('🔄 Promise rejection due to port conflict, handled by server startup logic');
    return;
  }
  
  // Log but continue running for non-critical rejections
  console.log('⚠️  Continuing server operation...');
});

// Memory management
function forceGarbageCollection() {
  if (global.gc) {
    global.gc();
    console.log('🗑️  Forced garbage collection completed');
  }
}

// Memory monitoring
function checkMemoryUsage() {
  const usage = process.memoryUsage();
  const heapUsedMB = Math.round(usage.heapUsed / 1024 / 1024);
  const heapTotalMB = Math.round(usage.heapTotal / 1024 / 1024);
  
  console.log(`💾 [${new Date().toLocaleTimeString()}] Memory: ${heapUsedMB}MB / ${heapTotalMB}MB`);
  
  // Force GC if memory usage is high (>100MB)
  if (heapUsedMB > 100) {
    console.log('⚠️  High memory usage detected, forcing garbage collection...');
    forceGarbageCollection();
  }
}

// Graceful shutdown handler
let isShuttingDown = false;

function gracefulShutdown(signal) {
  if (isShuttingDown) return;
  isShuttingDown = true;
  
  console.log(`\n🛑 ${signal} received, shutting down gracefully...`);
  
  // Close database connection
  if (db) {
    db.close((err) => {
      if (err) {
        console.error('❌ Error closing database:', err);
      } else {
        console.log('✅ Database connection closed');
      }
      
      console.log('🏁 Server shutdown complete');
      process.exit(0);
    });
  } else {
    console.log('🏁 Server shutdown complete');
    process.exit(0);
  }
  
  // Force exit after 10 seconds if graceful shutdown fails
  setTimeout(() => {
    console.log('⚠️  Force exit after timeout');
    process.exit(1);
  }, 10000);
}

// Handle shutdown signals
process.on('SIGTERM', () => gracefulShutdown('SIGTERM'));
process.on('SIGINT', () => gracefulShutdown('SIGINT'));

// Health monitoring and memory cleanup
setInterval(() => {
  if (!isShuttingDown) {
    checkMemoryUsage();
  }
}, 5 * 60 * 1000); // Every 5 minutes

// Periodic garbage collection
setInterval(() => {
  if (!isShuttingDown) {
    forceGarbageCollection();
  }
}, 10 * 60 * 1000); // Every 10 minutes

// Start server only in non-production environment
if (process.env.NODE_ENV !== 'production') {
  startServer();
}

module.exports = app;