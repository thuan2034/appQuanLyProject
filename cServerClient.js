const net = require("net");
const EventEmitter = require("events");

class CServerClient extends EventEmitter {
  constructor(host, port) {
    super();
    this.host = host;
    this.port = port;
    this.client = null;
    this.connected = false;
    this.buffer = "";
    this.commandQueue = [];
    this.currentResolve = null;
    this.currentReject = null;
    this.fileDownloadMode = false;
    this.fileBuffer = Buffer.alloc(0);
    this.connect();
  }

  connect() {
    this.client = net.createConnection({ host: this.host, port: this.port }, () => {
      console.log("Connected to C server");
      this.connected = true;
      this.emit("connected");
      this.processQueue();
    });

    this.client.on("data", (data) => {
      // If in fileDownloadMode, handle binary data differently
      if (this.fileDownloadMode) {
        // Accumulate file data
        this.fileBuffer = Buffer.concat([this.fileBuffer, data]);

        // Look for the confirmation message indicating end of file data
        const confirmation = Buffer.from("200 <File downloaded successfully>\n");
        const index = this.fileBuffer.indexOf(confirmation);

        if (index !== -1) {
          // Extract file data before the confirmation
          const fileData = this.fileBuffer.slice(0, index);

          // Resolve the download promise
          if (this.currentResolve) {
            this.currentResolve(fileData);
            this.currentResolve = null;
            this.currentReject = null;
          }

          // Reset download mode
          this.fileDownloadMode = false;
          this.fileBuffer = Buffer.alloc(0);
          this.processQueue();
        }

      } else {
        // Normal (non-download) command handling (old logic)
        this.buffer += data.toString();
        let newlineIndex;
        while ((newlineIndex = this.buffer.indexOf("\n")) !== -1) {
          const line = this.buffer.slice(0, newlineIndex).trim();
          this.buffer = this.buffer.slice(newlineIndex + 1);
          console.log(`Received: ${line}`);
          if (line.startsWith("MSG<")) {
            // Handle broadcast message
            const msg = this.parseBroadcastMessage(line);
            if (msg) {
              this.emit("newMessage", msg);
            } else {
              console.warn(`Failed to parse broadcast message: ${line}`);
            }
          } else {
            // Handle command response
            if (this.currentResolve) {
              this.currentResolve(line);
              this.currentResolve = null;
              this.currentReject = null;
              this.processQueue();
            } else {
              console.warn(`Received unexpected response: ${line}`);
            }
          }
        }
      }
    });

    this.client.on("end", () => {
      console.log("Disconnected from C server");
      this.connected = false;
      this.emit("disconnected");
      setTimeout(() => this.connect(), 1000); // Reconnect logic
    });

    
    this.client.on("error", (err) => {
      console.error("C server connection error:", err);
      this.connected = false;
      this.emit("error", err);
      setTimeout(() => this.connect(), 1000); // Reconnect logic
    });
  }

   
   parseBroadcastMessage(line) {
    const regex = /^MSG<(\d+)><([^>]+)><([^>]+)>$/;
    const match = line.match(regex);
    if (match) {
      const [, projectID, username, content] = match;
      return {
        projectID: parseInt(projectID, 10),
        username,
        content,
        time: new Date().toLocaleString(), // Optionally, you can have the server provide the timestamp
      };
    }
    return null;
  }

  
  sendCommand(command) {
    return new Promise((resolve, reject) => {
      console.log(`Enqueuing command: ${command}`);
      this.commandQueue.push({ command, resolve, reject, isFileDownload: false });
      this.processQueue();
    });
  }

  downloadFile(taskID, fileName, token) {
    return new Promise((resolve, reject) => {
      console.log(`Enqueuing download command: DOW<${taskID}><${fileName}><${token}>`);
      this.commandQueue.push({
        command: `DOW<${taskID}><${fileName}><${token}>`,
        resolve,
        reject,
        isFileDownload: true
      });
      this.processQueue();
    });
  }

  processQueue() {
    if (!this.connected || this.currentResolve) {
      return;
    }

    const next = this.commandQueue.shift();
    if (next) {
      console.log(`Sending command: ${next.command}`);
      this.currentResolve = next.resolve;
      this.currentReject = next.reject;

      // If it's a download command, enable fileDownloadMode
      if (next.isFileDownload) {
        this.fileDownloadMode = true;
      } else {
        this.fileDownloadMode = false;
      }

      this.client.write(next.command + "\n");
    }
  }

  sendRawData(buffer) {
    if (this.connected) {
      this.client.write(buffer);
    } else {
      console.error("Cannot send raw data: not connected to C server.");
    }
  }

  async sendCommandWithRawData(command, rawDataBuffer) {
    try {
      const response1 = await this.sendCommand(command);
      if (!response1.startsWith("200")) {
        throw new Error(`C server error: ${response1}`);
      }

      // Send raw data
      this.sendRawData(rawDataBuffer);

      // Await final confirmation (the old logic expects a newline-based response)
      const response2 = await this.sendCommand(""); 
      if (!response2.startsWith("200")) {
        throw new Error(`C server error after file upload: ${response2}`);
      }

      return response2;
    } catch (err) {
      throw err;
    }
  }

  close() {
    if (this.client) {
      this.client.end();
    }
  }
}

module.exports = CServerClient;
