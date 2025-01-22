document.addEventListener('DOMContentLoaded', async () => {
  const token = localStorage.getItem('authToken');
  
  if (!token) {
    alert('Không token. Đăng nhập lại.');
    window.location.href = 'index.html';
    return;
  }
  // Retrieve and Display User Information
  const username = localStorage.getItem('username');
  const email = localStorage.getItem('userEmail');

  const usernameElem = document.getElementById('username');
  const emailElem = document.getElementById('email');

  if (username && email) {
    usernameElem.textContent = `Xin chào, ${username}`;
    emailElem.textContent = `Email: ${email}`;
  } else {
    usernameElem.textContent = 'Username: N/A';
    emailElem.textContent = 'Email: N/A';
  }

  const projectList = document.getElementById('projectList');

  // Function to load projects
  async function loadProjects() {
    projectList.innerHTML = ''; // Clear existing projects
    try {
      const response = await fetch('http://localhost:3000/projects', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json'},
        body: JSON.stringify({ token })
      });
      const data = await response.json();
      console.log('Projects:', data);
  
      if (data.projects && data.projects.length > 0) {
        data.projects.forEach(proj => {
          const card = document.createElement('div');
          card.className = 'project-card';
          card.innerHTML = `
            <img src="projectAvatar.png" alt="Project Avatar" class="project-avatar" />
            <h3>${proj.name}</h3>
            <small>ID: ${proj.id}</small>
          `;
          card.addEventListener('click', () => {
            // Navigate to project detail page
            window.location.href = `project_detail.html?id=${proj.id}`;
          });
          projectList.appendChild(card);
        });
      } else {
        projectList.innerHTML = '<p>Không tìm thấy dự án nào.</p>';
      }
    } catch (err) {
      console.error('Error:', err);
      alert('Error fetching projects');
    }
  }

  // Initial load
  await loadProjects();

  // Logout
  const logoutBtn = document.getElementById('logoutBtn');
  logoutBtn.addEventListener('click', async () => {
    try {
      const resp = await fetch('http://localhost:3000/logout', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json'},
        body: JSON.stringify({ token })
      });
      const result = await resp.json();
      if (result.success) {
        alert('Đẵng xuất thành công');
        localStorage.removeItem('authToken');
        window.location.href = 'index.html';
      } else {
        alert('Logout failed: ' + (result.error || 'Unknown error'));
      }
    } catch(e) {
      console.error(e);
      alert('Error connecting to server');
    }
  });

  // Show/hide create project form
  const createProjectBtn = document.getElementById('createProjectBtn');
  const createProjectForm = document.getElementById('createProjectForm');
  const submitProjectBtn = document.getElementById('submitProjectBtn');
  const cancelProjectBtn = document.getElementById('cancelProjectBtn');
  const createProjectMessage = document.getElementById('createProjectMessage');

  createProjectBtn.addEventListener('click', () => {
    createProjectForm.style.display = 'block';
    window.scrollTo(0, document.body.scrollHeight);
  });

  cancelProjectBtn.addEventListener('click', () => {
    createProjectForm.style.display = 'none';
    createProjectMessage.textContent = '';
    document.getElementById('newProjectName').value = '';
    document.getElementById('newProjectDescription').value = '';
  });

  submitProjectBtn.addEventListener('click', async () => {
    const projectName = document.getElementById('newProjectName').value.trim();
    const projectDescription = document.getElementById('newProjectDescription').value.trim();
    if (!projectName || !projectDescription) {
      createProjectMessage.textContent = 'Hãy điền đủ các trường thông tin';
      createProjectMessage.style.color = 'red';
      return;
    }

    try {
      const resp = await fetch('http://localhost:3000/createProject', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json'},
        body: JSON.stringify({ projectName, projectDescription, token })
      });
      const result = await resp.json();
      if (result.projectID) {
        createProjectMessage.textContent = 'Tạo dự án thành công (ID: ' + result.projectID + ')';
        createProjectMessage.style.color = 'green';
        // Reload the project list after a short delay
        setTimeout(() => {
          createProjectForm.style.display = 'none';
          createProjectMessage.textContent = '';
          document.getElementById('newProjectName').value = '';
          document.getElementById('newProjectDescription').value = '';
          loadProjects();
        }, 1000);
      } else {
        createProjectMessage.textContent = 'Failed to create project: ' + (result.error || 'Unknown error');
        createProjectMessage.style.color = 'red';
      }
    } catch(e) {
      console.error(e);
      createProjectMessage.textContent = 'Error connecting to server';
      createProjectMessage.style.color = 'red';
    }
  });
});
