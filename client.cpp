#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#define PORT 6600
int sock = 0;
char token[32];
char message[2048];
size_t message_size = sizeof(message);
void handle_registration();
void handle_login();
void handle_logout();
int is_valid_email(const char *email);
void clear_input_buffer();
void fetch_projects_list();
void handle_view_project(int projectID);
void handle_create_project(char *project_name, char *project_description);
void handle_invite_member(int projectID, char *email);
void handle_view_project_members(int projectID);
void handle_view_tasks(int projectID);
void handle_create_task(int projectID, char *task_name, char *member_email);
void handle_view_one_task(int projectID, int taskID);
void handle_add_file(int projectID, int taskID, char *file_path);
void handle_add_comment(int taskID, char *comment);
void handle_update_status(int taskID, char *status);
int is_allowed_file_type(const char *file_path);
void handle_chat(int projectID);
void menu();
void home_page();

int main()
{
    struct sockaddr_in serv_addr;
    // Initialize the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Connection failed\n");
        return -1;
    }
    printf("Connected to server\n");
    // Communication loop
    while (1)
    { // check if already login (if token is not null)
        if (token[0] != '\0')
        {
            home_page();
        }
        else // when not login yet display menu
        {
            menu();
        }
    }
    close(sock);
    return 0;
}
void menu()
{
    int choice;
    system("clear");
    printf("═══════════════════════════════════\n");
    printf("            MAIN MENU              \n");
    printf("═══════════════════════════════════\n");
    printf("1. Register                        \n");
    printf("2. Login                           \n");
    printf("-----------------------------------\n");
    printf("Please select an option (1 or 2): ");

    printf("Your choice: ");
    if (scanf("%d", &choice) != 1)
    {
        clear_input_buffer(); // Clear invalid input
        return;               // Restart the loop
    }
    if (choice == 1)
    {
        handle_registration();
    }
    else if (choice == 2)
    {
        handle_login();
    }
}

