// task_detail.js

document.addEventListener("DOMContentLoaded", async () => {
  const authToken = localStorage.getItem("authToken");
  const userEmail = localStorage.getItem("userEmail");

  if (!authToken || !userEmail) {
    alert("Không tìm thấy token. Vui lý đăng nhập lại.");
    window.location.href = "index.html";
    return;
  }

  // Get taskID from URL
  const urlParams = new URLSearchParams(window.location.search);
  const taskID = urlParams.get("taskID");
  if (!taskID) {
    alert("Không có ID dự án được cung cấp.");
    window.location.href = "projects.html";
    return;
  }

  // Initialize comments state
  let currentOffset = 0;
  const COMMENTS_LIMIT = 5;
  let allCommentsLoaded = false;

  // Elements related to comments
  const commentsList = document.getElementById("commentsList");
  const seeMoreCommentsBtn = document.getElementById("seeMoreCommentsBtn");
  const commentsLoadingMessage = document.getElementById("commentsLoadingMessage");
  const commentsErrorMessage = document.getElementById("commentsErrorMessage");

  // Fetch task details (excluding comments)
  try {
    const response = await fetch("http://localhost:3000/taskDetail", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ taskID, token: authToken }),
    });
    const data = await response.json();

    if (data.error) {
      alert("Error getting task detail: " + data.error);
      return;
    }

    document.getElementById("taskID").textContent = data.id;
    document.getElementById("taskName").textContent = data.name;
    document.getElementById("taskStatus").textContent = data.status;
    document.getElementById("taskTimeStart").textContent = data.timeStart;
    document.getElementById("taskTimeEnd").textContent = data.timeEnd;
    document.getElementById("taskAssignedTo").textContent = data.assignedTo;
    document.getElementById("taskDescription").textContent = data.description;

    // Display attachments
    const attachmentsList = document.getElementById("attachmentsList");
    attachmentsList.innerHTML = "";
    if (data.attachments && data.attachments.length > 0) {
      data.attachments.forEach((att) => {
        const li = document.createElement("li");
        const link = document.createElement("a");
        link.href = `http://localhost:3000/downloadAttachment?taskID=${encodeURIComponent(taskID)}&fileName=${encodeURIComponent(att)}&token=${encodeURIComponent(authToken)}`;
        link.textContent = att;
        link.target = "_blank";
        link.download = att;
        li.appendChild(link);
        attachmentsList.appendChild(li);
      });
    } else {
      attachmentsList.innerHTML = "<li>Không có tệp đính kèm.</li>";
    }

    // Determine if current user is assigned to this task
    if (userEmail === data.assignedTo) {
      document.getElementById("updateStatusForm").style.display = "block";
      document.getElementById("uploadAttachmentForm").style.display = "block"; // Show upload form
    } else {
      document.getElementById("updateStatusForm").style.display = "none";
      document.getElementById("uploadAttachmentForm").style.display = "none"; // Hide upload form
    }

    // Initial fetch of comments
    await fetchComments();

  } catch (err) {
    console.error("Error:", err);
    alert("ELỗi lấy thông tin công việc");
  }

  // Function to fetch comments using /getComments endpoint
  async function fetchComments() {
    if (allCommentsLoaded) return;

    // Show loading message
    commentsLoadingMessage.style.display = "block";
    commentsErrorMessage.style.display = "none";

    try {
      const response = await fetch("http://localhost:3000/getComments", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ taskID, offset: currentOffset, token: authToken }),
      });
      const data = await response.json();

      if (data.error) {
        commentsErrorMessage.textContent = "Lỗi khi tải nhận xét: " + data.error;
        commentsErrorMessage.style.display = "block";
        commentsLoadingMessage.style.display = "none";
        return;
      }

      const { comments } = data;

      if (comments && comments.length > 0) {
        comments.forEach((comment) => {
          const li = document.createElement("li");
          li.innerHTML = `<strong>${escapeHTML(comment.username)}</strong> (${escapeHTML(comment.time)}):<br>${escapeHTML(comment.content)}`;
          commentsList.appendChild(li);
        });

        // If fewer than COMMENTS_LIMIT comments were returned, no more comments to load
        if (comments.length < COMMENTS_LIMIT) {
          allCommentsLoaded = true;
          seeMoreCommentsBtn.style.display = "none";
        } else {
          // There might be more comments
          seeMoreCommentsBtn.style.display = "block";
        }

        // Update the offset for next fetch
        currentOffset += COMMENTS_LIMIT;
      } else {
        if (currentOffset === 0) {
          commentsList.innerHTML = "<li>Không có nhận xét nào.</li>";
        }
        allCommentsLoaded = true;
        seeMoreCommentsBtn.style.display = "none";
      }

    } catch (err) {
      console.error("Lỗi:", err);
      commentsErrorMessage.textContent = "Lỗi khi tải nhận xét";
      commentsErrorMessage.style.display = "block";
    } finally {
      commentsLoadingMessage.style.display = "none";
    }
  }

  // Escape HTML to prevent XSS
  function escapeHTML(str) {
    if (typeof str !== 'string') return '';
    return str.replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")
              .replace(/>/g, "&gt;")
              .replace(/"/g, "&quot;")
              .replace(/'/g, "&#039;");
  }

  // Handle "See More Comments" Button Click
  seeMoreCommentsBtn.addEventListener("click", async () => {
    await fetchComments();
  });

  // Handle Back to Project Button
  const backToProjectBtn = document.getElementById("backToProjectBtn");
  backToProjectBtn.addEventListener("click", () => {
    window.history.back(); // Simple back navigation
  });

  // Handle Logout
  const logoutBtn = document.getElementById("logoutBtn");
  logoutBtn.addEventListener("click", async () => {
    try {
      const resp = await fetch("http://localhost:3000/logout", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ token: authToken }),
      });
      const result = await resp.json();
      if (result.success) {
        alert("Logout successful");
        localStorage.removeItem("authToken");
        localStorage.removeItem("userEmail");
        window.location.href = "index.html";
      } else {
        alert("Logout failed: " + (result.error || "Unknown error"));
      }
    } catch (e) {
      console.error(e);
      alert("Error connecting to server");
    }
  });

  // Handle Update Status
  const submitStatusBtn = document.getElementById("submitStatusBtn");
  const cancelStatusBtn = document.getElementById("cancelStatusBtn");
  const newStatusSelect = document.getElementById("newStatus");
  const updateStatusMessage = document.getElementById("updateStatusMessage");

  submitStatusBtn.addEventListener("click", async () => {
    const newStatus = newStatusSelect.value;
    if (!newStatus) {
      updateStatusMessage.textContent = "Hãy chọn trạng thái";
      updateStatusMessage.style.color = "red";
      return;
    }

    try {
      const resp = await fetch("http://localhost:3000/updateStatus", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ taskID, status: newStatus, token: authToken }),
      });
      const result = await resp.json();
      if (resp.ok) {
        updateStatusMessage.textContent =
          result.message || "Cập nhật trạng thái thành công.";
        updateStatusMessage.style.color = "green";
        // Update the status in the UI
        document.getElementById("taskStatus").textContent = newStatus;
        // Optionally, hide the update status form
        // document.getElementById('updateStatusForm').style.display = 'none';
      } else {
        updateStatusMessage.textContent = `Cập nhật trạng thái thất bại: ${
          result.error || "Unknown error"
        }`;
        updateStatusMessage.style.color = "red";
      }
    } catch (e) {
      console.error(e);
      updateStatusMessage.textContent = "Error connecting to server";
      updateStatusMessage.style.color = "red";
    }
  });

  cancelStatusBtn.addEventListener("click", () => {
    newStatusSelect.value = "";
    updateStatusMessage.textContent = "";
    document.getElementById("updateStatusForm").style.display = "none";
  });

  // Handle Add Comment
  const submitCommentBtn = document.getElementById("submitCommentBtn");
  const newCommentTextarea = document.getElementById("newComment");
  const addCommentMessage = document.getElementById("addCommentMessage");

  newCommentTextarea.addEventListener("keydown", function (e) {
    if (e.key === "Enter") {
      e.preventDefault(); // Prevent the default action (new line)
      submitCommentBtn.click(); // Trigger the comment submission
    }
  });
  newCommentTextarea.addEventListener("input", function () {
    const sanitizedValue = this.value.replace(/(\r\n|\n|\r)/gm, " ");
    if (this.value !== sanitizedValue) {
      this.value = sanitizedValue;
      addCommentMessage.textContent = "New lines are not allowed in comments.";
      addCommentMessage.style.color = "orange";
    } else {
      addCommentMessage.textContent = "";
    }
  });
  submitCommentBtn.addEventListener("click", async () => {
    const comment = newCommentTextarea.value.trim();
    if (!comment) {
      addCommentMessage.textContent = "Điền nhân xét.";
      addCommentMessage.style.color = "red";
      return;
    }

    try {
      const resp = await fetch("http://localhost:3000/addComment", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ taskID, comment, token: authToken }),
      });
      const result = await resp.json();
      if (resp.ok) {
        addCommentMessage.textContent =
          result.message || "Thêm nhận xét thành công.";
        addCommentMessage.style.color = "green";
        newCommentTextarea.value = "";
        // Reset comments and fetch from the beginning
        resetComments();
        await fetchComments();
      } else {
        addCommentMessage.textContent = `Thêm nhận xét thất bại: ${
          result.error || "Unknown error"
        }`;
        addCommentMessage.style.color = "red";
      }
    } catch (e) {
      console.error(e);
      addCommentMessage.textContent = "Error connecting to server";
      addCommentMessage.style.color = "red";
    }
  });

  // Function to reset comments state
  function resetComments() {
    currentOffset = 0;
    allCommentsLoaded = false;
    commentsList.innerHTML = "";
    seeMoreCommentsBtn.style.display = "none";
    commentsErrorMessage.style.display = "none";
  }

  // Handle Upload Attachment Button
  const uploadAttachmentBtn = document.getElementById("uploadAttachmentBtn");
  const attachmentFileInput = document.getElementById("attachmentFile");
  const uploadAttachmentMessage = document.getElementById(
    "uploadAttachmentMessage"
  );

  uploadAttachmentBtn.addEventListener("click", async () => {
    const file = attachmentFileInput.files[0];
    const taskID = document.getElementById("taskID").textContent;

    if (!file) {
      uploadAttachmentMessage.textContent = "Chọn file.";
      uploadAttachmentMessage.style.color = "red";
      return;
    }

    // Prepare form data
    const formData = new FormData();
    formData.append("taskID", taskID);
    formData.append("attachment", file);
    formData.append("token", authToken); // Include the auth token

    try {
      const resp = await fetch("http://localhost:3000/uploadAttachment", {
        method: "POST",
        body: formData,
      });

      const result = await resp.json();
      if (resp.ok) {
        uploadAttachmentMessage.textContent =
          "Đính kèm file thành công.";
        uploadAttachmentMessage.style.color = "green";
        attachmentFileInput.value = ""; // Reset the file input
        // Optionally, refresh the attachments list
        await refreshAttachments();
      } else {
        uploadAttachmentMessage.textContent = `Failed to upload attachment: ${
          result.error || "Unknown error"
        }`;
        uploadAttachmentMessage.style.color = "red";
      }
    } catch (e) {
      console.error(e);
      uploadAttachmentMessage.textContent = "Error connecting to server.";
      uploadAttachmentMessage.style.color = "red";
    }
  });

  // Function to refresh attachments list (optional)
  async function refreshAttachments() {
    try {
      const response = await fetch("http://localhost:3000/taskDetail", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ taskID, token: authToken }),
      });
      const data = await response.json();

      if (data.error) {
        alert("Error refreshing attachments: " + data.error);
        return;
      }

      const attachmentsList = document.getElementById("attachmentsList");
      attachmentsList.innerHTML = "";
      if (data.attachments && data.attachments.length > 0) {
        data.attachments.forEach((att) => {
          const li = document.createElement("li");
          const link = document.createElement("a");
          link.href = `http://localhost:3000/downloadAttachment?taskID=${encodeURIComponent(taskID)}&fileName=${encodeURIComponent(att)}&token=${encodeURIComponent(authToken)}`; // Use secure route
          link.textContent = att;
          link.target = "_blank";
          li.appendChild(link);
          attachmentsList.appendChild(li);
        });
      } else {
        attachmentsList.innerHTML = "<li>Không có tệp đính kèm.</li>";
      }
    } catch (err) {
      console.error("Error:", err);
      alert("Error refreshing attachments");
    }
  }
});
