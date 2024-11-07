#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "session.h"
char *generate_token()
{
    char *token = malloc(32);
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand(time(NULL));
    for (size_t i = 0; i < 31; i++)
    {
        token[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    token[31] = '\0';
    return token;
}
void init_session_manager(SessionManager *manager)
{
    manager->head = NULL;
    pthread_mutex_init(&manager->mutex, NULL);
}
// Add a session to the linked list
void add_session(SessionManager *manager, int userID, const char *token)
{
    pthread_mutex_lock(&manager->mutex);

    UserSession *new_session = malloc(sizeof(UserSession));
    if (new_session == NULL)
    {
        pthread_mutex_unlock(&manager->mutex);
        return; // Memory allocation failed
    }

    new_session->userID = userID;
    strncpy(new_session->token, token, sizeof(new_session->token));
    new_session->next = manager->head;
    manager->head = new_session;

    pthread_mutex_unlock(&manager->mutex);
}

// Find a session by token
UserSession *find_session(SessionManager *manager, const char *token)
{
    pthread_mutex_lock(&manager->mutex);

    UserSession *current = manager->head;
    while (current != NULL)
    {
        if (strcmp(current->token, token) == 0)
        {
            pthread_mutex_unlock(&manager->mutex);
            return current; // Session found
        }
        current = current->next;
    }

    pthread_mutex_unlock(&manager->mutex);
    return NULL; // Session not found
}

// Remove a session from the linked list
void remove_session(SessionManager *manager, const char *token)
{
    pthread_mutex_lock(&manager->mutex);

    UserSession *current = manager->head;
    UserSession *prev = NULL;

    while (current != NULL)
    {
        if (strcmp(current->token, token) == 0)
        {
            if (prev == NULL)
            {
                // Removing the head
                manager->head = current->next;
            }
            else
            {
                prev->next = current->next;
            }
            free(current);
            pthread_mutex_unlock(&manager->mutex);
            return;
        }
        prev = current;
        current = current->next;
    }

    pthread_mutex_unlock(&manager->mutex);
}

// Free all sessions (for cleanup at the end of the program)
void free_sessions(SessionManager *manager)
{
    pthread_mutex_lock(&manager->mutex);

    UserSession *current = manager->head;
    while (current != NULL)
    {
        UserSession *temp = current;
        current = current->next;
        free(temp);
    }

    pthread_mutex_unlock(&manager->mutex);
    pthread_mutex_destroy(&manager->mutex);
}

void print_sessions(SessionManager *manager)
{
    pthread_mutex_lock(&manager->mutex);

    UserSession *current = manager->head;
    while (current != NULL)
    {
        printf("User ID: %d, Token: %s\n", current->userID, current->token);
        current = current->next;
    }
    pthread_mutex_unlock(&manager->mutex);
}