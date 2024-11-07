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
    // Start building the projects_list string
    // Iterate over each row in the result
    int num_rows = PQntuples(res);
    for (int i = 0; i < num_rows; i++)
    {
        // Get projectID and name values for the current row
        char *projectID = PQgetvalue(res, i, 0);
        char *name = PQgetvalue(res, i, 1);
        // Append `[projectID,name]` to the projects_list
        strncat(projects_list, "[", 1024 - strlen(projects_list) - 1);
        strncat(projects_list, projectID,1024 - strlen(projects_list) - 1);
        strncat(projects_list, " ", 1024 - strlen(projects_list) - 1);
        strncat(projects_list, name, 1024 - strlen(projects_list) - 1);
        strncat(projects_list, "]", 1024 - strlen(projects_list) - 1);
    }
    PQclear(res);
    printf("Projects: %s\n", projects_list);
    return projects_list;
}
