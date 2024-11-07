#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>
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
                send(client_sock, "401 <Unauthorized: Invalid email or password>\n", strlen("401 <Unauthorized: Invalid email or password>\n"), 0);
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
                print_sessions(session_manager);
            }
        }
        else if (strncmp(client_message, "PRJ", 3) == 0)
        {
            char token[32];
            strncpy(token, client_message + 4, sizeof(token) - 1);
            token[sizeof(token) - 1] = '\0';
            printf("Token: %s\n", token);
            UserSession *userSession = find_session(session_manager, token);
            if (userSession == NULL)
            {
                send(client_sock, "401 <Unauthorized: Invalid token>\n", strlen("401 <Unauthorized: Invalid token>\n"), 0);
                continue;
            }
            int userID = userSession->userID;
            char response[512];
            char *projects= get_projects_list(conn, userID);
            if (projects == NULL)
            {
                send(client_sock, "500 <Internal server error>\n", strlen("500 <Internal server error>\n"), 0);
                continue;
            }
            snprintf(response, sizeof(response), "200 <%s>\n", projects);
            send(client_sock, response, strlen(response), 0);
            printf("Response: %s\n", response);
            free(projects);
        }
        memset(client_message, 0, sizeof(client_message));
    }

    PQfinish(conn);
    close(client_sock);
    return NULL;
}
