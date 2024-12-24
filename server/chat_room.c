// chat_room.c
#include "chat_room.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
// Initialize the chat room list
void init_chat_room_list(ChatRoomList *list) {
    list->head = NULL;
    pthread_mutex_init(&list->list_mutex, NULL);
}

// Get or create a chat room by projectID
ChatRoom* get_or_create_chat_room(ChatRoomList *list, int projectID) {
    pthread_mutex_lock(&list->list_mutex);
    ChatRoom *current = list->head;
    while (current != NULL) {
        if (current->projectID == projectID) {
            pthread_mutex_unlock(&list->list_mutex);
            return current;
        }
        current = current->next;
    }

    // Chat room doesn't exist; create a new one
    ChatRoom *new_room = malloc(sizeof(ChatRoom));
    if (!new_room) {
        perror("Failed to allocate memory for new chat room");
        pthread_mutex_unlock(&list->list_mutex);
        return NULL;
    }
    new_room->projectID = projectID;
    new_room->clients = NULL;
    pthread_mutex_init(&new_room->room_mutex, NULL);
    new_room->next = list->head;
    list->head = new_room;
    pthread_mutex_unlock(&list->list_mutex);
    printf("Created new chat room with Project ID: %d\n", projectID);
    return new_room;
}

// Add client to chat room
void add_client_to_room(ChatRoom *room, int client_socket) {
    pthread_mutex_lock(&room->room_mutex);
    ClientNode *new_client = malloc(sizeof(ClientNode));
    if (!new_client) {
        perror("Failed to allocate memory for client node");
        pthread_mutex_unlock(&room->room_mutex);
        return;
    }
    new_client->socket = client_socket;
    new_client->next = room->clients;
    room->clients = new_client;
    pthread_mutex_unlock(&room->room_mutex);
    printf("Client %d joined Project %d\n", client_socket, room->projectID);
}

// Remove client from chat room
void remove_client_from_room(ChatRoom *room, int client_socket) {
    pthread_mutex_lock(&room->room_mutex);
    ClientNode *current = room->clients;
    ClientNode *prev = NULL;
    while (current != NULL) {
        if (current->socket == client_socket) {
            if (prev == NULL) {
                room->clients = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            pthread_mutex_unlock(&room->room_mutex);
            printf("Client %d left Project %d\n", client_socket, room->projectID);
            return;
        }
        prev = current;
        current = current->next;
    }
    pthread_mutex_unlock(&room->room_mutex);
}

// Broadcast message to all clients in the chat room except the sender
void broadcast_message(ChatRoom *room, int sender_socket, const char *message) {
    pthread_mutex_lock(&room->room_mutex);
    ClientNode *current = room->clients;
    while (current != NULL) {
        if (current->socket != sender_socket) { // Don't send to the sender
            if (send(current->socket, message, strlen(message), 0) < 0) {
                perror("Failed to send message to client");
            }
        }
        current = current->next;
    }
    pthread_mutex_unlock(&room->room_mutex);
}

// Cleanup all chat rooms and free resources
void cleanup_chat_rooms(ChatRoomList *list) {
    pthread_mutex_lock(&list->list_mutex);
    ChatRoom *current = list->head;
    while (current != NULL) {
        ChatRoom *temp = current;
        current = current->next;

        // Clean up client list
        pthread_mutex_lock(&temp->room_mutex);
        ClientNode *client = temp->clients;
        while (client != NULL) {
            ClientNode *client_temp = client;
            client = client->next;
            free(client_temp);
        }
        pthread_mutex_unlock(&temp->room_mutex);
        pthread_mutex_destroy(&temp->room_mutex);
        free(temp);
    }
    list->head = NULL;
    pthread_mutex_unlock(&list->list_mutex);
    pthread_mutex_destroy(&list->list_mutex);
}
