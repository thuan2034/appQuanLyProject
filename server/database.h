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
int insert_task(PGconn *conn, int projectID, const char *taskName,const char *member_email);
int attach_file_to_task(PGconn *conn,int userID, int taskID, const char *file_name);
char *view_one_task(PGconn *conn, int taskID);
int add_comment(PGconn *conn, int userID,int taskID, const char *comment);
#endif // DATABASE_H
