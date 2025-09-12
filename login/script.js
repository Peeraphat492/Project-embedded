// script.js
document.getElementById("loginForm").addEventListener("submit", function(event) {
  event.preventDefault(); // ป้องกันการรีเฟรชหน้า

  const username = document.getElementById("username").value.trim();
  const password = document.getElementById("password").value.trim();

  if (username === "" || password === "") {
    alert("⚠️ กรุณากรอกข้อมูลให้ครบถ้วน");
  } else {
    alert(`✅ ยินดีต้อนรับคุณ ${username}`);
    // ที่นี่สามารถเขียนโค้ดต่อ เช่น redirect ไปหน้า dashboard
    // window.location.href = "dashboard.html";
  }
});