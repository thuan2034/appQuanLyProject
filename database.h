#ifndef DATABASE_H
#define DATABASE_H

#include <libpq-fe.h>

PGconn *connect_db();
int register_user(PGconn *conn, const char *client_message);
int login_user(PGconn *conn, const char *client_message);
char *get_projects_list(PGconn *conn, int userID);
#endif // DATABASE_H
