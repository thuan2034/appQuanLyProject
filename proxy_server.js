const express = require("express");
const cors = require("cors");
const multer = require("multer");
const path = require("path");
const fs = require("fs");
const EventEmitter = require("events");

const CServerClient = require("./cServerClient"); // Adjust the path as necessary

const app = express();
app.use(cors());
app.use(express.json());
// Configure multer to store files in memory
const storage = multer.memoryStorage();
const upload = multer({
  storage: storage,
  limits: { fileSize: 50 * 1024 * 1024 }, // 50MB limit, adjust as needed
});
// Initialize the persistent connection
const cServer = new CServerClient("127.0.0.1", 6600);

// Event Emitter to handle messages from C server
const messageEmitter = new EventEmitter();

// Listen for messages from the C server and emit events
cServer.on("newMessage", (msg) => {
  // Emit a 'newMessage' event with the message details
  messageEmitter.emit("newMessage", msg);
});


// Optional: Handle global connection events
cServer.on("connected", () => {
  console.log("Persistent connection established with C server.");
});

cServer.on("disconnected", () => {
  console.warn("Persistent connection lost. Attempting to reconnect...");
});

cServer.on("error", (err) => {
  console.error("Persistent connection error:", err);
});

// Utility function to send commands to the C server
function sendCommandToCServer(command) {
  return cServer.sendCommand(command);
}

