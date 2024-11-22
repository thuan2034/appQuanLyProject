#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libpq-fe.h>
#include <unistd.h>
#include <sys/socket.h>
#include "database.h"
#include "session.h"

PGconn *connect_db()
{
    PGconn *conn = PQconnectdb("dbname=project_management user=postgres password=admin host=localhost port=5432");

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
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

int login_user(PGconn *conn, const char *client_message)
{
    char query[512], email[50], password[50];
    sscanf(client_message, "LOG<%49[^>]><%49[^>]>", email, password);
    snprintf(query, sizeof(query), "SELECT * FROM \"USER\" WHERE email='%s' AND password='%s'", email, password);
    PGresult *res = PQexec(conn, query);
    int userID = -1;
    if (PQntuples(res) == 1)
    {
        userID = atoi(PQgetvalue(res, 0, 0));
    }
    PQclear(res);
    return userID;
}
char *get_projects_list(PGconn *conn, int userID)
{
    char query[512];
    // Dynamically allocate memory for the response string
    char *projects_list = (char *)malloc(1024 * sizeof(char));
    if (projects_list == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
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
        fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
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
        strncat(projects_list, "[", 1024 - strlen(projects_list) - 1);
        strncat(projects_list, projectID, 1024 - strlen(projects_list) - 1);
        strncat(projects_list, " ", 1024 - strlen(projects_list) - 1);
        strncat(projects_list, name, 1024 - strlen(projects_list) - 1);
        strncat(projects_list, "]", 1024 - strlen(projects_list) - 1);
    }
    PQclear(res);
    // printf("Projects: %s\n", projects_list);
    return projects_list;
}
char *get_project(PGconn *conn, int userID, int projectID)
{
    const char *query =
        "WITH UserAuthorization AS ("
        "    SELECT 1 "
        "    FROM  \"PROJECT_MEMBER\" "
        "    WHERE \"userID\" = $1 AND \"projectID\" = $2"
        ") "
        "SELECT "
        "    P.\"projectID\", "
        "    P.name, "
        "    P.\"ownerID\", "
        "    P.description "
        "FROM "
        "    \"PROJECT\" P "
        "WHERE "
        "    P.\"projectID\" = $2 "
        "    AND EXISTS (SELECT 1 FROM UserAuthorization);";

    // Convert parameters to string
    char userIDStr[12], projectIDStr[12];
    snprintf(userIDStr, sizeof(userIDStr), "%d", userID);
    snprintf(projectIDStr, sizeof(projectIDStr), "%d", projectID);

    // Parameter array
    const char *paramValues[2] = {userIDStr, projectIDStr};

    // Execute query with parameters
    PGresult *res = PQexecParams(conn, query, 2, NULL, paramValues, NULL, NULL, 0);

    // Check query result
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "Query failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return NULL;
    }
    // Check if project details are returned
    int rows = PQntuples(res);
    if (rows == 0)
    {
        printf("Not authorized or project not found.\n");
        return NULL;
    }
    else
    {
        const char *projectIDStr = PQgetvalue(res, 0, 0); // projectID
        const char *name = PQgetvalue(res, 0, 1);         // name
        const char *ownerID = PQgetvalue(res, 0, 2);      // ownerID
        const char *description = PQgetvalue(res, 0, 3);  // description
        size_t response_size = 32 + strlen(projectIDStr) + strlen(name) + strlen(ownerID) + strlen(description);
        char *project = malloc(response_size);
        if (project == NULL)
        {
            fprintf(stderr, "Memory allocation failed!\n");
            PQclear(res);
            return project;
        }
        project[0] = '\0';
        // Build the response string
        snprintf(project, response_size, "<%s><%s><%s><%s>", projectIDStr, name, ownerID, description);
        printf("%s\n", project);
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
        printf("Project created with ID: %d\n", projectID);
    }
    else
    {
        fprintf(stderr, "Error inserting project: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return -1; // Return -1 to indicate failure
    }
    PQclear(res);
    // Insert into PROJECT_MEMBER
    snprintf(query, sizeof(query), "INSERT INTO \"PROJECT_MEMBER\" (\"projectID\", \"userID\") VALUES (%d, %d)", projectID, userID);
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr, "Error inserting into PROJECT_MEMBER: %s\n", PQerrorMessage(conn));
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
    printf("Email: %s\n", email);
    // Fetch the userID based on email
    snprintf(query, sizeof(query), "SELECT \"userID\" FROM \"USER\" WHERE email = '%s'", email);
    PGresult *res = PQexec(conn, query);

    if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        fprintf(stderr, "Error fetching userID: %s\n", PQerrorMessage(conn));
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
            printf("User is already a member of the project.\n");
            PQclear(res);
            return -1; // Indicate that the user is already a member
        }

        fprintf(stderr, "Error inserting into PROJECT_MEMBER: %s\n", errMessage);
        PQclear(res);
        return -1; // Return -1 for other errors
    }

    PQclear(res); // Free the result of the second query
    return 0;     // Return 0 to indicate success
}
char *get_tasks(PGconn *conn, int projectID)
{
    char query[512];
    char *tasks = malloc(1024 * sizeof(char));
    if (tasks == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }
    tasks[0] = '\0';
    snprintf(query, sizeof(query), "SELECT \"taskID\", name FROM \"TASK\" WHERE \"projectID\" = %d", projectID);
    PGresult *res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr, "Error fetching tasks: %s\n", PQerrorMessage(conn));
        PQclear(res);
        free(tasks);
        return NULL;
    }
    int num_rows = PQntuples(res);
    if (num_rows == 0)
    {
        PQclear(res);
        return tasks;
    }
    for (int i = 0; i < num_rows; i++)
    {
        char *taskID = PQgetvalue(res, i, 0);
        char *taskName = PQgetvalue(res, i, 1);
        strncat(tasks, "<", 1024 - strlen(tasks) - 1);
        strncat(tasks, taskID, 1024 - strlen(tasks) - 1); // Append taskID, 1024 - strlen(tasks) - 1);
        strncat(tasks, " ", 1024 - strlen(tasks) - 1);
        strncat(tasks, taskName, 1024 - strlen(tasks) - 1);
        strncat(tasks, ">", 1024 - strlen(tasks) - 1);
    }
    PQclear(res);
    return tasks;
}