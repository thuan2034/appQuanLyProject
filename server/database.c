#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "database.h"
#include "session.h"

PGconn *connect_db()
{
    PGconn *conn = PQconnectdb("dbname=project_management user=postgres password=admin host=localhost port=5432");

    if (PQstatus(conn) != CONNECTION_OK)
    {
        // fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(1);
    }
    return conn;
}

int register_user(PGconn *conn, const char *client_message)
{
    char query[512], username[50], email[50], password[50];
    sscanf(client_message, "REG<%49[^>]><%49[^>]><%49[^>]>", username, email, password);

    snprintf(query, sizeof(query), "INSERT INTO \"USER\" (name, email, password, time_created) VALUES ('%s', '%s', '%s', CURRENT_TIMESTAMP)", username, email, password);
    PGresult *res = PQexec(conn, query);
    int status = 0;
    if (PQresultStatus(res) == PGRES_COMMAND_OK)
    {
        status = 1;
    }
    PQclear(res);
    return status;
}

char *login_user(PGconn *conn, const char *client_message)
{
    char query[512], email[50], password[50];
    sscanf(client_message, "LOG<%49[^>]><%49[^>]>", email, password);
    snprintf(query, sizeof(query), "SELECT * FROM \"USER\" WHERE email='%s' AND password='%s'", email, password);
    PGresult *res = PQexec(conn, query);
    int userID = -1;
    // username = NULL;
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        // fprintf(stderr, "Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    if (PQntuples(res) == 1)
    {
        userID = atoi(PQgetvalue(res, 0, 0));
        char *username = PQgetvalue(res, 0, 1);
        PQclear(res);
        char *username_userId = malloc(1024); // Dynamically allocate memory
        if (username_userId == NULL)
        {
            PQclear(res);
            return NULL; // Handle memory allocation failure
        }
        snprintf(username_userId, 1024, "<%s><%d>", username, userID);
        return username_userId;
    }
    PQclear(res);
    return NULL;
}
char *get_projects_list(PGconn *conn, int userID)
{
    char query[512];
    // Dynamically allocate memory for the response string
    char *projects_list = (char *)malloc(1024 * sizeof(char));
    if (projects_list == NULL)
    {
        // fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    projects_list[0] = '\0';
    // Build query
    snprintf(query, sizeof(query), "SELECT P.\"projectID\", P.name "
                                   "FROM \"PROJECT\" P "
                                   "JOIN \"PROJECT_MEMBER\" PM ON P.\"projectID\" = PM.\"projectID\" "
                                   "WHERE PM.\"userID\" = %d",
             userID);

    PGresult *res = PQexec(conn, query);
    // Check if query was successful
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        // fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        free(projects_list);
        return NULL;
    }

    // Iterate over each row in the result
    int num_rows = PQntuples(res);
    for (int i = 0; i < num_rows; i++)
    {
        // Get projectID and name values for the current row
        char *projectID = PQgetvalue(res, i, 0);
        char *name = PQgetvalue(res, i, 1);
        // Append `[projectID,name]` to the projects_list
        strncat(projects_list, "<", 1024 - strlen(projects_list) - 1);
        strncat(projects_list, projectID, 1024 - strlen(projects_list) - 1);
        strncat(projects_list, " ", 1024 - strlen(projects_list) - 1);
        strncat(projects_list, name, 1024 - strlen(projects_list) - 1);
        strncat(projects_list, ">", 1024 - strlen(projects_list) - 1);
    }
    PQclear(res);
    // printf("Projects: %s\n", projects_list);
    return projects_list;
}
char *get_project(PGconn *conn, int userID, int projectID)
{
    char query[512];
    snprintf(query, sizeof(query), "WITH UserAuthorization AS ("
                                   "    SELECT 1 "
                                   "    FROM  \"PROJECT_MEMBER\" "
                                   "    WHERE \"userID\" = %d AND \"projectID\" = %d"
                                   ") "
                                   "SELECT "
                                   "    P.name, "
                                   "    U.\"name\", "
                                   "    P.description "
                                   "FROM "
                                   "    \"PROJECT\" P "
                                   "    JOIN \"USER\" U ON P.\"ownerID\" = U.\"userID\" "
                                   "WHERE "
                                   "    P.\"projectID\" = %d "
                                   "    AND EXISTS (SELECT 1 FROM UserAuthorization);",
             userID, projectID, projectID);

    // Execute query with parameters
    PGresult *res = PQexec(conn, query);
    // Check query result
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        // fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    // Check if project details are returned
    int rows = PQntuples(res);
    if (rows == 0)
    {
        // printf("Not authorized or project not found.\n");
        return NULL;
    }
    else
    {
        const char *name = PQgetvalue(res, 0, 0);        // name
        const char *createdBy = PQgetvalue(res, 0, 1);   // ownerID
        const char *description = PQgetvalue(res, 0, 2); // description
        size_t response_size = 32 + strlen(name) + strlen(createdBy) + strlen(description);
        char *project = malloc(response_size);
        if (project == NULL)
        {
            // fprintf(stderr, "Memory allocation failed!\n");
            PQclear(res);
            return project;
        }
        project[0] = '\0';
        // Build the response string
        snprintf(project, response_size, "<%s><%s><%s>", name, createdBy, description);
        // printf("%s\n", project);
        PQclear(res);
        return project;
    }
}
int create_project(PGconn *conn, int userID, const char *projectName, const char *projectDescription)
{
    char query[512];
    snprintf(query, sizeof(query), "INSERT INTO \"PROJECT\" (name, \"ownerID\", description) VALUES ('%s', %d, '%s') RETURNING \"projectID\"", projectName, userID, projectDescription);

    PGresult *res = PQexec(conn, query);
    int projectID = -1;
    // Check the correct result status
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        projectID = atoi(PQgetvalue(res, 0, 0));
        // printf("Project created with ID: %d\n", projectID);
    }
    else
    {
        // fprintf(stderr, "Error inserting project: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1; // Return -1 to indicate failure
    }
    PQclear(res);
    // Insert into PROJECT_MEMBER
    snprintf(query, sizeof(query), "INSERT INTO \"PROJECT_MEMBER\" (\"projectID\", \"userID\") VALUES (%d, %d)", projectID, userID);
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        // fprintf(stderr, "Error inserting into PROJECT_MEMBER: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return projectID; // Return -1 to indicate failure
    }
    PQclear(res);
    return projectID; // Return the new project ID
}
int insert_project_member(PGconn *conn, int projectID, const char *email)
{
    char query[512];
    int userID = -1;
    // printf("Email: %s\n", email);
    // Fetch the userID based on email
    snprintf(query, sizeof(query), "SELECT \"userID\" FROM \"USER\" WHERE email = '%s'", email);
    PGresult *res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        // fprintf(stderr, "Error fetching userID: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1; // Return -1 to indicate failure
    }

    // Get the userID from the result
    userID = atoi(PQgetvalue(res, 0, 0));
    PQclear(res); // Free the result of the first query

    // Insert into PROJECT_MEMBER
    snprintf(query, sizeof(query), "INSERT INTO \"PROJECT_MEMBER\" (\"projectID\", \"userID\") VALUES (%d, %d)", projectID, userID);
    res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        // Check if it's a duplicate key violation
        const char *errMessage = PQresultErrorMessage(res);
        if (strstr(errMessage, "duplicate key") != NULL)
        {
            // printf("User is already a member of the project.\n");
            PQclear(res);
            return -2; // Indicate that the user is already a member
        }

        // fprintf(stderr, "Error inserting into PROJECT_MEMBER: %s\n", errMessage);
        PQclear(res);
        return -3; // Return -1 for other errors
    }

    PQclear(res);  // Free the result of the second query
    return userID; // Return 0 to indicate success
}
char *get_tasks(PGconn *conn, int projectID)
{
    char query[512];
    char *tasks = malloc(1024 * sizeof(char));
    if (tasks == NULL)
    {
        // fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    tasks[0] = '\0';
    snprintf(query, sizeof(query), "SELECT T.\"taskID\", T.name AS task_name, T.status, U.name AS username "
                                   "FROM \"TASK\" T "
                                   "LEFT JOIN \"USER\" U ON T.\"userID\" = U.\"userID\" "
                                   "WHERE T.\"projectID\" = %d",
             projectID);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        // fprintf(stderr, "Error fetching tasks: %s\n", PQerrorMessage(conn));
        PQclear(res);
        free(tasks);
        return NULL;
    }
    int num_rows = PQntuples(res);
    if (num_rows == 0)
    {
        PQclear(res);
        strcat(tasks, "<>");
        return tasks;
    }
    for (int i = 0; i < num_rows; i++)
    {
        char *taskID = PQgetvalue(res, i, 0);
        char *taskName = PQgetvalue(res, i, 1);
        char *taskStatus = PQgetvalue(res, i, 2);
        char *memberName = PQgetvalue(res, i, 3);
        strncat(tasks, "<", 1024 - strlen(tasks) - 1);
        strncat(tasks, taskID, 1024 - strlen(tasks) - 1); // Append taskID, 1024 - strlen(tasks) - 1);
        strncat(tasks, ">", 1024 - strlen(tasks) - 1);
        strncat(tasks, "<", 1024 - strlen(tasks) - 1);
        strncat(tasks, taskName, 1024 - strlen(tasks) - 1); // Append taskName, 1024 - strlen(tasks) - 1);
        strncat(tasks, ">", 1024 - strlen(tasks) - 1);
        strncat(tasks, "<", 1024 - strlen(tasks) - 1);
        strncat(tasks, taskStatus, 1024 - strlen(tasks) - 1); // Append taskStatus, 1024 - strlen(tasks) - 1);
        strncat(tasks, ">", 1024 - strlen(tasks) - 1);
        strncat(tasks, "<", 1024 - strlen(tasks) - 1);
        strncat(tasks, memberName, 1024 - strlen(tasks) - 1); // Append memberName, 1024 - strlen(tasks) - 1);
        strncat(tasks, ">", 1024 - strlen(tasks) - 1);
    }
    PQclear(res);
    return tasks;
}
int insert_task(PGconn *conn, int projectID, const char *taskName, const char *member_email)
{
    char query[512];
    snprintf(query, sizeof(query), "WITH UserInProject AS ("
                                   "    SELECT 1 "
                                   "    FROM \"PROJECT_MEMBER\" "
                                   "    WHERE \"projectID\" = %d AND \"userID\" = (SELECT \"userID\" FROM \"USER\" WHERE email = '%s')"
                                   ") "
                                   "INSERT INTO \"TASK\" (\"projectID\", \"name\", \"status\", \"userID\") "
                                   "SELECT %d, '%s', 'not_started', (SELECT \"userID\" FROM \"USER\" WHERE email = '%s') "
                                   "WHERE EXISTS (SELECT 1 FROM UserInProject) "
                                   "RETURNING \"taskID\";",
             projectID, member_email, projectID, taskName, member_email);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        // fprintf(stderr, "User not in project or error inserting task: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    int taskID = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return taskID;
}
int attach_file_to_task(PGconn *conn, int userID, int taskID, const char *file_name)
{
    char query[512];
    snprintf(query, sizeof(query), "INSERT INTO \"ATTACHMENT\" (\"taskID\", file_name) "
                                   "SELECT %d, '%s' "
                                   "WHERE EXISTS ("
                                   "  SELECT 1 FROM \"TASK\" WHERE \"taskID\" = %d AND \"userID\" = %d"
                                   ")",
             taskID, file_name, taskID, userID);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        // fprintf(stderr, "Error attaching file to task: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }
    if (atoi(PQcmdTuples(res)) == 0)
    {
        // fprintf(stderr, "No attachment added: userID %d does not own taskID %d\n", userID, taskID);
        PQclear(res);
        return -1; // No rows inserted
    }
    PQclear(res);
    return 0;
}
char *view_one_task(PGconn *conn, int taskID)
{
    char query[512];
    snprintf(query, sizeof(query),
             "SELECT T.\"taskID\", T.name, T.status, T.time_created, "
             "U.email, STRING_AGG(A.file_name, '|') AS file_names, "
             "C.\"commentID\", C.username, C.content, C.time "
             "FROM \"TASK\" T "
             "JOIN \"USER\" U ON T.\"userID\" = U.\"userID\" "
             "LEFT JOIN \"ATTACHMENT\" A ON T.\"taskID\" = A.\"taskID\" "
             "LEFT JOIN \"COMMENT\" C ON T.\"taskID\" = C.\"taskID\" "
             "WHERE T.\"taskID\" = %d "
             "GROUP BY T.\"taskID\", T.name, T.status, T.time_created, U.email, "
             "C.\"commentID\", C.username, C.content, C.time;",
             taskID);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        PQclear(res);
        return NULL;
    }

    int rows = PQntuples(res);
    if (rows == 0)
    {
        PQclear(res);
        return NULL; // No data found
    }

    char *task = malloc(2048); // Allocate space for the formatted result
    if (task == NULL)
    {
        PQclear(res);
        return NULL; // Memory allocation failed
    }
    memset(task, 0, 2048); // Initialize task string

    // Extract data from the query result
    char *name = PQgetvalue(res, 0, 1);
    char *status = PQgetvalue(res, 0, 2);
    char *time_created = PQgetvalue(res, 0, 3);
    char *email = PQgetvalue(res, 0, 4);
    char *file_names = PQgetvalue(res, 0, 5);
    if (file_names == NULL)
        file_names = ""; // If no files, use an empty string

    // Prepare initial part of task string
    snprintf(task, 2048, "<%s><%s><%s><%s><%s><%s><", PQgetvalue(res, 0, 0), name, status, time_created, email, file_names);

    // Process comments (if any)
    char comments[2048] = "";
    for (int i = 0; i < rows; i++)
    {
        char *commentID = PQgetvalue(res, i, 6);
        char *username = PQgetvalue(res, i, 7);
        char *content = PQgetvalue(res, i, 8);
        char *comment_time = PQgetvalue(res, i, 9);

        // Format the comment and append to comments string
        snprintf(comments + strlen(comments), sizeof(comments) - strlen(comments),
                 "[%s][%s][%s][%s]", commentID, username, content, comment_time);
    }

    // If there are no comments, ensure that it returns "[]"
    if (strlen(comments) == 0)
    {
        snprintf(comments, sizeof(comments), "[]");
    }

    // Finalize task string with comments
    snprintf(task + strlen(task), 2048 - strlen(task), "%s>", comments);

    PQclear(res);
    return task;
}
int add_comment(PGconn *conn, int userID, int taskID, const char *comment)
{
    char escaped_comment[1024];                                                // Buffer to hold the escaped comment
    PQescapeStringConn(conn, escaped_comment, comment, strlen(comment), NULL); // Escape special characters in the comment

    char query[512 + 1024]; // Adjust buffer size to accommodate the escaped comment
    snprintf(query, sizeof(query),
             "INSERT INTO \"COMMENT\" (\"taskID\", username, content) "
             "VALUES (%d, (SELECT \"name\" FROM \"USER\" WHERE \"userID\" = %d), '%s') "
             "RETURNING \"commentID\";",
             taskID, userID, escaped_comment);

    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        // fprintf(stderr, "Error adding comment: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    // printf("Comment added successfully to task ID %d.\n", taskID);
    return 0;
}
int update_task_status(PGconn *conn, int userID, int taskID, const char *status)
{
    char query[512];
    snprintf(query, sizeof(query), "UPDATE \"TASK\" "
                                   "SET status = '%s' "
                                   "WHERE \"taskID\" = %d AND \"userID\" = %d "
                                   "RETURNING \"taskID\";",
             status, taskID, userID);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        // fprintf(stderr, "Error updating task status: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1;
    }

    PQclear(res);
    // printf("Task status updated successfully for task ID %d.\n", taskID);
    return 0;
}
char *get_project_members(PGconn *conn, int projectID)
{
    char query[512];
    snprintf(query, sizeof(query), "SELECT U.email, U.\"name\" "
                                   "FROM \"PROJECT_MEMBER\" PM "
                                   "JOIN \"USER\" U ON PM.\"userID\" = U.\"userID\" "
                                   "WHERE PM.\"projectID\" = %d;",
             projectID);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        // fprintf(stderr, "Error fetching project members: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    int rows = PQntuples(res);
    if (rows == 0)
    {
        // printf("No project members found for project ID: %d\n", projectID);
        PQclear(res);
        return NULL;
    }
    else
    {
        char *members = malloc(2048);
        if (members == NULL)
        {
            // fprintf(stderr, "Error allocating memory for project members\n");
            PQclear(res);
            return NULL;
        }
        members[0] = '\0';
        for (int i = 0; i < rows; i++)
        {
            char *email = PQgetvalue(res, i, 0);
            char *name = PQgetvalue(res, i, 1);
            //<email><username>...
            snprintf(members + strlen(members), 2048 - strlen(members), "<%s><%s>", email, name);
        }
        // printf("%s\n", members);
        PQclear(res);
        return members;
    }
}

