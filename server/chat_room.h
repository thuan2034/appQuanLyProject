// chat_room.h
#ifndef CHAT_ROOM_H
#define CHAT_ROOM_H

#include <pthread.h>

// Forward declaration
typedef struct ClientNode ClientNode;

// Chat Room Structure
typedef struct ChatRoom {
    int projectID;
    ClientNode *clients;          // Linked list of clients in the room
    pthread_mutex_t room_mutex;   // Mutex to protect the room's client list
    struct ChatRoom *next;        // Pointer to the next chat room in the list
} ChatRoom;

// Client Node Structure for Linked List
struct ClientNode {
    int socket;
    struct ClientNode *next;
};

// Chat Room List Structure
typedef struct {
    ChatRoom *head;
    pthread_mutex_t list_mutex;   // Mutex to protect the chat room list
} ChatRoomList;

// Function Prototypes
void init_chat_room_list(ChatRoomList *list);
ChatRoom* get_or_create_chat_room(ChatRoomList *list, int projectID);
void add_client_to_room(ChatRoom *room, int client_socket);
void remove_client_from_room(ChatRoom *room, int client_socket);
void broadcast_message(ChatRoom *room, int sender_socket, const char *message);
void cleanup_chat_rooms(ChatRoomList *list);

#endif // CHAT_ROOM_H