void home_page()
{
    // system("clear");
    int choice;
    int projectID = 0;
    char project_name[50], project_description[512];
    printf("════════════════════════════════════════════════════════════════════════════════\n");
    printf("                                HOME PAGE                                       \n");
    printf("════════════════════════════════════════════════════════════════════════════════\n");
    // Fetch and display the list of projects
    fetch_projects_list();
    printf("\nOPTIONS:\n");
    printf("-------------------------------------------------------------------------------\n");
    printf("1. Go to a project\n");
    printf("2. Create a new project\n");
    printf("3. Logout\n");
    printf("-------------------------------------------------------------------------------\n");
    printf("Please enter your choice: ");

    if (scanf("%d", &choice) != 1)
    {
        clear_input_buffer(); // Clear invalid input
        return;               // Restart the loop
    }
    switch (choice)
    {
    case 1:
        printf("Enter project ID: ");
        if (scanf("%d", &projectID) != 1)
        {
            clear_input_buffer(); // Clear invalid input
            return;               // Restart the loop
        }
        handle_view_project(projectID);
        break;
    case 2:

        printf("Create Project\n");
        printf("Enter project name: ");
        while (fgets(project_name, sizeof(project_name), stdin) != NULL)
        {
            // Remove the newline character at the end, if present
            project_name[strcspn(project_name, "\n")] = '\0';
            if (strlen(project_name) > 0)
            {
                // printf("Username: %s\n", username);
                break;
            }
        }
        printf("Enter description: ");
        while (fgets(project_description, sizeof(project_description), stdin) != NULL)
        {
            // Remove the newline character at the end, if present
            project_description[strcspn(project_description, "\n")] = '\0';
            if (strlen(project_description) > 0)
            {
                // printf("Username: %s\n", username);
                break;
            }
        }
        handle_create_project(project_name, project_description);
        break;
    case 3:
        handle_logout();
        break;
    default:
        break;
    }
}
void handle_registration()
{
    system("clear");
    char username[50], email[50], password[50];
    printf("═══════════════════════════════════\n");
    printf("           REGISTRATION            \n");
    printf("═══════════════════════════════════\n");
    // Get username
    printf("Enter username: ");
    fgets(username, sizeof(username), stdin);
    while (fgets(username, sizeof(username), stdin) != NULL)
    {
        username[strcspn(username, "\n")] = '\0'; // Remove trailing newline
        if (strlen(username) > 0)
        {
            break;
        }
        printf("Username cannot be empty. Please enter a username: ");
    }
    // Get email
    printf("Enter email: ");
    scanf("%s", email); // Simple scanf for email
    while (!is_valid_email(email))
    {
        printf("Invalid email. Please enter a valid email: ");
        scanf("%s", email);
    }
    // Get password
    printf("Enter password: ");
    scanf("%s", password);
    // Optional: Add password validation (e.g., length, special characters)
    while (strlen(password) < 6)
    {
        printf("Password must be at least 6 characters long. Please enter again: ");
        scanf("%s", password);
    }
    // printf("Password: %s\n", password);
    snprintf(message, message_size, "REG<%s><%s><%s>", username, email, password);
    // printf("Message sent: %s\n", message);
    send(sock, message, strlen(message), 0);
    char server_response[512];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("Server response: %s\n", server_response);
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
    }
}
void handle_login()
{
    system("clear");
    char email[50], password[50];
    printf("═══════════════════════════════════\n");
    printf("               LOGIN               \n");
    printf("═══════════════════════════════════\n");
    printf("Enter email: ");
    scanf("%s", email);
    while (!is_valid_email(email))
    {
        printf("Invalid email. Please enter a valid email: ");
        scanf("%s", email);
    }

    printf("Enter password: ");
    scanf("%s", password);
    // printf("Password: %s\n", password);
    snprintf(message, message_size, "LOG<%s><%s>", email, password);
    // printf("Message sent: %s\n", message);
    send(sock, message, strlen(message), 0);
    char server_response[512];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        if (strncmp(server_response, "200", 3) == 0)
        {
            printf("✅ %s\n", server_response);
            strncpy(token, server_response + 5, sizeof(token));
            token[sizeof(token) - 1] = '\0';
            printf("Token: %s\n", token);
        }
        else
        {
            printf("❌ %s\n", server_response);
            char c;
            getchar();
            scanf("%c", &c);
        }
    }
    else
    {
        printf("Login failed.\n");
    }
}
void handle_logout()
{
    snprintf(message, message_size, "OUT<%s>", token);
    send(sock, message, strlen(message), 0);
    memset(token, 0, sizeof(token));
    token[0] = '\0';
}
int is_valid_email(const char *email)
{
    // Check for a null pointer or empty string
    if (email == NULL || *email == '\0')
    {
        return 0;
    }
    // Find the '@' character
    const char *at = strchr(email, '@');
    if (at == NULL || at == email)
    { // '@' must exist and not be the first character
        return 0;
    }
    // There should be at least one '.' after the '@' in the domain part
    const char *dot = strrchr(at, '.');
    if (dot == NULL || dot <= at + 1)
    { // '.' must exist and be after '@'
        return 0;
    }
    // Ensure there's at least two characters after the last '.'
    if (strlen(dot) < 3)
    {
        return 0;
    }
    // Check for invalid characters
    for (const char *p = email; p < at; p++)
    {
        if (!isalnum(*p) && *p != '.' && *p != '_' && *p != '-')
        {
            return 0;
        }
    }
    for (const char *p = at + 1; p < dot; p++)
    {
        if (!isalnum(*p) && *p != '.' && *p != '-')
        {
            return 0;
        }
    }
    return 1; // Passed all checks, email is valid
}
void fetch_projects_list()
{
    // system("clear");
    snprintf(message, sizeof(message), "PRJ<%s>", token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[512];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        char buffer[512]; // Ensure this size is sufficient
        strncpy(buffer, server_response + 4, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0'; // Null-terminate the buffer

        char *entry = strtok(buffer, "<>");
        printf("╔════════════════════════════════════════════════════╗\n");
        printf("║                     PROJECTS LIST                  ║\n");
        printf("╠══════╦═════════════════════════════════════════════╣\n");
        printf("║ ID   ║ Project Name                                ║\n");
        printf("╠══════╬═════════════════════════════════════════════╣\n");

        int project_count = 0;

        while (entry)
        {

            // Parse the project ID and name
            int project_id;
            char project_name[128];

            if (sscanf(entry, "%d %[^\n]", &project_id, project_name) == 2)
            {
                // Successfully parsed project entry
                printf("║ %-4d ║ %-43s ║\n", project_id, project_name);
                printf("╠══════╬═════════════════════════════════════════════╣\n");
                project_count++;
            }
            // else
            // {
            //     // Handle invalid tokens
            //     printf("⚠️  Skipping invalid project entry: '%s'\n", entry);
            // }

            // Get the next token
            entry = strtok(NULL, "<>");
        }

        if (project_count == 0)
        {
            printf("║      ║ No projects available.                      ║\n");
            printf("╚══════╩═════════════════════════════════════════════╝\n");
        }
        else
        {
            printf("╚══════╩═════════════════════════════════════════════╝\n");
        }
    }
}
void handle_view_project(int projectID)
{

    snprintf(message, message_size, "PRD<%d><%s>", projectID, token);
    int len = strlen(message);
    message[len] = '\0';
    // printf("Message sent: %s\n", message);
    send(sock, message, strlen(message), 0);
    char server_response[2048];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        if (strncmp(server_response, "200", 3) == 0)
        {
            char createdBy[128];
            char projectName[128];
            char description[256];
            // Parse the server response (assumes fixed format: 200 <projectID><name><ownerID><description>)
            sscanf(server_response, "200 <%127[^>]><%127[^>]><%255[^>]>", projectName, createdBy, description);

            // Create a formatted string buffer
            char response[2048];
            snprintf(response, sizeof(response),
                     "═══════════════════════════════════════\n"
                     "          Project Details\n"
                     "═══════════════════════════════════════\n"
                     "Project ID     : %d\n"
                     "Name           : %s\n"
                     "Created By     : %s\n"
                     "Description    : %s\n"
                     "═══════════════════════════════════════\n",
                     projectID, projectName, createdBy, description);

            // Print the formatted response (or send it as needed)
            int choice;
            do
            {
                system("clear");
                printf("%s", response);
                printf("Enter:\n");
                printf("1. Go back to project list\n");
                printf("2. View tasks\n");
                printf("3. Invite member\n");
                printf("4. Chat\n");
                printf("5. View project members\n");
                if (scanf("%d", &choice) != 1)
                {
                    clear_input_buffer(); // Clear invalid input
                    continue;
                    ; // Restart the loop
                }
                if (choice == 1)
                {
                    return;
                }
                else if (choice == 2)
                {
                    handle_view_tasks(projectID);
                }
                else if (choice == 3)
                {
                    char email[50];
                    printf("Enter email: ");
                    scanf("%s", email);
                    while (!is_valid_email(email))
                    {
                        printf("Invalid email. Please enter a valid email: ");
                        scanf("%s", email);
                    }
                    handle_invite_member(projectID, email);
                }
                else if (choice == 4)
                {
                    handle_chat(projectID);
                }
                else if (choice ==5)
                {
                    handle_view_project_members(projectID);
                }
            } while (choice != 1);
        }
        else
        {
            printf("%s\n", server_response);
            char c;
            printf("Press any key to go back\n");
            getchar();
            scanf("%c", &c);
        }
    }

    else
    {
        printf("!!!!!!Server down!!!!!!!\n");
    }
}
void handle_create_project(char *project_name, char *project_description)
{
    system("clear");
    snprintf(message, message_size, "PRO<%s><%s><%s>", project_name, project_description, token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[128];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
    }
}
void handle_invite_member(int projectID, char *email)
{
    // system("clear");

    // printf("Debug: projectID=%d, email=%s, token=%s\n", projectID, email, token);
    snprintf(message, message_size, "INV<%d><%s><%s>", projectID, email, token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[128];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s\n", server_response);
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
    }
}
void handle_view_tasks(int projectID)
{
    do
    {
        system("clear");
        snprintf(message, message_size, "VTL<%d><%s>", projectID, token);
        send(sock, message, strlen(message), 0);
        // printf("Message sent: %s\n", message);
        char server_response[2048];
        memset(server_response, 0, sizeof(server_response));
        int read_size = recv(sock, server_response, sizeof(server_response), 0);
        if (read_size > 0)
        {
            // printf("%s", server_response);
            if (strncmp(server_response, "200", 3) == 0)
            {
                const char *data = strchr(server_response, ' ') + 1;
                printf("╔══════════╦═══════════════════════╦══════════════╦══════════════════════════╗\n");
                printf("║ Task ID  ║ Task Name             ║ Status       ║ Assigned To              ║\n");
                printf("╠══════════╬═══════════════════════╬══════════════╬══════════════════════════╣\n");

                // Parse the response and print the tasks
                const char *ptr = data;
                int task_count = 0;

                while (*ptr != '\0')
                {
                    int taskID;
                    char taskName[50] = {0};
                    char status[20] = {0};
                    char assignedTo[50] = {0};

                    // Parse each task block
                    if (sscanf(ptr, "<%d><%[^>]><%[^>]><%[^>]>", &taskID, taskName, status, assignedTo) == 4)
                    {
                        printf("║ %-8d ║ %-21s ║ %-12s ║ %-24s ║\n", taskID, taskName, status, assignedTo);
                        printf("╠══════════╬═══════════════════════╬══════════════╬══════════════════════════╣\n");
                        task_count++;
                    }

                    // Move to the next task block
                    for (int i = 0; i < 4 && ptr; i++)
                    {
                        ptr = strchr(ptr + 1, '<');
                    }

                    if (!ptr)
                        break; // Exit if no more tasks
                }

                // If no tasks were found
                if (task_count == 0)
                {
                    printf("║          ║ No tasks available.   ║              ║                          ║\n");
                    printf("╚══════════╩═══════════════════════╩══════════════╩══════════════════════════╝\n");
                }
                else
                {
                    // Print footer
                    printf("╚══════════╩═══════════════════════╩══════════════╩══════════════════════════╝\n");
                }

                int choice;

                printf("Enter:\n");
                printf("1. Go back\n");
                printf("2. Create new task\n");
                printf("3. View a task details\n");
                if (scanf("%d", &choice) != 1)
                {
                    clear_input_buffer(); // Clear invalid input
                    continue;             // Restart the loop
                }
                if (choice == 1)
                {
                    break;
                }
                if (choice == 2)
                {
                    char task_name[50], member_email[50];
                    printf("Create Task\n");
                    printf("Enter task name: ");
                    while (fgets(task_name, sizeof(task_name), stdin) != NULL)
                    {
                        // Remove the newline character at the end, if present
                        task_name[strcspn(task_name, "\n")] = '\0';
                        if (strlen(task_name) > 0)
                        {
                            break;
                        }
                    }
                    printf("Assign this task to (email): ");
                    scanf("%s", member_email);
                    while (!is_valid_email(member_email))
                    {
                        printf("Invalid email. Please enter a valid email: ");
                        scanf("%s", member_email);
                    }
                    handle_create_task(projectID, task_name, member_email);
                }
                else if (choice == 3)
                {
                    int taskID;
                    printf("Enter task ID: ");
                    if (scanf("%d", &taskID) != 1)
                    {
                        clear_input_buffer(); // Clear invalid input
                        continue;             // Restart the loop
                    }
                    handle_view_one_task(projectID, taskID);
                }
            }
            else
            {
                printf("Error: %s\n", server_response);
                char c;
                printf("OK?\n");
                getchar();
                scanf("%c", &c);
                break;
            }
        }
    } while (1);
}
void handle_create_task(int projectID, char *task_name, char *member_email)
{

    snprintf(message, message_size, "TSK<%d><%s><%s><%s>", projectID, task_name, member_email, token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[128];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
    }
}
void handle_add_file(int projectID, int taskID, char *file_path)
{
    char file_size[256];

    if (access(file_path, F_OK) != 0)
    {
        perror("❌File does not exist");
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
        return;
    }

    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        printf("❌Failed to open file.\n");
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
        return;
    }
    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size_value = ftell(file);
    fseek(file, 0, SEEK_SET); // Reset the file pointer to the beginning

    // Convert the file size to a string
    snprintf(file_size, sizeof(file_size), "%ld", file_size_value);
    // Extract file name
    char *file_name = strrchr(file_path, '/');
    if (file_name == NULL)
        file_name = strrchr(file_path, '\\');
    file_name = (file_name == NULL) ? file_path : file_name + 1;

    // Validate file type
    if (!is_allowed_file_type(file_path))
    {
        printf("❌Unsupported file type. Only text, documents, and images are allowed.\n");
        fclose(file);
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
        return;
    }

    // Send header
    char header[512];
    snprintf(header, sizeof(header), "ATH<%d><%d><%s><%s><%s>", projectID, taskID, file_name, token, file_size);
    send(sock, header, strlen(header), 0);
    printf("Message sent: %s\n", header);

    // Wait for server acknowledgment
    char server_response[128];
    memset(server_response, 0, sizeof(server_response));
    if (recv(sock, server_response, sizeof(server_response), 0) <= 0 || strncmp(server_response, "200", 3) != 0)
    {
        printf("Server response: %s\n", server_response);
        printf("Server rejected attachment request.\n");
        fclose(file);
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
        return;
    }

    // Send file data
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
        if (send(sock, buffer, bytes_read, 0) <= 0)
        {
            perror("😢Failed to send file data");
            fclose(file);
            char c;
            printf("OK?\n");
            getchar();
            scanf("%c", &c);
            return;
        }
    }
    fclose(file);
    // printf("File sent successfully: %s\n", file_name);
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);
    }
}
int is_allowed_file_type(const char *file_path)
{
    const char *allowed_extensions[] = {".txt", ".doc", ".docx", ".pdf", ".jpg", ".jpeg", ".png", ".pptx"};
    size_t num_allowed = sizeof(allowed_extensions) / sizeof(allowed_extensions[0]);

    const char *file_ext = strrchr(file_path, '.');
    if (file_ext == NULL) // No extension
        return 0;

    for (size_t i = 0; i < num_allowed; i++)
    {
        if (strcasecmp(file_ext, allowed_extensions[i]) == 0) // Case-insensitive comparison
            return 1;
    }
    return 0;
}
void handle_view_one_task(int projectID, int taskID)
{
    do
    {
        system("clear");
        snprintf(message, message_size, "VOT<%d><%s>", taskID, token);
        send(sock, message, strlen(message), 0);
        // printf("Message sent: %s\n", message);
        char server_response[2048];
        memset(server_response, 0, sizeof(server_response));
        int read_size = recv(sock, server_response, sizeof(server_response), 0);
        if (read_size > 0)
        {
            printf("server_response: %s\n", server_response);
            if (strncmp(server_response, "200", 3) == 0)
            {

                char taskName[256], comment[256], status[64], createTime[128];
                char memberEmail[128], files[512];
                int parsed = sscanf(server_response, "200 <%d><%255[^>]><%255[^>]><%63[^>]><%127[^>]><%127[^>]><%511[^>]>",
                                    &taskID, taskName, comment, status, createTime, memberEmail, files);
                if (parsed < 6)
                {
                    fprintf(stderr, "Error: Unable to parse server response.\n");
                    return;
                }
                printf("\n═════════════════════════════════════════════════════════════════════════\n");
                printf("                  %s\n", taskName);
                printf("═════════════════════════════════════════════════════════════════════════\n\n");
                printf("Task ID         : %d\n", taskID);
                printf("Comment         : %s\n", comment); // Fallback to "No comment" if empty
                printf("Status          : %s\n", status);
                printf("Created Time    : %s\n", createTime);
                printf("Member Incharge : %s\n", memberEmail);
                printf("Files Attached  : %s\n", files); // Fallback if empty
                printf("\n════════════════════════════════════════════════════════════════════════\n");

                int choice;
                printf("1. Go back to task list\n");
                printf("2. Add Attachment\n");
                printf("3. Add Comment\n");
                printf("4. Update Status\n");
                printf("Enter your choice: ");
                if (scanf("%d", &choice) != 1)
                {
                    clear_input_buffer(); // Clear invalid input
                    continue;             // Restart the loop
                }
                if (choice == 4)
                {
                    char new_status[32];
                    printf("Choose status:\n");
                    printf("1. Not Started\n");
                    printf("2. In Progress\n");
                    printf("3. Completed\n");
                    printf("Enter your choice: ");
                    if (scanf("%d", &choice) != 1)
                    {
                        clear_input_buffer(); // Clear invalid input
                        continue;             // Restart the loop
                    }
                    if (choice == 1)
                    {
                        strcpy(new_status, "Not started");
                    }
                    else if (choice == 2)
                    {
                        strcpy(new_status, "In Progress");
                    }
                    else if (choice == 3)
                    {
                        strcpy(new_status, "Completed");
                    }
                    handle_update_status(taskID, new_status);
                }
                else if (choice == 3)
                {
                    char new_comment[256];
                    printf("Enter comment: ");
                    while (fgets(new_comment, sizeof(new_comment), stdin) != NULL)
                    {
                        // Remove the newline character at the end, if present
                        new_comment[strcspn(new_comment, "\n")] = '\0';
                        if (strlen(new_comment) > 0)
                        {
                            // printf("new_comment: %s\n", new_comment);
                            break;
                        }
                    }
                    if ("\n" == new_comment)
                    {
                        strcpy(new_comment, "No comment");
                    };
                    handle_add_comment(taskID, new_comment);
                }
                else if (choice == 2)
                {
                    char file_path[256];
                    printf("Enter file path: ");
                    scanf("%s", file_path);
                    handle_add_file(projectID, taskID, file_path);
                }
                else if (choice == 1)
                {
                    break;
                }
            }
            else
            {
                char c;
                printf("OK?\n");
                getchar();
                scanf("%c", &c);
                break;
            }
        }
    } while (1);
}
void handle_add_comment(int taskID, char *comment)
{
    snprintf(message, message_size, "CMT<%d><%s><%s>", taskID, comment, token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[128];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        // printf("%s", server_response);
        char c;
        printf("OK?\n");
        scanf("%c", &c);
    }
}
void handle_update_status(int taskID, char *status)
{
    snprintf(message, message_size, "STT<%d><%s><%s>", taskID, status, token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[128];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
        char c;
        printf("OK?\n");
        scanf("%c", &c);
    }
}
void handle_chat(int projectID)
{
    system("clear");
    snprintf(message, message_size, "CHT<%d><%s>", projectID, token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[128];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
    }
}
void clear_input_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ; // Discard remaining input
}
void handle_view_project_members(int projectID)
{
    snprintf(message, message_size, "MEM<%d><%s>", projectID, token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    char server_response[2048];
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
        char c;
        printf("OK?\n");
        getchar();
        scanf("%c", &c);

    }
}