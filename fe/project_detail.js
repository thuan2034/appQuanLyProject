document.addEventListener("DOMContentLoaded", async () => {
  // -----------------------
  // Xác thực và Khởi tạo
  // -----------------------

  const token = localStorage.getItem("authToken");
  if (!token) {
    alert("Không tìm thấy token. Vui lòng đăng nhập lại.");
    window.location.href = "index.html";
    return;
  }

  const currentUsername = localStorage.getItem("username") || "Bạn"; // Lấy tên người dùng hiện tại

  // Lấy projectID từ URL
  const urlParams = new URLSearchParams(window.location.search);
  const projectID = urlParams.get("id");
  if (!projectID) {
    alert("Không có ID dự án được cung cấp.");
    window.location.href = "projects.html";
    return;
  }

  // -----------------------
  // Lấy và Hiển thị Chi tiết Dự án
  // -----------------------

  try {
    const response = await fetch("http://localhost:3000/projectDetail", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ projectID, token }),
    });
    const data = await response.json();

    if (data.error) {
      alert("Lỗi khi lấy chi tiết dự án: " + data.error);
      return;
    }

    document.getElementById("projectName").textContent = data.name;
    document.getElementById("projectCreator").textContent = data.createdBy;
    document.getElementById("projectDescription").textContent = data.description;
  } catch (err) {
    console.error("Lỗi:", err);
    alert("Lỗi khi lấy chi tiết dự án");
  }

  // -----------------------
  // Khởi tạo Thành viên và Công việc
  // -----------------------

  let projectMembers = []; // Lưu trữ thành viên để gán công việc

  await fetchAndDisplayMembers();
  await fetchAndDisplayTasks();

  // -----------------------
  // Xử lý Sự kiện
  // -----------------------

  // Xử lý nút Trở về Dự án
  const returnBtn = document.getElementById("returnBtn");
  returnBtn.addEventListener("click", () => {
    window.location.href = "projects.html";
  });

  // Xử lý nút Đăng xuất
  const logoutBtns = document.querySelectorAll("#logoutBtn, #logoutBtnRight");
  logoutBtns.forEach(btn => {
    btn.addEventListener("click", async () => {
      try {
        const resp = await fetch("http://localhost:3000/logout", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ token }),
        });
        const result = await resp.json();
        if (result.success) {
          alert("Đăng xuất thành công");
          localStorage.removeItem("authToken");
          window.location.href = "index.html";
        } else {
          alert("Đăng xuất thất bại: " + (result.error || "Lỗi không xác định"));
        }
      } catch (e) {
        console.error(e);
        alert("Lỗi kết nối đến máy chủ");
      }
    });
  });

  // Xử lý Form Tạo Công việc
  const createTaskForm = document.getElementById("createTaskForm");
  const newTaskNameInput = document.getElementById("newTaskName");
  const newTaskDescriptionInput = document.getElementById("newTaskDescription");
  const timeStartInput = document.getElementById("timeStart"); // New
  const timeEndInput = document.getElementById("timeEnd");     // New
  const submitTaskBtn = document.getElementById("submitTaskBtn");
  const cancelTaskBtn = document.getElementById("cancelTaskBtn");
  const createTaskMessage = document.getElementById("createTaskMessage");

  submitTaskBtn.addEventListener("click", async (e) => {
    e.preventDefault(); // Prevent form submission if within a form

    const taskName = newTaskNameInput.value.trim();
    let taskDescription = newTaskDescriptionInput.value.trim();
    if (taskDescription === "") {
      taskDescription = "Không có mô tả";
    }
    const assignedTo = document.getElementById("assignTo").value;

    // Get the values of timeStart and timeEnd
    const timeStart = timeStartInput.value;
    const timeEnd = timeEndInput.value;

    // Basic validation
    if (!taskName || !assignedTo || !timeStart || !timeEnd) {
      createTaskMessage.textContent = "Vui lòng điền đầy đủ thông tin";
      createTaskMessage.style.color = "red";
      return;
    }

    // Ensure timeStart is on or before timeEnd
    if (new Date(timeStart) > new Date(timeEnd)) {
      createTaskMessage.textContent = "Ngày bắt đầu phải trước hoặc bằng ngày kết thúc";
      createTaskMessage.style.color = "red";
      return;
    }

    try {
      const resp = await fetch("http://localhost:3000/createTask", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          projectID,       // Ensure this variable is defined elsewhere in your code
          taskName,
          taskDescription,
          memberEmail: assignedTo,
          timeStart,
          timeEnd,
          token,           // Ensure this variable is defined elsewhere in your code
        }),
      });
      const result = await resp.json();
      if (result.taskID) {
        createTaskMessage.textContent = `Tạo công việc thành công (ID: ${result.taskID})`;
        createTaskMessage.style.color = "green";
        setTimeout(() => {
          createTaskMessage.textContent = "";
          newTaskNameInput.value = "";
          newTaskDescriptionInput.value = "";
          timeStartInput.value = ""; // Reset
          timeEndInput.value = "";   // Reset
          document.getElementById("assignTo").value = "";
          fetchAndDisplayTasks();
        }, 1000);
      } else {
        createTaskMessage.textContent = `Tạo công việc thất bại: ${result.error || "Lỗi không xác định"}`;
        createTaskMessage.style.color = "red";
      }
    } catch (e) {
      console.error(e);
      createTaskMessage.textContent = "Lỗi kết nối đến máy chủ";
      createTaskMessage.style.color = "red";
    }
  });

  cancelTaskBtn.addEventListener("click", () => {
    newTaskNameInput.value = "";
    newTaskDescriptionInput.value = "";
    timeStartInput.value = ""; // Reset
    timeEndInput.value = "";   // Reset
    document.getElementById("assignTo").value = "";
    createTaskMessage.textContent = "";
  });

  // Xử lý danh sách công việc
  const tasksList = document.getElementById("tasksList");
  tasksList.addEventListener("click", (e) => {
    const li = e.target.closest("li");
    if (li && li.dataset.taskId) {
      const taskID = li.dataset.taskId;
      window.location.href = `task_detail.html?taskID=${taskID}`;
    }
  });

  // Xử lý Mời Thành viên
  const inviteBtn = document.getElementById("inviteBtn");
  const inviteEmailInput = document.getElementById("inviteEmail");
  const inviteResult = document.getElementById("inviteResult");

  inviteBtn.addEventListener("click", async () => {
    const email = inviteEmailInput.value.trim();
    if (!email) {
      inviteResult.textContent = "Vui lòng nhập email";
      inviteResult.style.color = "red";
      return;
    }
    try {
      const resp = await fetch("http://localhost:3000/inviteMember", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ projectID, email, token }),
      });
      const result = await resp.json();
      if (result.error) {
        inviteResult.textContent = "Lỗi mời thành viên: " + result.error;
        inviteResult.style.color = "red";
      } else {
        inviteResult.textContent = "Mời thành viên thành công!";
        inviteResult.style.color = "green";
        await fetchAndDisplayMembers();
        populateAssignToDropdown();
        inviteEmailInput.value = "";
      }
    } catch (e) {
      console.error(e);
      inviteResult.textContent = "Lỗi kết nối đến máy chủ";
      inviteResult.style.color = "red";
    }
  });

  // Xử lý nút Chat
  document.getElementById("chatroomBtn").addEventListener("click", () => {
    window.location.href = "chatroom.html?id=" + projectID;
  });

  // -----------------------
  // Định nghĩa Hàm
  // -----------------------

  // Hàm lấy và hiển thị danh sách thành viên
  async function fetchAndDisplayMembers() {
    const membersList = document.getElementById("membersList");
    const assignToSelect = document.getElementById("assignTo");
    try {
      const resp = await fetch("http://localhost:3000/members", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ projectID, token }),
      });
      const result = await resp.json();
      if (result.error) {
        alert("Lỗi khi lấy danh sách thành viên: " + result.error);
        return;
      }
      membersList.innerHTML = "";
      assignToSelect.innerHTML = '<option value="">Người phụ trách</option>';
      if (result.members && result.members.length > 0) {
        projectMembers = result.members;
        result.members.forEach((member) => {
          const li = document.createElement("li");
          li.textContent = `Email: ${member.email}, Tên người dùng: ${member.username}`;
          membersList.appendChild(li);

          const option = document.createElement("option");
          option.value = member.email;
          option.textContent = `${member.username} (${member.email})`;
          assignToSelect.appendChild(option);
        });
      } else {
        const li = document.createElement("li");
        li.textContent = "Không tìm thấy thành viên nào.";
        membersList.appendChild(li);
      }
    } catch (e) {
      console.error(e);
      alert("Lỗi khi lấy danh sách thành viên.");
    }
  }

  // Hàm lấy và hiển thị danh sách công việc với Gantt chart
  async function fetchAndDisplayTasks() {
    const tasksList = document.getElementById("tasksList");
    const ganttContainer = document.getElementById("gantt");

    try {
      const resp = await fetch("http://localhost:3000/viewTasks", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ projectID, token }),
      });
      const result = await resp.json();
      if (result.error) {
        alert("Lỗi khi lấy danh sách công việc: " + result.error);
        return;
      }
      console.log(result);
      tasksList.innerHTML = "";
      ganttContainer.innerHTML = ""; // Clear previous Gantt chart

      if (result.tasks && result.tasks.length > 0) {
        // Prepare data for Frappe Gantt
        const ganttTasks = result.tasks.map((task) => {
          // Map status to progress percentage and color
          let progress = 0;
          let color = "#7cd6fd"; // Default color for "not_started"

          switch (task.status.toLowerCase()) {
            case "not_started":
              progress = 0;
              color = "#7cd6fd"; // Blue
              break;
            case "in progress":
              progress = 50;
              color = "#f2c744"; // Yellow
              break;
            case "completed":
              progress = 100;
              color = "#81d742"; // Green
              break;
            default:
              progress = 0;
              color = "#7cd6fd";
          }

          return {
            id: task.id.toString(),
            name: task.name,
            start: task.taskStartDate,
            end: task.taskEndDate,
            progress: progress,
            dependencies: "", // Add dependencies if any
            custom_class: "", // Optional: Add custom classes for styling
            color: color, // Custom color based on status
          };
        });

        // Render Gantt Chart
        const gantt = new Gantt("#gantt", ganttTasks, {
          on_click: (task) => {
            window.location.href = `task_detail.html?taskID=${task.id}`;
          },
          on_date_change: (task, start, end) => {
            console.log(task, start, end);
          },
          on_progress_change: (task, progress) => {
            console.log(task, progress);
          },
          on_view_change: (mode) => {
            console.log(mode);
          },
          custom_popup_html: (task) => {
            return `
              <div class="details-container">
                <h5>${task.name}</h5>
                <p>Start: ${task.start}</p>
                <p>End: ${task.end}</p>
                <p>Progress: ${task.progress}%</p>
              </div>
            `;
          },
        });

        // Display tasks in the list
        result.tasks.forEach((task) => {
          const li = document.createElement("li");
          li.innerHTML = `
            <div class="task-info">
              <strong>${task.name}</strong>
              <span class="task-id">ID: ${task.id}</span>
              <span>${task.taskStartDate} - ${task.taskEndDate}</span>
              <p>Trạng thái: ${task.status}</p>
              <p>Phân công cho: ${task.assignedTo}</p>
            </div>
          `;
          li.dataset.taskId = task.id;
          tasksList.appendChild(li);
        });
      } else {
        tasksList.innerHTML = "<li>Không tìm thấy công việc nào.</li>";
        ganttContainer.innerHTML = "<p>Không có công việc nào để hiển thị trên Gantt chart.</p>";
      }
    } catch (e) {
      console.error(e);
      alert("Lỗi khi lấy danh sách công việc.");
    }
  }

  // Function to populate Assign To dropdown (if needed)
  function populateAssignToDropdown() {
    const assignToSelect = document.getElementById("assignTo");
    assignToSelect.innerHTML = '<option value="">Người phụ trách</option>';
    projectMembers.forEach((member) => {
      const option = document.createElement("option");
      option.value = member.email;
      option.textContent = `${member.username} (${member.email})`;
      assignToSelect.appendChild(option);
    });
  }
});
