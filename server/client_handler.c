#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include "database.h"
#include "session.h"
#include "client_handler.h"
void *client_handler(void *args)
{
    ClientHandlerArgs *client_args = (ClientHandlerArgs *)args;
    int client_sock = client_args->socket;
    SessionManager *session_manager = client_args->session_manager;
    ChatRoomList *chat_rooms = client_args->chat_rooms;
    free(client_args); // Don't forget to free the passed struct
    char client_message[2048];
    char username[50];
    PGconn *conn = connect_db();

    while (recv(client_sock, client_message, sizeof(client_message), 0) > 0)
    {

        if (strncmp(client_message, "REG", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            int status = register_user(conn, client_message);
            if (status == 1)
            {
                send(client_sock, "200 <Registration successful>\n", strlen("200 <Registration successful>\n"), 0);
                printf("Server response: 200 <Registration successful>\n");
            }
            else
            {
                send(client_sock, "500 <Registration failed: Email or username already exists>\n", strlen("500 <Registration failed: Email or username already exists>\n"), 0);
                printf("Server response: 500 <Registration failed: Email or username already exists>\n");
            }
        }
        else if (strncmp(client_message, "LOG", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            // username_userID format: <username><userID>
            char *username_userID = login_user(conn, client_message);

            if (username_userID == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid email or password>\n", strlen("401 <Unauthorized: Invalid email or password>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid email or password>\n");
            }
            else
            {
                char token[32], response[256];
                int userID;
                strncpy(token, generate_token(), sizeof(token) - 1);
                token[sizeof(token) - 1] = '\0';
                // printf("Token: %s\n", token);
                sscanf(username_userID, "<%49[^>]><%d>", username, &userID);
                snprintf(response, sizeof(response), "200 <%s><%s>\n", token, username);
                // printf("Response: %s\n", response);
                send(client_sock, response, strlen(response), 0);
                printf("Server response: %s\n", response);
                add_session(session_manager, userID, token);
                // print_sessions(session_manager);
            }
        }
        else if (strncmp(client_message, "OUT", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32];
            strncpy(token, client_message + 4, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
            remove_session(session_manager, token);
            // print_sessions(session_manager);
            send(client_sock, "200 <Logout successful>\n", strlen("200 <Logout successful>\n"), 0);
            printf("Server response: 200 <Logout successful>\n");
        }
        else if (strncmp(client_message, "PRJ", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32];
            strncpy(token, client_message + 4, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            int userID = userSession->userID;
            char response[512];
            char *projects = get_projects_list(conn, userID);
            if (projects == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                printf("Server response: 500 <Internal server error>\n");
                continue;
            }
            snprintf(response, sizeof(response), "200 %s\n", projects);
            send(client_sock, response, strlen(response), 0);
            printf("Server response: %s\n", response);
            free(projects);
            projects = NULL;
        }
        else if (strncmp(client_message, "PRD", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32];
            int projectID;
            sscanf(client_message, "PRD<%d><%s>", &projectID, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            int userID = userSession->userID;
            // printf("%d %s\n", projectID, token);
            char response[2048];
            char *project = get_project(conn, userID, projectID);
            if (project == NULL)
            {
                send(client_sock, "403 <Not authorized or project not found>\n", strlen("403 <Not authorized or project not found.>\n"), 0);
                printf("Server response: 403 <Not authorized or project not found>\n");
                continue;
            }
            else
            {
                // printf("%s\n", project);
                snprintf(response, sizeof(response), "200 %s\n", project);
                send(client_sock, response, strlen(response), 0);
                printf("Server response: %s\n", response);
                free(project);
                project = NULL;
            }
        }
        else if (strncmp(client_message, "PRO", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32], projectName[50], projectDescription[512];
            sscanf(client_message, "PRO<%[^>]><%[^>]><%[^>]>", projectName, projectDescription, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            int userID = userSession->userID;
            char response[2048];
            int projectID = create_project(conn, userID, projectName, projectDescription);
            if (projectID == -1)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                printf("Server response: 500 <Internal server error>\n");
                continue;
            }
            snprintf(response, sizeof(response), "200 <%d>\n", projectID);
            send(client_sock, response, strlen(response), 0);
            printf("Server response: %s\n", response);
        }
        else if (strncmp(client_message, "INV", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32], email[50];
            int projectID;
            sscanf(client_message, "INV<%d><%[^>]><%[^>]>", &projectID, email, token);
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            int status = insert_project_member(conn, projectID, email);
            if (status == -1)
            {
                send(client_sock, "404 <User not found>\n", strlen("404 <User not found>\n"), 0);
                printf("Server respone: 404 <User not found>\n");
                continue;
            }
            else if (status == -2)
            {
                send(client_sock, "403 <User already in project>\n", strlen("403 <User already in project>\n"), 0);
                printf("Server respone: 403 <User already in project>\n");
                continue;
            }
            else if (status == -3)
            {
                send(client_sock, "500 <Unable to invite>\n", strlen("500 <Unable to invite>\n"), 0);
                printf("Server respone: 500 <Unable to invite>\n");
                continue;
            }
            send(client_sock, "200 <Invitation successful>\n", strlen("200 <Invitation successful>\n"), 0);
            printf("Server response: 200 <Invitation successful>\n");
        }
        else if (strncmp(client_message, "MEM", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32];
            int projectID;
            sscanf(client_message, "MEM<%d><%[^>]>", &projectID, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            char *members = get_project_members(conn, projectID);
            if (members == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                continue;
            }
            else
            {
                char response[2048];
                snprintf(response, sizeof(response), "200 %s\n", members);
                send(client_sock, response, strlen(response), 0);
                printf("Server response: %s\n", response);
                free(members);
            }
        }
        else if (strncmp(client_message, "VTL", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
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
                send(client_sock, response, strlen(response), 0);
                printf("Server response: %s\n", response);
            }
            free(tasks);
            tasks = NULL;
        }
        else if (strncmp(client_message, "TSK", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32], taskName[50], member_email[50], description[1024], time_created[128], time_end[128];
            int projectID;
            sscanf(client_message, "TSK<%d><%[^>]><%[^>]><%[^>]><%[^>]><%[^>]><%[^>]>", &projectID, taskName,description, member_email,time_created, time_end, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            int taskID = insert_task(conn, projectID, taskName, member_email, description, time_created, time_end);
            if (taskID == -1)
            {
                send(client_sock, "403 <USER NOT IN PROJECT>\n", strlen("403 <USER NOT IN PROJECT>\n"), 0);
                printf("Server response: 403 <USER NOT IN PROJECT>\n");
                continue;
            }
            char response[32];
            snprintf(response, sizeof(response), "200 <%d>\n", taskID);
            send(client_sock, response, strlen(response), 0);
            printf("Server response: %s\n", response);
        }
        else if (strncmp(client_message, "ATH", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32], file_name[50];
            int taskID;
            long file_size;
            sscanf(client_message, "ATH<%d><%[^>]><%[^>]><%ld>", &taskID, file_name, token, &file_size);
            token[sizeof(token) - 1] = '\0';
            // printf("Token: %s\n", token);
            // Validate session
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            int userID = userSession->userID;
            // Construct folder path
            char folder_path[256];
            snprintf(folder_path, sizeof(folder_path), "/mnt/e/ThucHanhLapTrinhMang20241/project/refactor/project_manager/%d", taskID);
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
                printf("Server response: 500 <Internal Server Error: Cannot receive file>\n");
                continue;
            }
            int status = attach_file_to_task(conn, userID, taskID, file_name);
            if (status == -1)
            {
                send(client_sock, "403 <You dont have permission to attach file to this task>\n", strlen("403 <You dont have permission to attach file to this task>\n"), 0);

                printf("Server response: 403 <You dont have permission to attach file to this task>\n");
                continue;
            }
            // Notify client to start sending file data
            send(client_sock, "200 <Ready to receive file>\n", strlen("200 <Ready to receive file>\n"), 0);
            printf("Server response: 200 <Ready to receive file>\n");

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
            printf("Server response: 200 <Attachment added successfully>\n");
        }
        else if (strncmp(client_message, "VOT", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
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
                printf("Server response: 404 <Task not found in project>\n");
                continue;
            }
            char response[2048];
            snprintf(response, sizeof(response), "200 %s\n", task_info);
            send(client_sock, response, strlen(response), 0);
            printf("Server response: %s\n", response);
            free(task_info);
            task_info = NULL;
        }
        else if(strncmp(client_message,"VCM",3) == 0){
            printf("Client message: %s\n", client_message);
            int taskID, offset;
            char token[32];
            sscanf(client_message, "VCM<%d><%d><%s>", &taskID,&offset, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            char *comments = get_comments(conn, taskID, offset);
            if (comments == NULL)
            {
                send(client_sock, "404 <Task not found in project>\n", strlen("404 <Task not found in project>\n"), 0);
                printf("Server response: 404 <Task not found in project>\n");
                continue;
            }
            char response[2048];
            snprintf(response, sizeof(response), "200 %s\n", comments);
            send(client_sock, response, strlen(response), 0);
            printf("Server response: %s\n", response);
            free(comments);
            comments = NULL;
        }
        else if (strncmp(client_message, "CMT", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32], comment[256];
            int taskID, userID;
            sscanf(client_message, "CMT<%d><%[^>]><%s>", &taskID, comment, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server respone: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            userID = userSession->userID;
            int status = add_comment(conn, userID, taskID, comment);
            if (status == -1)
            {
                send(client_sock, "403 <You don't have permission to add comment to this task>\n", strlen("403 <You don't have permission to add comment to this task>\n"), 0);
                printf("Server respone: 403 <You don't have permission to add comment to this task>\n");
                continue;
            }
            send(client_sock, "200 <Comment added successfully>\n", strlen("200 <Comment added successfully>\n"), 0);
            printf("Server respone: 200 <Comment added successfully>\n");
        }
        else if (strncmp(client_message, "STT", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32], status[32];
            int taskID, userID;
            sscanf(client_message, "STT<%d><%[^>]><%s>", &taskID, status, token);
            token[sizeof(token) - 1] = '\0';
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server respone: 401 <Unauthorized: Invalid token>\n");
                continue;
            }
            userID = userSession->userID;
            int result = update_task_status(conn, userID, taskID, status);
            if (result == -1)
            {
                send(client_sock, "403 <You don't have permission to update this task status>\n", strlen("403 <You don't have permission to update this task status>\n"), 0);
                printf("Server respone: 403 <You don't have permission to update this task status>\n");
                continue;
            }
            send(client_sock, "200 <Task status updated successfully>\n", strlen("200 <Task status updated successfully>\n"), 0);
            printf("Server respone: 200 <Task status updated successfully>\n");
        }
        else if (strncmp(client_message, "DOW", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            char token[32], fileName[64];
            int taskID;
            sscanf(client_message, "DOW<%d><%[^>]><%s>", &taskID, fileName, token);
            token[sizeof(token) - 1] = '\0';

            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Construct file path
            char filePath[512];
            snprintf(filePath, sizeof(filePath), "/mnt/e/ThucHanhLapTrinhMang20241/project/refactor/project_manager/%d/%s", taskID, fileName);

            FILE *file = fopen(filePath, "rb");
            if (file == NULL)
            {
                send(client_sock, "404 <File not found>\n", strlen("404 <File not found>\n"), 0);
                printf("Server response: 404 <File not found>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Read and send file data as binary
            char buffer[1024];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0)
            {
                send(client_sock, buffer, bytes_read, 0);
            }
            fclose(file);

            // Send confirmation message
            send(client_sock, "200 <File downloaded successfully>\n", strlen("200 <File downloaded successfully>\n"), 0);
            printf("Server response: 200 <File downloaded successfully>\n");
        }
        else if (strncmp(client_message, "JCH", 3) == 0){
            printf("Client message: %s\n", client_message);
            int projectID;
            char token[32];
            // Parse the message: JCH<projectID><token>
            if (sscanf(client_message, "JCH<%d><%31[^>]>", &projectID, token) != 2) {
                send(client_sock, "400 <Invalid request format>\n", strlen("400 <Invalid request format>\n"), 0);
                printf("Server response: 400 <Invalid request format>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }
            token[31] = '\0'; // Ensure null-termination

            // Authenticate the token
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Retrieve or create the chat room
            ChatRoom *chat_room = get_or_create_chat_room(chat_rooms, projectID);
            if (chat_room == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                printf("Server response: 500 <Internal server error>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Add the client to the chat room
            add_client_to_room(chat_room, client_sock);

            // Optionally, send a confirmation message
            send(client_sock, "200 <Joined chat room successfully>\n", strlen("200 <Joined chat room successfully>\n"), 0);
            printf("Server response: 200 <Joined chat room successfully>\n");
        }
        else if (strncmp(client_message, "VCH", 3) == 0)
        {
            printf("Client message: %s\n", client_message);

            // Variables to hold parsed data
            char token[32];
            int projectID;
            int limit;
            int offset;

            // Parse the message: VCH<projectID><token><offset>
            // Since limit is hardfixed to 10, we remove it from the parsing
            if (sscanf(client_message, "VCH<%d><%31[^>]><%d><%d>", &projectID, token, &limit, &offset) != 4)
            {
                send(client_sock, "400 <Invalid request format>\n", strlen("400 <Invalid request format>\n"), 0);
                printf("Server response: 400 <Invalid request format>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }
            // Null-terminate the token to prevent buffer overflow
            token[31] = '\0';

            // Authenticate the token
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            int userID = userSession->userID;

            // Check if the user has access to the project
            if (!user_has_access(conn, userID, projectID))
            {
                send(client_sock, "403 <Forbidden: Access to project denied>\n", strlen("403 <Forbidden: Access to project denied>\n"), 0);
                printf("Server response: 403 <Forbidden: Access to project denied>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Fetch the chat history from the database with fixed limit=10
            char *chat_history = get_chat_history(conn, projectID, limit, offset);
            if (chat_history == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                printf("Server response: 500 <Internal server error>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }
            // printf("Chat history: %s\n", chat_history); // Removed to prevent unnecessary logging
            // Send the chat history to the client
            char response[4096];
            snprintf(response, sizeof(response), "%s\n", chat_history);
            send(client_sock, response, strlen(response), 0);
            printf("Server response: %s\n", response);
            free(chat_history);
        }
        else if (strncmp(client_message, "MSG", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            int projectID;
            char token[32];
            char messageContent[1024];

            // Parse the message: MSG<projectID><token><content>
            if (sscanf(client_message, "MSG<%d><%31[^>]><%1023[^>]>", &projectID, token, messageContent) != 3)
            {
                send(client_sock, "400 <Invalid request format>\n", strlen("400 <Invalid request format>\n"), 0);
                printf("Server response: 400 <Invalid request format>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            token[31] = '\0'; // Ensure null-termination

            // Authenticate the token
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Check if the user has access to the project
            int userID = userSession->userID;
            if (!user_has_access(conn, userID, projectID))
            {
                send(client_sock, "403 <Forbidden: Access to project denied>\n", strlen("403 <Forbidden: Access to project denied>\n"), 0);
                printf("Server response: 403 <Forbidden: Access to project denied>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Insert the message into the database
            if (insert_message(conn, projectID, userID, messageContent) == -1)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                printf("Server response: 500 <Internal server error>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Retrieve the user's name
            if (username == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                printf("Server response: 500 <Internal server error>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Construct the broadcast message in the format: MSG<projectID<userName><content>
            char broadcast_msg[2048];
            snprintf(broadcast_msg, sizeof(broadcast_msg), "MSG<%d><%s><%s>\n", projectID, username, messageContent);

            // Retrieve the chat room
            ChatRoom *chat_room = get_or_create_chat_room(chat_rooms, projectID);
            if (chat_room == NULL)
            {
                // Optionally handle the case where the chat room doesn't exist
                send(client_sock, "404 <Chat room not found>\n", strlen("404 <Chat room not found>\n"), 0);
                printf("Server response: 404 <Chat room not found>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Broadcast the message to other clients in the chat room
            broadcast_message(chat_room, client_sock, broadcast_msg);
            printf("Broadcasted message to chat room %d: %s\n", projectID, broadcast_msg);

            // Send a confirmation to the sender
            send(client_sock, "200 <Message sent successfully>\n", strlen("200 <Message sent successfully>\n"), 0);
            printf("Server response: 200 <Message sent successfully>\n");

        }
         else if (strncmp(client_message, "LCH", 3) == 0)
        {
            printf("Client message: %s\n", client_message);
            int projectID;
            char token[32];
            // Parse the message: LCH<projectID><token>
            if (sscanf(client_message, "LCH<%d><%31[^>]>", &projectID, token) != 2)
            {
                send(client_sock, "400 <Invalid request format>\n", strlen("400 <Invalid request format>\n"), 0);
                printf("Server response: 400 <Invalid request format>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }
            token[31] = '\0'; // Ensure null-termination

            // Authenticate the token
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                printf("Server response: 401 <Unauthorized: Invalid token>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Retrieve the chat room without creating a new one
            // Assuming you have a get_chat_room function; if not, implement it in chat_room.c
            ChatRoom *chat_room = get_or_create_chat_room(chat_rooms, projectID);
            if (chat_room == NULL)
            {
                send(client_sock, "404 <Chat room not found>\n", strlen("404 <Chat room not found>\n"), 0);
                printf("Server response: 404 <Chat room not found>\n");
                memset(client_message, 0, sizeof(client_message));
                continue;
            }

            // Remove the client from the chat room
            remove_client_from_room(chat_room, client_sock);

            // Optionally, send a confirmation message
            send(client_sock, "200 <Left chat room successfully>\n", strlen("200 <Left chat room successfully>\n"), 0);
            printf("Server response: 200 <Left chat room successfully>\n");
        }
        else
        {
            printf("Client message: %s\n", client_message);
            send(client_sock, "400 <Invalid request>\n", strlen("400 <Invalid request>\n"), 0);
            printf("Server respone: 400 <Invalid request>\n");
        }
        memset(client_message, 0, sizeof(client_message));
    }

    PQfinish(conn);
    close(client_sock);
    return NULL;
}