// Function to check if a user has access to a project
int user_has_access(PGconn *conn, int userID, int projectID)
{
    const char *paramValues[2];
    char userID_str[12];
    char projectID_str[12];

    snprintf(userID_str, sizeof(userID_str), "%d", userID);
    snprintf(projectID_str, sizeof(projectID_str), "%d", projectID);

    paramValues[0] = projectID_str;
    paramValues[1] = userID_str;

    PGresult *res = PQexecParams(conn,
                                 "SELECT 1 FROM \"PROJECT_MEMBER\" WHERE \"projectID\" = $1 AND \"userID\" = $2 LIMIT 1;",
                                 2, // number of parameters
                                 NULL,
                                 paramValues,
                                 NULL,
                                 NULL,
                                 0); // text results

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "user_has_access: Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return 0; // Deny access on error
    }

    int has_access = (PQntuples(res) > 0) ? 1 : 0;
    PQclear(res);
    return has_access;
}

// Function to fetch chat history
char *get_chat_history(PGconn *conn, int projectID, int limit, int offset)
{
    const char *paramValues[3];
    char projectID_str[12];
    char limit_str[12];
    char offset_str[12];

    snprintf(projectID_str, sizeof(projectID_str), "%d", projectID);
    snprintf(limit_str, sizeof(limit_str), "%d", limit);
    snprintf(offset_str, sizeof(offset_str), "%d", offset);

    paramValues[0] = projectID_str;
    paramValues[1] = limit_str;
    paramValues[2] = offset_str;

    // Order by timestamp DESC to get the most recent messages first
    PGresult *res = PQexecParams(conn,
                                 "SELECT m.\"messageID\", u.name, to_char(m.timestamp, 'YYYY-MM-DD HH24:MI:SS'), m.content "
                                 "FROM \"MESSAGE\" m "
                                 "JOIN \"USER\" u ON m.\"userID\" = u.\"userID\" "
                                 "WHERE m.\"projectID\" = $1 "
                                 "ORDER BY m.timestamp DESC "
                                 "LIMIT $2 OFFSET $3;",
                                 3,    // number of parameters
                                 NULL, // let the backend deduce param types
                                 paramValues,
                                 NULL, // don't need param lengths since text
                                 NULL, // default to all text format
                                 0);   // text results

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "get_chat_history: Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }

    int rows = PQntuples(res);
    if (rows == 0)
    {
        // No messages found
        PQclear(res);
        printf("No messages found for project ID: %d\n", projectID);
        return strdup("200 "); // Return only the status code
    }

    // Calculate the total buffer size needed
    // Start with "200 " (4 characters)
    size_t buffer_size = 4; // "200 " including space
    // Calculate the size needed for all messages
    for (int i = 0; i < rows; i++)
    {
        // Get the message components
        char *msgID = PQgetvalue(res, i, 0);
        char *username = PQgetvalue(res, i, 1);
        char *timestamp = PQgetvalue(res, i, 2);
        char *content = PQgetvalue(res, i, 3);

        // Calculate the length of the formatted message
        // msgID|username|timestamp|content
        int msg_length = strlen(msgID) + 1 + strlen(username) + 1 + strlen(timestamp) + 1 + strlen(content);

        // Each length prefix is "<length>" which can be up to 10 digits
        buffer_size += 2 + 10 + 1; // '<' + length digits + '>'
        buffer_size += msg_length;
    }

    char *buffer = malloc(buffer_size + 1); // +1 for null-terminator
    if (!buffer)
    {
        fprintf(stderr, "get_chat_history: Memory allocation failed\n");
        PQclear(res);
        return NULL;
    }

    strcpy(buffer, "200 "); // Initialize buffer with status code
    char *ptr = buffer + 4; // Move pointer past "200 "

    // To maintain chronological order (oldest first), we need to store messages in reverse
    // Since we fetched them in DESC order, we'll store them in a temporary array and reverse later

    char **messages = malloc(rows * sizeof(char *));
    if (!messages)
    {
        fprintf(stderr, "get_chat_history: Memory allocation for messages failed\n");
        free(buffer);
        PQclear(res);
        return NULL;
    }

    // Store messages in temporary array
    for (int i = 0; i < rows; i++)
    {
        char *msgID = PQgetvalue(res, i, 0);
        char *username = PQgetvalue(res, i, 1);
        char *timestamp = PQgetvalue(res, i, 2);
        char *content = PQgetvalue(res, i, 3);

        // Format the message content
        char message[1200];
        snprintf(message, sizeof(message), "%s|%s|%s|%s", msgID, username, timestamp, content);

        // Allocate memory for the message
        messages[i] = strdup(message);
        if (!messages[i])
        {
            fprintf(stderr, "get_chat_history: Memory allocation for a message failed\n");
            // Free previously allocated messages
            for (int j = 0; j < i; j++)
            {
                free(messages[j]);
            }
            free(messages);
            free(buffer);
            PQclear(res);
            return NULL;
        }
    }

    // Append messages to buffer in reverse order to maintain chronological order
    for (int i = rows - 1; i >= 0; i--)
    {
        char *message = messages[i];
        int msg_length = strlen(message);

        // Format the length prefix
        char length_prefix[16];
        snprintf(length_prefix, sizeof(length_prefix), "<%d>", msg_length);

        // Append length prefix
        strcpy(ptr, length_prefix);
        ptr += strlen(length_prefix);

        // Append message content
        strcpy(ptr, message);
        ptr += strlen(message);

        free(messages[i]); // Free individual message after appending
    }

    free(messages); // Free the temporary array

    // Null-terminate the buffer
    *ptr = '\0';

    PQclear(res);
    return buffer;
}
// Corrected count_messages function
int count_messages(PGconn *conn, int projectID)
{
    const char *paramValues[1];
    char projectID_str[12];

    snprintf(projectID_str, sizeof(projectID_str), "%d", projectID);
    paramValues[0] = projectID_str;

    PGresult *res = PQexecParams(conn,
                                 "SELECT COUNT(*) "
                                 "FROM \"MESSAGE\" m "
                                 "JOIN \"USER\" u ON m.\"userID\" = u.\"userID\" "
                                 "WHERE m.\"projectID\" = $1;",
                                 1, // number of parameters
                                 NULL,
                                 paramValues,
                                 NULL,
                                 NULL,
                                 0); // text results

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "count_messages: Query failed: %s", PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }

    int count = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    return count;
}

int insert_message(PGconn *conn, int projectID, int userID, const char *message)
{
    char query[512];
    snprintf(query, sizeof(query), "INSERT INTO \"MESSAGE\" (\"projectID\", \"userID\", content) VALUES (%d, %d, '%s') RETURNING \"messageID\"", projectID, userID, message);
    PGresult *res = PQexec(conn, query);
    int messageID = -1;
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        messageID = atoi(PQgetvalue(res, 0, 0));
    }
    else
    {
        fprintf(stderr, "insert_message: Query failed: %s", PQerrorMessage(conn));
    }
    PQclear(res);
    return messageID;
}