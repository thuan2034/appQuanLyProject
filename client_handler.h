#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "session.h"

typedef struct
{
    int socket;
    SessionManager *session_manager;
} ClientHandlerArgs;
void *client_handler(void *args);

#endif // CLIENT_HANDLER_H
