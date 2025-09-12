// ทำให้เลือกเวลาได้ทีละช่อง
const slots = document.querySelectorAll(".time-slot");
slots.forEach(slot => {
  slot.addEventListener("click", () => {
    slots.forEach(s => s.classList.remove("selected"));
    slot.classList.add("selected");
  });
});

// เพิ่มฟังก์ชัน confirm reservation
document.querySelector("footer button").addEventListener("click", () => {
  const selected = document.querySelector(".time-slot.selected");
  if (selected) {
    alert(`คุณจองเวลา: ${selected.innerText}`);
  } else {
    alert("กรุณาเลือกช่วงเวลา");
  }
});