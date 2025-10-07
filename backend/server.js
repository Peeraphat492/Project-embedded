const express = require('express');
const cors = require('cors');
const sqlite3 = require('sqlite3').verbose();
const bcrypt = require('bcryptjs');
const jwt = require('jsonwebtoken');
const rateLimit = require('express-rate-limit');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || 'your-secret-key-here';

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static('public'));

// Rate limiting
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100 // limit each IP to 100 requests per windowMs
});
app.use('/api/', limiter);

// Database initialization
const db = new sqlite3.Database('./database.db', (err) => {
  if (err) {
    console.error('Error opening database:', err);
  } else {
    console.log('Connected to SQLite database');
    initializeDatabase();
  }
});

// Initialize database tables
function initializeDatabase() {
  // Create tables in sequence to avoid race conditions
  db.serialize(() => {
    // Users table
    db.run(`CREATE TABLE IF NOT EXISTS users (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      username TEXT UNIQUE NOT NULL,
      password TEXT NOT NULL,
      email TEXT,
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )`);

    // Rooms table
    db.run(`CREATE TABLE IF NOT EXISTS rooms (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL,
      description TEXT,
      capacity INTEGER,
      image_url TEXT,
      amenities TEXT,
      status TEXT DEFAULT 'available',
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP
    )`);

    // Bookings table
    db.run(`CREATE TABLE IF NOT EXISTS bookings (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      user_id INTEGER,
      room_id INTEGER,
      booking_date DATE NOT NULL,
      start_time TIME NOT NULL,
      end_time TIME NOT NULL,
      access_code TEXT,
      status TEXT DEFAULT 'active',
      created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
      FOREIGN KEY (user_id) REFERENCES users (id),
      FOREIGN KEY (room_id) REFERENCES rooms (id)
    )`);

    // Insert default admin user
    const defaultPassword = bcrypt.hashSync('1234', 10);
    db.run(`INSERT OR IGNORE INTO users (username, password) VALUES (?, ?)`, 
      ['admin', defaultPassword]);

    // Insert default rooms only if table is empty
    db.get("SELECT COUNT(*) as count FROM rooms", (err, row) => {
      if (err) {
        console.error('Error checking rooms count:', err);
        return;
      }
      
      if (row.count === 0) {
        const defaultRooms = [
          {
            name: 'Meeting Room1',
            description: 'ห้องประชุมขนาดกลาง เหมาะสำหรับการประชุมทีม',
            capacity: 8,
            image_url: 'https://www.homenayoo.com/wp-content/uploads/2018/03/1091.jpg'
          },
          {
            name: 'Meeting Room2', 
            description: 'ห้องประชุมขนาดใหญ่ พร้อมอุปกรณ์การนำเสนอ',
            capacity: 12,
            image_url: 'https://cdn.ananda.co.th/blog/thegenc/wp-content/uploads/2023/09/Meeting-01-825x550.jpg'
          },
          {
            name: 'Cooking Room',
            description: 'ห้องครัวสำหรับทำอาหารและกิจกรรมการปรุงอาหาร',
            capacity: 6,
            image_url: 'https://img.home.co.th/Images/img_v/BuyHome/20200525-114901-Co-Kitchen-FB-00.jpg'
          },
          {
            name: 'Karaoke Room',
            description: 'ห้องคาราโอเกะพร้อมระบบเสียงคุณภาพสูง',
            capacity: 10,
            image_url: 'https://www.centarahotelsresorts.com/centara/sites/centara-centara/files/styles/822x800/public/2022-05/Karaoke-Room.jpg.JPG?itok=6JeEfR9-'
          },
          {
            name: 'Game Room',
            description: 'ห้องเกมส์พร้อมเครื่องเกมคอนโซลและเกมต่างๆ',
            capacity: 8,
            image_url: 'https://prop2morrow.com/wp-content/uploads/2021/12/2021-12-10_17-14-58-assetwise.jpg'
          },
          {
            name: 'Facial Sauna Room',
            description: 'ห้องซาวน่าสำหรับผ่อนคลายและดูแลผิวหน้า',
            capacity: 4,
            image_url: 'https://iauna.net/wp-content/uploads/2021/05/cs.01.png'
          }
        ];

        defaultRooms.forEach(room => {
          db.run(`INSERT INTO rooms (name, description, capacity, image_url) VALUES (?, ?, ?, ?)`,
            [room.name, room.description, room.capacity, room.image_url]);
        });

        console.log('✅ Default rooms inserted successfully');
      } else {
        console.log(`📊 Found ${row.count} existing rooms in database`);
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

// Get all rooms
app.get('/api/rooms', (req, res) => {
  db.all('SELECT * FROM rooms WHERE status = ?', ['available'], (err, rows) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    res.json(rows);
  });
});

// Get room by ID
app.get('/api/rooms/:id', (req, res) => {
  const roomId = req.params.id;
  db.get('SELECT * FROM rooms WHERE id = ?', [roomId], (err, row) => {
    if (err) {
      return res.status(500).json({ error: 'Database error' });
    }
    if (!row) {
      return res.status(404).json({ error: 'Room not found' });
    }
    res.json(row);
  });
});

// Get available time slots for a room
app.get('/api/rooms/:id/availability/:date', (req, res) => {
  const { id, date } = req.params;
  
  // Get all bookings for the room on the specified date
  db.all(
    'SELECT start_time, end_time FROM bookings WHERE room_id = ? AND booking_date = ? AND status = ?',
    [id, date, 'active'],
    (err, bookings) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }

      // Define all possible time slots
      const allTimeSlots = [
        { start: '08:00', end: '09:00' },
        { start: '09:00', end: '10:00' },
        { start: '10:00', end: '11:00' },
        { start: '11:00', end: '12:00' },
        { start: '12:00', end: '13:00' },
        { start: '13:00', end: '14:00' },
        { start: '14:00', end: '15:00' },
        { start: '15:00', end: '16:00' },
        { start: '16:00', end: '17:00' },
        { start: '17:00', end: '18:00' },
        { start: '18:00', end: '19:00' },
        { start: '19:00', end: '20:00' },
        { start: '20:00', end: '21:00' },
        { start: '21:00', end: '22:00' },
        { start: '22:00', end: '23:00' },
        { start: '23:00', end: '00:00' }
      ];

      // Filter out booked time slots
      const availableSlots = allTimeSlots.filter(slot => {
        return !bookings.some(booking => 
          booking.start_time === slot.start && booking.end_time === slot.end
        );
      });

      res.json(availableSlots);
    }
  );
});

// Create a new booking
app.post('/api/bookings', authenticateToken, (req, res) => {
  const { room_id, booking_date, start_time, end_time } = req.body;
  const user_id = req.user.id;

  if (!room_id || !booking_date || !start_time || !end_time) {
    return res.status(400).json({ error: 'All booking details required' });
  }

  // Generate 6-digit access code
  const access_code = Math.floor(100000 + Math.random() * 900000).toString();

  // Check if the time slot is available
  db.get(
    'SELECT id FROM bookings WHERE room_id = ? AND booking_date = ? AND start_time = ? AND end_time = ? AND status = ?',
    [room_id, booking_date, start_time, end_time, 'active'],
    (err, existingBooking) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }

      if (existingBooking) {
        return res.status(409).json({ error: 'Time slot already booked' });
      }

      // Create the booking
      db.run(
        'INSERT INTO bookings (user_id, room_id, booking_date, start_time, end_time, access_code) VALUES (?, ?, ?, ?, ?, ?)',
        [user_id, room_id, booking_date, start_time, end_time, access_code],
        function(err) {
          if (err) {
            return res.status(500).json({ error: 'Failed to create booking' });
          }

          // Get the complete booking info with room details
          db.get(
            `SELECT b.*, r.name as room_name, r.image_url as room_image, u.username 
             FROM bookings b
             JOIN rooms r ON b.room_id = r.id
             JOIN users u ON b.user_id = u.id
             WHERE b.id = ?`,
            [this.lastID],
            (err, booking) => {
              if (err) {
                return res.status(500).json({ error: 'Database error' });
              }

              res.status(201).json({
                success: true,
                booking: booking
              });
            }
          );
        }
      );
    }
  );
});

// Get user's bookings
app.get('/api/bookings/my', authenticateToken, (req, res) => {
  const user_id = req.user.id;

  db.all(
    `SELECT b.*, r.name as room_name, r.image_url as room_image
     FROM bookings b
     JOIN rooms r ON b.room_id = r.id
     WHERE b.user_id = ? AND b.status = ?
     ORDER BY b.booking_date DESC, b.start_time DESC`,
    [user_id, 'active'],
    (err, bookings) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      res.json(bookings);
    }
  );
});

