#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include "database.h"
#include "session.h"
#include "client_handler.h"
void *client_handler(void *args)
{
    ClientHandlerArgs *client_args = (ClientHandlerArgs *)args;
    int client_sock = client_args->socket;
    SessionManager *session_manager = client_args->session_manager;

    free(client_args); // Don't forget to free the passed struct
    char client_message[2048];
    PGconn *conn = connect_db();

    while (recv(client_sock, client_message, sizeof(client_message), 0) > 0)
    {

        if (strncmp(client_message, "REG", 3) == 0)
        {
            int status = register_user(conn, client_message);
            if (status == 1)
            {
                send(client_sock, "200 <Registration successful>\n", strlen("200 <Registration successful>\n"), 0);
            }
            else
            {
                send(client_sock, "500 <Registration failed: Email or username already exists>\n", strlen("500 <Registration failed: Email or username already exists>\n"), 0);
            }
        }
        else if (strncmp(client_message, "LOG", 3) == 0)
        {
            int userID = login_user(conn, client_message);
            if (userID == -1)
            {
                send(client_sock, "401 <Unauthorized: Invalid email or password>", strlen("401 <Unauthorized: Invalid email or password>"), 0);
            }
            else
            {
                char token[32], response[64];
                strncpy(token, generate_token(), sizeof(token) - 1);
                token[sizeof(token) - 1] = '\0';
                snprintf(response, sizeof(response), "200 <Login successful><%s>\n", token);
                // printf("Response: %s\n", response);
                send(client_sock, response, strlen(response), 0);
                add_session(session_manager, userID, token);
                // print_sessions(session_manager);
            }
        }
        else if (strncmp(client_message, "OUT", 3) == 0)
        {
            char token[32];
            strncpy(token, client_message + 4, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
            remove_session(session_manager, token);
            // print_sessions(session_manager);
        }
        else if (strncmp(client_message, "PRJ", 3) == 0)
        {
            char token[32];
            strncpy(token, client_message + 4, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            int userID = userSession->userID;
            char response[512];
            char *projects = get_projects_list(conn, userID);
            if (projects == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                continue;
            }
            snprintf(response, sizeof(response), "200 <%s>\n", projects);
            send(client_sock, response, strlen(response), 0);
            // printf("Response: %s\n", response);
            free(projects);
            projects = NULL;
        }
        else if (strncmp(client_message, "PRD", 3) == 0)
        {
            char token[32];
            int projectID;
            sscanf(client_message, "PRD<%d><%s>", &projectID, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            int userID = userSession->userID;
            // printf("%d %s\n", projectID, token);
            char response[2048];
            char *project = get_project(conn, userID, projectID);
            if (project == NULL)
            {
                send(client_sock, "403 <Not authorized or project not found>\n", strlen("403 <Not authorized or project not found.>\n"), 0);
                continue;
            }
            else
            {
                // printf("%s\n", project);
                snprintf(response, sizeof(response), "200 %s\n", project);
                send(client_sock, response, strlen(response), 0);
                // printf("Response: %s\n", response);
                free(project);
                project = NULL;
            }
        }
        else if (strncmp(client_message, "PRO", 3) == 0)
        {
            char token[32], projectName[50], projectDescription[512];
            sscanf(client_message, "PRO<%[^>]><%[^>]><%[^>]>", projectName, projectDescription, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            int userID = userSession->userID;
            char response[2048];
            int projectID = create_project(conn, userID, projectName, projectDescription);
            if (projectID == -1)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                continue;
            }
            snprintf(response, sizeof(response), "200 <%d>\n", projectID);
            send(client_sock, response, strlen(response), 0);
            // printf("Response: %s\n", response);
        }
        else if (strncmp(client_message, "INV", 3) == 0)
        {
            char token[32], email[50];
            int projectID;
            sscanf(client_message, "INV<%d><%[^>]><%[^>]>", &projectID, email, token);
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            int status = insert_project_member(conn, projectID, email);
            if (status == -1)
            {
                send(client_sock, "404 <User not found>\n", strlen("404 <User not found>\n"), 0);
                continue;
            }

            send(client_sock, "200 <Invitation successful>\n", strlen("200 <Invitation successful>\n"), 0);
        }
        else if (strncmp(client_message, "VTL", 3) == 0)
        {
            char token[32];
            int projectID;
            sscanf(client_message, "VTL<%d><%[^>]>", &projectID, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            char *tasks = get_tasks(conn, projectID);
            if (tasks == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                continue;
            }
            else
            {
                char response[2048];
                snprintf(response, sizeof(response), "200 %s\n", tasks);
                // printf("%s\n", tasks);
                // printf("Response: %s\n", response);
                send(client_sock, response, strlen(response), 0);
            }
            free(tasks);
            tasks = NULL;
        }
        else if (strncmp(client_message, "TSK", 3) == 0)
        {
            char token[32], taskName[50], member_email[50];
            int projectID;
            sscanf(client_message, "TSK<%d><%[^>]><%[^>]><%[^>]>", &projectID, taskName, member_email, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            int status = insert_task(conn, projectID, taskName, member_email);
            if (status == -1)
            {
                send(client_sock, "403 <USER NOT IN PROJECT>\n", strlen("403 <USER NOT IN PROJECT>\n"), 0);
                continue;
            }
            send(client_sock, "200 <Task created>\n", strlen("200 <Task created>\n"), 0);
        }
        else if (strncmp(client_message, "ATH", 3) == 0)
        {
            char token[32], file_name[50];
            int projectID, taskID;
            long file_size;
            sscanf(client_message, "ATH<%d><%d><%[^>]><%[^>]><%ld>", &projectID, &taskID, file_name, token, &file_size);
            token[sizeof(token) - 1] = '\0';
            printf("Token: %s\n", token);
            // Validate session
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            int userID = userSession->userID;
            // Construct folder path
            char folder_path[256];
            snprintf(folder_path, sizeof(folder_path), "/mnt/e/ThucHanhLapTrinhMang20241/project/refactor/project_manager/%d", projectID);
            // printf("Folder path: %s\n", folder_path);
            if (mkdir(folder_path, 0755) == -1)
            {
                perror("mkdir failed");
            }
            snprintf(folder_path, sizeof(folder_path), "/mnt/e/ThucHanhLapTrinhMang20241/project/refactor/project_manager/%d/%d", projectID, taskID);
            if (mkdir(folder_path, 0755) == -1)
            {
                perror("mkdir failed");
            }
            // Construct full file path on the server
            char server_file_path[512];
            snprintf(server_file_path, sizeof(server_file_path), "%s/%s", folder_path, file_name);
            // Open file for writing
            FILE *file = fopen(server_file_path, "wb");
            if (file == NULL)
            {
                perror("Failed to open file");
                send(client_sock, "500 <Internal Server Error: Cannot receive file>\n", strlen("500 <Internal Server Error: Cannot receive file>\n"), 0);
                continue;
            }
            int status = attach_file_to_task(conn, userID, taskID, file_name);
            if (status == -1)
            {
                send(client_sock, "403 <You dont have permission to attach file to this task>\n", strlen("403 <You dont have permission to attach file to this task>\n"), 0);
                printf("You dont have permission to attach file to this task\n");
                continue;
            }
            // Notify client to start sending file data
            send(client_sock, "200 <Ready to receive file>\n", strlen("200 <Ready to receive file>\n"), 0);

            // Receive file data from client
            char buffer[1024];
            ssize_t bytes_received;
            long total_bytes_received = 0;
            while (total_bytes_received < file_size && (bytes_received = recv(client_sock, buffer, sizeof(buffer), 0)) > 0)
            {
                fwrite(buffer, 1, bytes_received, file);
                total_bytes_received += bytes_received;
            }

            fclose(file);
            // Confirm success
            send(client_sock, "200 <Attachment added successfully>\n", strlen("200 <Attachment added successfully>\n"), 0);
        }
        else if (strncmp(client_message, "VOT", 3) == 0)
        {
            int taskID;
            char token[32];
            sscanf(client_message, "VOT<%d><%s>", &taskID, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            char *task_info = view_one_task(conn, taskID);
            if (task_info == NULL)
            {
                send(client_sock, "404 <Task not found in project>\n", strlen("404 <Task not found in project>\n"), 0);
                continue;
            }
            send(client_sock, task_info, strlen(task_info), 0);
            free(task_info);
            task_info = NULL;
        }
        else if (strncmp(client_message, "CMT", 3) == 0)
        {
            char token[32], comment[256];
            int taskID, userID;
            sscanf(client_message, "CMT<%d><%[^>]><%s>", &taskID, comment, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            userID = userSession->userID;
            int status = add_comment(conn, userID, taskID, comment);
            if (status == -1)
            {
                send(client_sock, "403 <You don't have permission to add comment to this task>\n", strlen("403 <You don't have permission to add comment to this task>\n"), 0);
                continue;
            }
            send(client_sock, "200 <Comment added successfully>\n", strlen("200 <Comment added successfully>\n"), 0);
        }
        memset(client_message, 0, sizeof(client_message));
    }

    PQfinish(conn);
    close(client_sock);
    return NULL;
}
