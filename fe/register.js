document.getElementById('registerForm').addEventListener('submit', async (e) => {
    e.preventDefault();
    const formData = new FormData(e.target);
    const email = formData.get('email');
    const username = formData.get('username');
    const password = formData.get('password');
  
    try {
      const response = await fetch('http://localhost:3000/register', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ email, username, password })
      });
      const data = await response.json();
      
      if (data.response.includes('200')) {
        alert('Registration successful. Redirecting to login...');
        // Chuyển hướng về trang Login
        window.location.href = 'index.html';
      } else {
        alert('Registration failed: ' + data.response);
      }
    } catch (err) {
      console.error('Error:', err);
      alert('Error connecting to server');
    }
  });
  