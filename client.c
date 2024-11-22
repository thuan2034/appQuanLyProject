#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#define PORT 5000
int sock = 0;
char token[32];
char message[2048], server_response[2048];
void handle_registration(char *message, size_t message_size);
void handle_login(char *message, size_t message_size);
void handle_logout(char *message, size_t message_size);
int is_valid_email(const char *email);
void fetch_projects(char *token);
void handle_choose_project(char *message, size_t message_size);
void handle_create_project(char *message, size_t message_size);
void handle_invite_member(char *message, size_t message_size, int projectID);
void handle_view_tasks(char *message, size_t message_size, int projectID);
void handle_create_task(char *message, size_t message_size, int projectID);
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
    printf("MENU\n*********************\nEnter 1 for registration\nEnter 2 for login\n");
    scanf("%d", &choice);
    if (choice == 1)
    {
        handle_registration(message, sizeof(message));
    }
    else if (choice == 2)
    {
        handle_login(message, sizeof(message));
    }
}
void home_page()
{
    int choice;
    printf("********************************HOME PAGE********************************\n");
    fetch_projects(token);
    printf("Enter: \n");
    printf("1. Go to a project\n");
    printf("2. Create new project\n");
    printf("3. Logout\n");
    printf("*************************************************************************\n");
    printf("Your choice: ");
    scanf("%d", &choice);
    switch (choice)
    {
    case 1:
        handle_choose_project(message, sizeof(message));
        break;
    case 2:
        handle_create_project(message, sizeof(message));
        break;
    case 3:
        handle_logout(message, sizeof(message));
        break;
    default:
        break;
    }
}
void handle_registration(char *message, size_t message_size)
{
    char username[50], email[50], password[50];
    printf("Registration\n");
    printf("Enter username: ");
    while (fgets(username, sizeof(username), stdin) != NULL)
    {
        // Remove the newline character at the end, if present
        username[strcspn(username, "\n")] = '\0';
        if (strlen(username) > 0)
        {
            // printf("Username: %s\n", username);
            break;
        }
    }
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
    snprintf(message, message_size, "REG<%s><%s><%s>", username, email, password);
    printf("Message sent: %s\n", message);
    send(sock, message, strlen(message), 0);
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("Server response: %s\n", server_response);
    }
}
void handle_login(char *message, size_t message_size)
{
    char email[50], password[50];
    printf("Login\n");
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
    printf("Message sent: %s\n", message);
    send(sock, message, strlen(message), 0);
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        if (strncmp(server_response, "200", 3) == 0)
        {
            printf("server_response: %s\n", server_response);
            strncpy(token, server_response + 23, sizeof(token));
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
        }
        else
        {
            printf("Server response: %s\n", server_response);
        }
    }
    else
    {
        printf("Login failed.\n");
    }
}
void handle_logout(char *message, size_t message_size)
{
    snprintf(message, message_size, "OUT<%s>", token);
    send(sock, message, strlen(message), 0);
    memset(token, 0, sizeof(token));
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
void fetch_projects(char *token)
{
    snprintf(message, sizeof(message), "PRJ<%s>", token);
    send(sock, message, strlen(message), 0);
    // printf("Message sent: %s\n", message);
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        char *start, *end, *entry;
        char buffer[256];
        // Find the start and end of the project list
        start = strchr(server_response, '<');
        end = strchr(server_response, '>');
        // Copy the content inside the '< >' into a buffer
        strncpy(buffer, start + 1, end - start - 1);
        buffer[end - start - 1] = '\0'; // Null-terminate the buffer
        // Tokenize the content by '][' to extract individual project entries
        entry = strtok(buffer, "[]");
        printf("======== PROJECTS ==========\n");
        printf("____________________________\n");
        printf("| ID |        Name        |\n");
        while (entry)
        {
            // Parse the project ID and name
            int project_id;
            char project_name[128];

            if (sscanf(entry, "%d %[^\n]", &project_id, project_name) == 2)
            {
                printf("|----|--------------------|\n");
                printf("|%-4d|%-20s|\n", project_id, project_name);
            }
            entry = strtok(NULL, "[]");
        }
        printf("|____|____________________|\n");
    }
}
void handle_choose_project(char *message, size_t message_size)
{
    int id = 0;
    printf("Enter project ID:");
    scanf("%d", &id);
    snprintf(message, message_size, "PRD<%d><%s>", id, token);
    int len = strlen(message);
    message[len] = '\0';
    printf("Message sent: %s\n", message);
    send(sock, message, strlen(message), 0);
    
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        if (strncmp(server_response, "200", 3) == 0)
        {
            int projectID, ownerID;
            char projectName[128];
            char description[256];
            // Parse the server response (assumes fixed format: 200 <projectID><name><ownerID><description>)
            sscanf(server_response, "200 <%d><%127[^>]><%d><%255[^>]>", &projectID, projectName, &ownerID, description);

            // Create a formatted string buffer
            char response[2048];
            snprintf(response, sizeof(response),
                     "=======================================\n"
                     "          Project Details\n"
                     "=======================================\n"
                     "Project ID     : %d\n"
                     "Name           : %s\n"
                     "Owner ID       : %d\n"
                     "Description    : %s\n"
                     "=======================================\n",
                     projectID, projectName, ownerID, description);

            // Print the formatted response (or send it as needed)
            int choice;
            do
            {
                printf("%s", response);
                printf("Enter:\n");
                printf("1. Go back\n");
                printf("2. View tasks\n");
                printf("3. Invite\n");
                scanf("%d", &choice);
                if (choice == 1)
                {
                    return;
                }
                else if (choice == 2)
                {
                    handle_view_tasks(message, message_size, projectID);
                }
                else if (choice == 3)
                {
                    handle_invite_member(message, message_size, projectID);
                }
            } while (choice != 1);
        }
    }

    else
    {
        printf("!!!!!!Server down!!!!!!!\n");
    }
}
void handle_create_project(char *message, size_t message_size)
{
    char project_name[50], project_description[512];
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
    snprintf(message, message_size, "PRO<%s><%s><%s>", project_name, project_description, token);
    send(sock, message, strlen(message), 0);
    printf("Message sent: %s\n", message);
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
    }
}
void handle_invite_member(char *message, size_t message_size, int projectID)
{
    char email[50];
    printf("Enter email: ");
    scanf("%s", email);
    while (!is_valid_email(email))
    {
        printf("Invalid email. Please enter a valid email: ");
        scanf("%s", email);
    }
    printf("Debug: projectID=%d, email=%s, token=%s\n", projectID, email, token);
    snprintf(message, message_size, "INV<%d><%s><%s>", projectID, email, token);
    send(sock, message, strlen(message), 0);
    printf("Message sent: %s\n", message);
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("%s", server_response);
    }
}
void handle_view_tasks(char *message, size_t message_size, int projectID)
{
    snprintf(message, message_size, "VTL<%d><%s>", projectID, token);
    send(sock, message, strlen(message), 0);
    printf("Message sent: %s\n", message);
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        int taskID;
        char taskName[50];
        char taskDescription[512];
        int taskStatus;
        char taskDueDate[10];
    }
}
void handle_create_task(char *message, size_t message_size, int projectID)
{
    char task_name[50], task_description[512];
    printf("Create Task\n");
    printf("Enter task name: ");
    while (fgets(task_name, sizeof(task_name), stdin) != NULL)
    {
        // Remove the newline character at the end, if present
        task_name[strcspn(task_name, "\n")] = '\0';
        if (strlen(task_name) > 0)
        {    
}}}