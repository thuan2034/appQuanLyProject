// chatroom.js
document.addEventListener("DOMContentLoaded", async () => {
  // -----------------------
  // Authentication & Initialization
  // -----------------------
  const token = localStorage.getItem("authToken");
  const currentUsername = localStorage.getItem("username") || "You"; // Retrieve current username
  const projectID = window.location.search.split("=")[1];
  if (!token) {
    alert("No token found. Please login again.");
    window.location.href = "index.html";
    return;
  }

  if (!projectID) {
    alert("No project ID found.");
    window.location.href = "projects.html";
    return;
  }

  // -----------------------
  // Initialize SSE for Real-time Updates
  // -----------------------
  const eventSource = new EventSource(
    `http://localhost:3000/stream?projectID=${projectID}&token=${token}`
  );

  eventSource.onmessage = function (event) {
    const data = JSON.parse(event.data);
    const newMessage = {
      msgID: "new", // Handle appropriately if msgID is returned
      username: data.username,
      time: data.time, // Use the time sent by server or frontend
      content: data.content,
    };
    appendMessage(newMessage, true);
  };

  eventSource.onerror = function (err) {
    console.error("EventSource failed:", err);
    // Optionally, implement reconnection logic or notify the user
  };

  // -----------------------
  // Chat Initialization
  // -----------------------

  let chatOffset = 0; // Starting point
  const chatLimit = 10;
  let isFetchingChat = false;
  let allChatFetched = false;

  const chatMessagesContainer = document.getElementById("chatMessages");
  const chatInput = document.getElementById("chatInput");
  const sendChatBtn = document.getElementById("sendChatBtn");

  // -----------------------
  // Join Chatroom
  // -----------------------
  try {
    const joinResponse = await fetch("http://localhost:3000/join", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        projectID,
        token,
      }),
    });
    const joinResult = await joinResponse.json();
    if (!joinResponse.ok) {
      alert(
        "Failed to join chatroom: " + (joinResult.error || "Unknown error")
      );
      window.location.href = "projects.html";
      return;
    }
  } catch (err) {
    console.error("Error joining chatroom:", err);
    alert("Error connecting to server");
    window.location.href = "projects.html";
    return;
  }

  // Fetch initial chat messages
  await fetchChatHistory();

  // -----------------------
  // Event Listeners
  // -----------------------



  // Handle sending chat messages
  sendChatBtn.addEventListener("click", async () => {
    const messageContent = chatInput.value.trim();
    if (messageContent === "") return;

    try {
      const resp = await fetch("http://localhost:3000/sendMessage", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          projectID,
          token,
          content: messageContent,
        }),
      });
      const result = await resp.json();

      if (resp.ok) {
        const newMessage = {
          msgID: "new", // Handle appropriately if msgID is returned
          username: currentUsername,
          time: new Date().toLocaleString(),
          content: messageContent,
        };
        appendMessage(newMessage, true);
        chatInput.value = "";
        chatMessagesContainer.scrollTop = chatMessagesContainer.scrollHeight;
      } else {
        alert("Failed to send message: " + (result.error || "Unknown error"));
      }
    } catch (err) {
      console.error("Error sending message:", err);
      alert("Error sending message");
    }
  });

  // Allow pressing 'Enter' key to send messages
  chatInput.addEventListener("keypress", async (e) => {
    if (e.key === "Enter") {
      e.preventDefault();
      sendChatBtn.click();
    }
  });

  // Implement scroll listener to fetch older messages when scrolled to top
  chatMessagesContainer.addEventListener("scroll", async () => {
    if (
      chatMessagesContainer.scrollTop === 0 &&
      !isFetchingChat &&
      !allChatFetched
    ) {
      await fetchChatHistory();
    }
  });

  // Handle Return to Project Button
  document.getElementById("returnBtn").addEventListener("click", async () => {
    await leaveChatroom();
    window.location.href = `project_detail.html?id=${projectID}`;
  });

  // Handle Logout Button
  document.getElementById("logoutBtn").addEventListener("click", async () => {
    await leaveChatroom();
    try {
      const resp = await fetch("http://localhost:3000/logout", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ token }),
      });
      const result = await resp.json();
      if (result.success) {
        alert("Logout successful");
        localStorage.removeItem("authToken");
        window.location.href = "index.html";
      } else {
        alert("Logout failed: " + (result.error || "Unknown error"));
      }
    } catch (e) {
      console.error(e);
      alert("Error connecting to server");
    }
  });
  // -----------------------
  // Fetch & Display Chat History
  // -----------------------

  async function fetchChatHistory() {
    if (isFetchingChat || allChatFetched) return;
    isFetchingChat = true;

    try {
      const resp = await fetch("http://localhost:3000/chatHistory", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          projectID,
          token,
          offset: chatOffset,
        }),
      });
      const result = await resp.json();

      if (resp.ok) {
        const messages = result.messages;

        if (messages.length < chatLimit) {
          allChatFetched = true;
        }

        chatOffset += messages.length;

        messages.reverse().forEach((message) => {
          appendMessage(message, false);
        });

        const previousScrollHeight = chatMessagesContainer.scrollHeight;
        chatMessagesContainer.scrollTop =
          chatMessagesContainer.scrollHeight - previousScrollHeight;
      } else {
        alert(
          "Failed to fetch chat history: " + (result.error || "Unknown error")
        );
      }
    } catch (err) {
      console.error("Error fetching chat history:", err);
      alert("Error fetching chat history");
    } finally {
      isFetchingChat = false;
    }
  }

  function appendMessage(message, isNew) {
    const messageDiv = document.createElement("div");
    messageDiv.classList.add("chat-message");

    if (message.username === currentUsername) {
      messageDiv.classList.add("right");
    } else {
      messageDiv.classList.add("left");
    }

    const metaDiv = document.createElement("div");
    metaDiv.classList.add("meta");
    metaDiv.textContent = `${message.username} • ${message.time}`;

    const contentDiv = document.createElement("div");
    contentDiv.classList.add("content");
    contentDiv.textContent = message.content;

    messageDiv.appendChild(metaDiv);
    messageDiv.appendChild(contentDiv);

    if (isNew) {
      chatMessagesContainer.appendChild(messageDiv);
      chatMessagesContainer.scrollTop = chatMessagesContainer.scrollHeight;
    } else {
      chatMessagesContainer.insertBefore(
        messageDiv,
        chatMessagesContainer.firstChild
      );
      const previousScrollHeight = chatMessagesContainer.scrollHeight;
      chatMessagesContainer.scrollTop =
        chatMessagesContainer.scrollHeight - previousScrollHeight;
    }
  }

  async function leaveChatroom() {
    try {
      await fetch("http://localhost:3000/leave", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          projectID,
          token,
        }),
      });
      const result = await response.json();

    if (!result.success) {
      throw new Error(result.error || "Failed to leave chatroom");
    }
      // Optionally handle response
    } catch (err) {
      console.error("Error leaving chatroom:", err);
      // Optionally alert the user
    }
  }
  // Optional: Auto-scroll to bottom on initial load
  chatMessagesContainer.scrollTop = chatMessagesContainer.scrollHeight;
});
