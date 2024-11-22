#ifndef DATABASE_H
#define DATABASE_H

#include <libpq-fe.h>

PGconn *connect_db();
int register_user(PGconn *conn, const char *client_message);
int login_user(PGconn *conn, const char *client_message);
char *get_projects_list(PGconn *conn, int userID);
char *get_project(PGconn *conn, int userID, int projectID);
int create_project(PGconn *conn, int userID, const char *projectName, const char *projectDescription);
int insert_project_member(PGconn *conn, int projectID, const char *email);
char *get_tasks(PGconn *conn, int projectID);
#endif // DATABASE_H