// Cancel a booking
app.delete('/api/bookings/:id', authenticateToken, (req, res) => {
  const bookingId = req.params.id;
  const user_id = req.user.id;

  db.run(
    'UPDATE bookings SET status = ? WHERE id = ? AND user_id = ?',
    ['cancelled', bookingId, user_id],
    function(err) {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }

      if (this.changes === 0) {
        return res.status(404).json({ error: 'Booking not found or unauthorized' });
      }

      res.json({ success: true, message: 'Booking cancelled successfully' });
    }
  );
});

// IoT device endpoints (for ESP32 integration)
app.get('/api/device/settings', (req, res) => {
  // Return current time control settings
  const now = new Date();
  res.json({
    currentTime: now.toTimeString().slice(0, 8),
    wakeTime: "09:00",
    stopTime: "11:00",
    relaystatus: false
  });
});

app.post('/api/device/control', (req, res) => {
  const { action, timeRange } = req.body;
  
  // Handle device control commands
  console.log(`Device control: ${action}, Time: ${timeRange}`);
  
  res.json({
    success: true,
    message: `Device ${action} command executed`,
    timestamp: new Date().toISOString()
  });
});

// Health check endpoint
app.get('/api/health', (req, res) => {
  res.json({ 
    status: 'OK', 
    timestamp: new Date().toISOString(),
    database: 'Connected'
  });
});

