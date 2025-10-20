const sqlite3 = require('sqlite3').verbose();

const db = new sqlite3.Database('./database.db');

console.log('=== CHECKING TODAY\'S BOOKINGS ===');

// ตรวจสอบข้อมูลการจองวันนี้
db.all(`
  SELECT b.*, r.name as room_name
  FROM bookings b
  LEFT JOIN rooms r ON b.room_id = r.id
  WHERE date(b.booking_date) = date('now', 'localtime')
  ORDER BY b.created_at DESC
`, (err, rows) => {
  if (err) {
    console.error('❌ Error:', err);
  } else {
    console.log(`📅 Found ${rows.length} bookings for today:`);
    
    if (rows.length === 0) {
      console.log('ℹ️  No bookings found for today');
    } else {
      rows.forEach((booking, index) => {
        console.log(`\n${index + 1}. Booking ID: ${booking.id}`);
        console.log(`   Room: ${booking.room_name} (ID: ${booking.room_id})`);
        console.log(`   Access Code: ${booking.access_code}`);
        console.log(`   Time: ${booking.start_time} - ${booking.end_time}`);
        console.log(`   Date: ${booking.booking_date}`);
        console.log(`   Status: ${booking.status}`);
        console.log(`   User ID: ${booking.user_id}`);
      });
    }
  }
  
  // ตรวจสอบการจองที่ active ในช่วงเวลาปัจจุบัน
  db.all(`
    SELECT b.*, r.name as room_name
    FROM bookings b
    LEFT JOIN rooms r ON b.room_id = r.id
    WHERE b.status = 'active'
    AND b.booking_date = date('now', 'localtime')
    AND time('now', 'localtime') BETWEEN b.start_time AND b.end_time
  `, (err, activeRows) => {
    if (err) {
      console.error('❌ Error checking active bookings:', err);
    } else {
      console.log(`\n🔴 Currently ACTIVE bookings: ${activeRows.length}`);
      activeRows.forEach((booking, index) => {
        console.log(`\n${index + 1}. ACTIVE: Room ${booking.room_name}`);
        console.log(`   Access Code: ${booking.access_code}`);
        console.log(`   Time: ${booking.start_time} - ${booking.end_time}`);
      });
    }
    
    db.close();
  });
});