async function testUnlock() {
  console.log('🔐 Testing unlock API...');
  
  try {
    const response = await fetch('http://localhost:3001/api/arduino/unlock/6', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify({
        accessCode: '631023',
        userId: 'arduino_6'
      })
    });
    
    const result = await response.json();
    console.log('📥 Response:', result);
    
    if (result.success && result.unlocked) {
      console.log('✅ Unlock successful!');
    } else {
      console.log('❌ Unlock failed');
    }
    
  } catch (error) {
    console.error('❌ Error:', error.message);
  }
}

testUnlock();