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
int is_valid_email(const char *email);
void fetch_projects_list(char *token);
void menu();
void home_page(char *token, char *server_response);

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
    {   // check if already login (if token is not null)
        if (token[0] != '\0')
        {   
            home_page(token, server_response);
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
void home_page(char *token, char *server_response)
{   int choice;
    printf("HOME PAGE\n*********************\n");
    printf("Your projects:\n");
    printf("%s\n", server_response + 56);
    printf("Enter project ID to view details: ");
    fetch_projects(token);
    scanf("%d", &choice);
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
    // printf("Message: %s\n", message);
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
    printf("Message: %s\n", message);
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
    memset(server_response, 0, sizeof(server_response));
    int read_size = recv(sock, server_response, sizeof(server_response), 0);
    if (read_size > 0)
    {
        printf("Server response: %s\n", server_response);
    }
}