// Get all bookings (Admin only)
app.get('/api/admin/bookings', (req, res) => {
  db.all(
    `SELECT b.*, r.name as room_name, u.username
     FROM bookings b
     LEFT JOIN rooms r ON b.room_id = r.id
     LEFT JOIN users u ON b.user_id = u.id
     ORDER BY b.created_at DESC`,
    [],
    (err, bookings) => {
      if (err) {
        return res.status(500).json({ error: 'Database error' });
      }
      res.json(bookings);
    }
  );
});

// Clear all bookings (Admin only)
app.delete('/api/admin/clear-bookings', (req, res) => {
  db.run('DELETE FROM bookings', [], function(err) {
    if (err) {
      return res.status(500).json({ error: 'Failed to clear bookings' });
    }
    res.json({ 
      success: true, 
      message: `Cleared ${this.changes} bookings`,
      timestamp: new Date().toISOString()
    });
  });
});

// Reset database to initial state (Admin only)
app.post('/api/admin/reset-database', (req, res) => {
  // Clear all user data but keep admin
  db.serialize(() => {
    db.run('DELETE FROM bookings');
    db.run('DELETE FROM users WHERE username != ?', ['admin']);
    
    res.json({
      success: true,
      message: 'Database reset to initial state',
      timestamp: new Date().toISOString()
    });
  });
});

// Get database statistics
app.get('/api/admin/stats', (req, res) => {
  const stats = {};
  
  db.get('SELECT COUNT(*) as count FROM users', (err, result) => {
    if (err) return res.status(500).json({ error: 'Database error' });
    stats.users = result.count;
    
    db.get('SELECT COUNT(*) as count FROM rooms', (err, result) => {
      if (err) return res.status(500).json({ error: 'Database error' });
      stats.rooms = result.count;
      
      db.get('SELECT COUNT(*) as count FROM bookings', (err, result) => {
        if (err) return res.status(500).json({ error: 'Database error' });
        stats.bookings = result.count;
        stats.timestamp = new Date().toISOString();
        
        res.json(stats);
      });
    });
  });
});

// Default route - redirect to login
app.get('/', (req, res) => {
  res.redirect('/login.html');
});

// Error handling middleware
app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({ error: 'Something went wrong!' });
});

// 404 handler (must be last)
app.use((req, res) => {
  res.status(404).json({ error: 'API endpoint not found' });
});

// Start server
app.listen(PORT, '0.0.0.0', () => {
  console.log(`🚀 Server running on port ${PORT}`);
  console.log(`📱 API available at http://localhost:${PORT}/api`);
  console.log(`🌐 External access: http://[YOUR-IP]:${PORT}`);
  console.log(`❤️  Health check: http://localhost:${PORT}/api/health`);
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n🛑 Shutting down server...');
  db.close((err) => {
    if (err) {
      console.error('Error closing database:', err);
    } else {
      console.log('Database connection closed.');
    }
    process.exit(0);
  });
});