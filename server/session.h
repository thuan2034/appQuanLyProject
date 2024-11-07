#ifndef SESSION_H
#define SESSION_H

#include <stdlib.h>
#include <pthread.h>
char *generate_token();
typedef struct UserSession {
    int userID;
    char token[32];           // Store the token for the session
    struct UserSession *next; // Pointer to the next session (for the linked list)
} UserSession;

typedef struct {
    UserSession *head;        // Head of the linked list
    pthread_mutex_t mutex;    // Mutex for thread safety
} SessionManager;
// Function prototypes
void init_session_manager(SessionManager *manager);
void add_session(SessionManager *manager, int userID, const char *token);
UserSession* find_session(SessionManager *manager, const char *token);
void remove_session(SessionManager *manager, const char *token);
void free_sessions(SessionManager *manager);
void print_sessions(SessionManager *manager);
#endif // SESSION_H
