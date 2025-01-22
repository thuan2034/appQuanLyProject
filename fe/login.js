// login.js

document.addEventListener("DOMContentLoaded", () => {
  const loginForm = document.getElementById("loginForm");
  const errorMessage = document.getElementById("errorMessage");

  loginForm.addEventListener("submit", async (e) => {
    e.preventDefault(); // Prevent form from submitting the default way

    const email = document.getElementById("email").value.trim();
    const password = document.getElementById("password").value.trim();

    if (!email || !password) {
      errorMessage.textContent = "Please enter both email and password.";
      return;
    }

    try {
      const response = await fetch("http://localhost:3000/login", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ email, password }),
      });

      const result = await response.json();

      if (response.ok) {
        // Assuming the server returns { response: "200 <AuthToken><username>" }
        const responseMatch = result.response.match(/200\s*<([^>]+)><([^>]+)>/);
        if (responseMatch && responseMatch[1] && responseMatch[2]) {
          const authToken = responseMatch[1]; // Extract the AuthToken
          const username = responseMatch[2];
          // Store the authToken and email in localStorage
          localStorage.setItem("authToken", authToken);
          localStorage.setItem("userEmail", email);
          localStorage.setItem("username", username);
          console.log("Authentication successful");
          console.log(`AuthToken: ${authToken}`);
          console.log(`Username: ${username}`);
          // Redirect to the projects page
          window.location.href = "projects.html";
        } else {
          errorMessage.textContent = "Invalid response from server.";
        }
      } else {
        // Handle server errors
        errorMessage.textContent =
          result.error || "Login failed. Please try again.";
      }
    } catch (error) {
      console.error("Error during login:", error);
      errorMessage.textContent = "An error occurred. Please try again later.";
    }
  });
});
