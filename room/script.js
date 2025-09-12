// script.js ของหน้า Rooms
// ตัวอย่าง: alert เวลาเลือกห้อง (สามารถปรับต่อเพื่อทำระบบจริง)
document.querySelectorAll('.group.cursor-pointer').forEach(room => {
  room.addEventListener('click', () => {
    const roomName = room.querySelector('div.absolute.bottom-2').innerText;
    alert(`คุณเลือก ${roomName}`);
  });
});