// -----------------------
// SSE Stream Endpoint
// -----------------------
app.get("/stream", (req, res) => {
  const { projectID, token } = req.query;
  if (!projectID || !token) {
      res.status(400).send("projectID and token are required");
      return;
  }

  // Set headers for SSE
  res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive',
  });

  res.write('\n');

  // Define a listener function
  const onMessage = (msg) => {
      // Check if the message is for the current projectID
      if (msg.projectID === parseInt(projectID, 10)) {
          // Send the message to the client
          const data = {
              username: msg.username,
              content: msg.content,
              time: msg.time, // Use server-side timestamp
          };
          console.log(data);
          res.write(`data: ${JSON.stringify(data)}\n\n`);
      }
  };

  // Register the listener
  messageEmitter.on('newMessage', onMessage);

  // Handle client disconnect
  req.on('close', () => {
      messageEmitter.removeListener('newMessage', onMessage);
      res.end();
  });
});
// Route đăng nhập
app.post("/login", async (req, res) => {
  console.log(req.body);
  const { email, password } = req.body;
  const command = `LOG<${email}><${password}>`;
  // Giả sử format này khớp với client_handler.c
  try {
    const response = await sendCommandToCServer(command);
    res.json({ response });
    console.log(response);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});

// Route đăng ký
app.post("/register", async (req, res) => {
  const { email, username, password } = req.body;
  const command = `REG<${email}><${username}><${password}>`;
  try {
    const response = await sendCommandToCServer(command);
    res.json({ response });
    console.log(response);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});

// Bạn có thể thêm các route khác tương tự cho: tạo dự án, thêm task, xem project…

app.post("/projects", async (req, res) => {
  const { token } = req.body;
  const command = `PRJ<${token}>`;
  try {
    const response = await sendCommandToCServer(command);
    // response dạng: "200 <1 test project1><2 website redesign>"
    console.log(response);
    if (response.startsWith("200")) {
      // Bỏ phần '200 ' ở đầu
      const content = response.replace(/^200\s*/, "");
      // content: "<1 test project1><2 website redesign>"

      // Dùng RegExp để tìm tất cả các cặp <... ...>
      // Mẫu: <(\d+)\s([^>]+)>
      // (\d+) bắt số ID, ([^>]+) bắt tất cả ký tự đến khi gặp '>'
      const regex = /<(\d+)\s([^>]+)>/g;
      let match;
      const projects = [];

      while ((match = regex.exec(content)) !== null) {
        const id = match[1];
        const name = match[2];
        projects.push({ id, name });
      }

      res.json({ projects });
    } else {
      res.status(400).json({ error: "Failed to get projects", response });
    }
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});
app.post("/projectDetail", async (req, res) => {
  const { projectID, token } = req.body;
  const command = `PRD<${projectID}><${token}>`;
  try {
    const response = await sendCommandToCServer(command);
    // response dạng: "200 <ProjectName><CreatedBy><Description>"
    console.log(response);
    if (response.startsWith("200")) {
      // Bỏ "200 "
      const content = response.replace(/^200\s*/, "");
      // content: "<ProjectName><CreatedBy><Description>"

      // Tách các cặp <...>
      const regex = /<([^>]*)>/g;
      let match;
      const parts = [];
      while ((match = regex.exec(content)) !== null) {
        parts.push(match[1]);
      }
      // parts = [ProjectName, CreatedBy, Description]
      if (parts.length === 3) {
        const name = parts[0];
        const createdBy = parts[1];
        const description = parts[2] || "No description available";

        res.json({
          name,
          createdBy,
          description,
        });
      } else {
        res
          .status(500)
          .json({ error: "Invalid response format from server C" });
      }
    } else {
      res.status(400).json({ error: "Failed to get project detail", response });
    }
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});
app.post("/viewTasks", async (req, res) => {
  const { projectID, token } = req.body;
  const command = `VTL<${projectID}><${token}>`;
  try {
    const response = await sendCommandToCServer(command);
    if (response.startsWith("200")) {
      // response dạng: "200<taskID><taskName><taskStatus><Assignedto><taskID2>..."
      // Tách theo pattern `<...>` tương tự như trước
      const content = response.replace(/^200\s*/, "");
      console.log(content);
      // Mỗi task: <id name status taskStartDate taskEndDate assignedTo >
      // Dùng regex:
      const regex = /<([^>]+)><([^>]+)><([^>]+)><([^>]+)><([^>]+)><([^>]+)>/g;
      let match;
      const tasks = [];
      while ((match = regex.exec(content)) !== null) {
        const [full, id, name, status, taskStartDate, taskEndDate, assignedTo] = match;
        tasks.push({ id, name, status, taskStartDate, taskEndDate, assignedTo });
      }
      res.json({ tasks });
    } else {
      res.status(400).json({ error: "Failed to get tasks", response });
    }
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});

app.post("/inviteMember", async (req, res) => {
  const { projectID, email, token } = req.body;
  const command = `INV<${projectID}><${email}><${token}>`;
  try {
    const response = await sendCommandToCServer(command);
    if (response.includes("200")) {
      res.json({ message: "Invitation successful" });
    } else {
      res.status(400).json({ error: "Failed to invite member", response });
    }
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});
app.post("/createProject", async (req, res) => {
  const { projectName, projectDescription, token } = req.body;
  const command = `PRO<${projectName}><${projectDescription}><${token}>`;
  try {
    const response = await sendCommandToCServer(command);
    // response: "200 <projectID>"
    if (response.startsWith("200")) {
      // Parse projectID
      const match = response.match(/<(\d+)>/);
      if (match && match[1]) {
        const projectID = match[1];
        res.json({ projectID });
      } else {
        res.status(400).json({ error: "Invalid response format from server" });
      }
    } else {
      res.status(400).json({ error: "Failed to create project", response });
    }
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});

app.post("/logout", async (req, res) => {
  const { token } = req.body;
  const command = `OUT<${token}>`;
  try {
    const response = await sendCommandToCServer(command);
    // response: "200 <Logout successful>" nếu thành công
    if (response.includes("200")) {
      res.json({ success: true });
    } else {
      res.status(400).json({ error: "Failed to logout", response });
    }
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "Internal server error" });
  }
});
// Route to get member list of a project
app.post("/members", async (req, res) => {
  const { projectID, token } = req.body;

  if (!projectID || !token) {
    return res.status(400).json({ error: "projectID and token are required" });
  }

  const command = `MEM<${projectID}><${token}>`;

  try {
    const response = await sendCommandToCServer(command);
    // Expected response format: "200 <email><username><email><username>..."
    console.log("C Server Response for MEMBERS:", response);

    if (response.startsWith("200")) {
      const content = response.replace(/^200\s*/, ""); // Remove '200 ' prefix

      // Regex to match <email><username>
      const regex = /<([^>]+)><([^>]+)>/g;
      let match;
      const members = [];

      while ((match = regex.exec(content)) !== null) {
        const email = match[1];
        const username = match[2];
        members.push({ email, username });
      }

      res.json({ members });
    } else {
      res.status(400).json({ error: "Failed to retrieve members", response });
    }
  } catch (err) {
    console.error("Error in /members route:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});
app.post("/createTask", async (req, res) => {
  const { projectID, taskName, taskDescription, memberEmail,timeStart,timeEnd, token } = req.body;

  if (!projectID || !taskName || !taskDescription || !memberEmail || !timeStart || !timeEnd || !token) {
    return res.status(400).json({
      error: "projectID, taskName,taskDescription, memberEmail,timeStart,timeEnd, and token are required",
    });
  }

  const command = `TSK<${projectID}><${taskName}><${taskDescription}><${memberEmail}><${timeStart}><${timeEnd}><${token}>`;

  try {
    const response = await sendCommandToCServer(command);
    // Expected response format: "200 <taskID>"
    console.log("Create Task Response:", response);
    if (response.startsWith("200")) {
      const match = response.match(/<(\d+)>/);
      if (match && match[1]) {
        const taskID = match[1];
        res.json({ taskID });
      } else {
        res
          .status(400)
          .json({ error: "Invalid response format from server C" });
      }
    } else {
      res.status(400).json({ error: "Failed to create task", response });
    }
  } catch (err) {
    console.error("Error in /createTask route:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});

// Route to get task detail
app.post("/taskDetail", async (req, res) => {
  const { taskID, token } = req.body;

  if (!taskID || !token) {
    return res.status(400).json({ error: "taskID and token are required" });
  }

  const command = `VOT<${taskID}><${token}>`;

  try {
    const response = await sendCommandToCServer(command);
    // Expected response format:
    // 200 <33><task 1><In Progress><2024-12-10 08:15:48.111555><phuc1@gmail.com><baocao.txt><[1][thuan][test comment][2024-12-13 22:23:38.288915][2][thuan][test comment 2][2024-12-13 22:23:38.288915]>

    console.log("Task Detail Response:", response);

    if (response.startsWith("200")) {
      const content = response.replace(/^200\s*/, "");
      const regex = /<([^>]*)>/g;
      let match;
      const parts = [];
      while ((match = regex.exec(content)) !== null) {
        parts.push(match[1]);
      }

      // Ensure at least 8 parts are present
      if (parts.length < 8) {
        return res
          .status(500)
          .json({ error: "Incomplete response from server C" });
      }

      const [
        id,
        name,
        status,
        timeStart,
        timeEnd,
        assignedTo,
        attachmentsRaw,
        description,
      ] = parts;

      // Parse attachments
      const attachments = attachmentsRaw ? attachmentsRaw.split("|") : [];
      res.json({
        id,
        name,
        status,
        timeStart,
        timeEnd,
        assignedTo,
        attachments,
        description,
      });
    } else {
      res.status(400).json({ error: "Failed to get task detail", response });
    }
  } catch (err) {
    console.error("Error in /taskDetail route:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});
app.post("/getComments", async (req, res) => {
  const { taskID, offset, token } = req.body;

  // Input Validation
  if (
    taskID === undefined ||
    offset === undefined ||
    token === undefined ||
    taskID === null ||
    offset === null ||
    token === null
  ) {
    return res
      .status(400)
      .json({ error: "taskID, offset, and token are required" });
  }

  // Additional validation to ensure taskID and offset are integers
  const parsedTaskID = parseInt(taskID, 10);
  const parsedOffset = parseInt(offset, 10);

  if (isNaN(parsedTaskID) || isNaN(parsedOffset)) {
    return res
      .status(400)
      .json({ error: "taskID and offset must be valid integers" });
  }

  // Construct the command string
  const command = `VCM<${parsedTaskID}><${parsedOffset}><${token}>`;

  try {
    // Send the command to the C server
    const response = await sendCommandToCServer(command);

    console.log("Get Comments Response:", response);

    // Check if the response starts with "200"
    if (response.startsWith("200")) {
      // Remove the status code prefix
      const content = response.replace(/^200\s*/, "");

      // Handle the no comments case
      if (content === "<>") {
        return res.json({ comments: [] });
      }

      // Use regex to extract all fields within angle brackets
      const regex = /<([^>]*)>/g;
      let match;
      const parts = [];

      while ((match = regex.exec(content)) !== null) {
        parts.push(match[1]);
      }

      // Each comment consists of 4 parts: commentID, username, content, time
      if (parts.length % 4 !== 0) {
        return res.status(500).json({
          error: "Malformed comments data received from the server",
        });
      }

      // Group the parts into comment objects
      const comments = [];

      for (let i = 0; i < parts.length; i += 4) {
        const [commentID, username, content, time] = parts.slice(i, i + 4);

        // Optional: Validate individual fields if necessary
        comments.push({
          commentID: commentID.trim(),
          username: username.trim(),
          content: content.trim(),
          time: time.trim(),
        });
      }

      // Send the comments as a JSON response
      return res.json({ comments });
    } else {
      // Handle non-200 responses
      return res.status(400).json({
        error: "Failed to retrieve comments from the server",
        response,
      });
    }
  } catch (err) {
    console.error("Error in /getComments route:", err);
    return res.status(500).json({ error: "Internal server error" });
  }
});
app.post("/updateStatus", async (req, res) => {
  const { taskID, status, token } = req.body;

  if (!taskID || !status || !token) {
    return res
      .status(400)
      .json({ error: "taskID, status, and token are required" });
  }

  // Fetch task details to verify assignedTo
  const getTaskDetailCommand = `VOT<${taskID}><${token}>`;

  try {
    const taskDetailResponse = await sendCommandToCServer(getTaskDetailCommand);
    // Expected response format:
    // 200 <33><task 1><In Progress><2024-12-10 08:15:48.111555><phuc1@gmail.com><baocao.txt><[1][thuan][test comment][2024-12-13 22:23:38.288915][2][thuan][test comment 2][2024-12-13 22:23:38.288915]>

    console.log("Update Status - Task Detail Response:", taskDetailResponse);

    if (taskDetailResponse.startsWith("200")) {
      const content = taskDetailResponse.replace(/^200\s*/, "");
      const regex = /<([^>]*)>/g;
      let match;
      const parts = [];
      while ((match = regex.exec(content)) !== null) {
        parts.push(match[1]);
      }

      // Ensure at least 6 parts are present
      if (parts.length < 6) {
        return res
          .status(500)
          .json({ error: "Incomplete task detail response from server C" });
      }

      const assignedToEmail = parts[4];

      // Here, we proceed to update the status
      const updateStatusCommand = `STT<${taskID}><${status}><${token}>`;

      const updateResponse = await sendCommandToCServer(updateStatusCommand);
      // Expected response: "200 <Task status updated successfully>"

      console.log("Update Status Response:", updateResponse);

      if (updateResponse.startsWith("200")) {
        res.status(200).json({ message: "Task status updated successfully." });
      } else {
        res.status(400).json({
          error: "Failed to update task status",
          response: updateResponse,
        });
      }
    } else {
      res.status(400).json({
        error: "Failed to fetch task details",
        response: taskDetailResponse,
      });
    }
  } catch (err) {
    console.error("Error in /updateStatus route:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});

// Route to add a comment
app.post("/addComment", async (req, res) => {
  const { taskID, comment, token } = req.body;

  if (!taskID || !comment || !token) {
    return res
      .status(400)
      .json({ error: "taskID, comment, and token are required" });
  }

  const addCommentCommand = `CMT<${taskID}><${comment}><${token}>`;

  try {
    const addCommentResponse = await sendCommandToCServer(addCommentCommand);
    // Expected response: "200 <Comment added successfully>"

    console.log("Add Comment Response:", addCommentResponse);

    if (addCommentResponse.startsWith("200")) {
      res.status(200).json({ message: "Comment added successfully." });
    } else {
      res
        .status(400)
        .json({ error: "Failed to add comment", response: addCommentResponse });
    }
  } catch (err) {
    console.error("Error in /addComment route:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});
app.post("/uploadAttachment", upload.single('attachment'), async (req, res) => {
  const authToken = req.body.token;
  const taskID = req.body.taskID;
  const file = req.file;

  if (!authToken || !taskID || !file) {
    return res.status(400).json({ error: "Missing token, taskID, or file" });
  }

  const fileName = file.originalname;
  const fileSize = file.size;

  // Formulate ATH command
  const command = `ATH<${taskID}><${fileName}><${authToken}><${fileSize}>`;

  try {
    // Send ATH command and raw file data to C server
    const finalResponse = await cServer.sendCommandWithRawData(command, file.buffer);
    
    // Check final response
    if (finalResponse.startsWith("200")) {
      res.json({ message: "Attachment uploaded successfully." });
    } else {
      res.status(500).json({ error: `C server error: ${finalResponse}` });
    }
  } catch (err) {
    console.error("Error uploading attachment:", err);
    res.status(500).json({ error: "Failed to upload attachment." });
  }
});

// Route to serve attachments
app.get("/attachments/:filename", async (req, res) => {
  const authToken = req.headers['authorization'] || req.query.token;
  const filename = req.params.filename;

  if (!authToken || !filename) {
    return res.status(400).json({ error: "Missing token or filename" });
  }

  // Since security checks are neglected, proceed to serve the file

  const filePath = path.join(__dirname, 'uploads', filename); // Adjust the path as needed

  fs.access(filePath, fs.constants.F_OK, (err) => {
    if (err) {
      return res.status(404).json({ error: "File not found" });
    }
    res.sendFile(filePath);
  });
});
app.get("/downloadAttachment", async (req, res) => {
  const { taskID, fileName, token } = req.query;

  if (!taskID || !fileName || !token) {
    return res.status(400).json({ error: "Missing taskID, fileName, or token" });
  }

  try {
    // Initiate the download command
    const fileData = await cServer.downloadFile(taskID, fileName, token);

    // Set headers to prompt file download in the browser
    res.setHeader('Content-Disposition', `attachment; filename="${fileName}"`);
    res.setHeader('Content-Type', 'application/octet-stream');

    // Send the binary file data
    res.send(fileData);
  } catch (err) {
    console.error("Error downloading attachment:", err);
    res.status(500).json({ error: "Failed to download attachment" });
  }
});



app.post("/chatHistory", async (req, res) => {
  const { projectID, token, offset } = req.body;

  if (!projectID || !token || offset === undefined) {
    return res.status(400).json({ error: "projectID, token, and offset are required" });
  }

  // Limit is hardcoded to 10
  const limit = 10;

  // Construct the VCH command
  const command = `VCH<${projectID}><${token}><${limit}><${offset}>`;

  try {
    const response = await sendCommandToCServer(command);
    // Expected response format: "200 <length1>message1<length2>message2..."

    if (response.startsWith("200")) {
      const content = response.replace(/^200\s*/, "");

      // Parse messages using RegExp
      const regex = /<(\d+)>([^<]+)/g;
      let match;
      const messages = [];

      while ((match = regex.exec(content)) !== null) {
        const length = parseInt(match[1], 10);
        const message = match[2].trim();

        // message format: msgID|username|time|content
        const [msgID, username, time, ...contentParts] = message.split("|");
        const contentText = contentParts.join("|"); // In case '|' appears in content

        messages.push({
          msgID,
          username,
          time,
          content: contentText,
        });
      }

      res.json({ messages });
    } else {
      res.status(400).json({ error: "Failed to fetch chat history", response });
    }
  } catch (err) {
    console.error("Error in /chatHistory route:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});

app.post("/sendMessage", async (req, res) => {
  const { projectID, token, content } = req.body;

  if (!projectID || !token || !content) {
    return res.status(400).json({ error: "projectID, token, and content are required" });
  }

  // Construct the VSM (View Send Message) command
  const command = `MSG<${projectID}><${token}><${content}>`; // Ensure this command matches your C server's expected format

  try {
    const response = await sendCommandToCServer(command);
    // Expected response: "200 <Message sent successfully>" or "400 <Error message>"

    if (response.startsWith("200")) { 
      res.json({ message: "Message sent successfully" });
    } else {
      res.status(400).json({ error: "Failed to send message", response });
    }
  } catch (err) {
    console.error("Error in /sendMessage route:", err);
    res.status(500).json({ error: "Internal server error" });
  }
});

app.listen(3000, () => {
  console.log("Node.js server listening on http://localhost:3000");
});

// -----------------------
// Join Chatroom Endpoint
// -----------------------
app.post("/join", async (req, res) => {
  const { projectID, token } = req.body;

  if (projectID === undefined || token === undefined) {
      return res.status(400).json({ error: "projectID and token are required" });
  }

  // Construct the JOIN command
  const command = `JCH<${projectID}><${token}>`;

  try {
      const response = await sendCommandToCServer(command);
      // Expected response: "200 <Success message>" or "400 <Error message>"

      if (response.startsWith("200")) {
          res.json({ success: true, message: "Joined chatroom successfully" });
      } else {
          res.status(400).json({ error: "Failed to join chatroom", response });
      }
  } catch (err) {
      console.error("Error in /join route:", err);
      res.status(500).json({ error: "Internal server error" });
  }
});

// proxy_server.js

// ... existing code

// -----------------------
// Leave Chatroom Endpoint
// -----------------------
app.post("/leave", async (req, res) => {
  const { projectID, token } = req.body;

  if (projectID === undefined || token === undefined) {
      return res.status(400).json({ error: "projectID and token are required" });
  }

  // Construct the LEAVE command
  const command = `LCH<${projectID}><${token}>`;

  try {
      const response = await sendCommandToCServer(command);
      // Expected response: "200 <Success message>" or "400 <Error message>"

      if (response.startsWith("200")) {
          res.json({ success: true, message: "Left chatroom successfully" });
      } else {
          res.status(400).json({ error: "Failed to leave chatroom", response });
      }
  } catch (err) {
      console.error("Error in /leave route:", err);
      res.status(500).json({ error: "Internal server error" });
  }
});

// ... existing code to listen on port 3000
