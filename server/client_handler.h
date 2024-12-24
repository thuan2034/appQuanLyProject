#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "session.h"
#include "chat_room.h"
typedef struct
{
    int socket;
    SessionManager *session_manager;
    ChatRoomList *chat_rooms;
} ClientHandlerArgs;
void *client_handler(void *args);

#endif // CLIENT_HANDLER_H
