// File to help manage stuff
#ifndef HELPERS_H
#define HELPERS_H

#include "server.h"
#include "linkedList.h"

// Holds User information (username, file descriptor)
typedef struct User {
    char *name;
    int fd;
} user_t;

// Holds Room information (room name, creator, current users in the room)
typedef struct Room {
    char *roomName;
    char *host;
    List_t allUsers; // maybe make this char **
} room_t;

// Holds Job information
typedef struct Job {
    int client;
    uint8_t protocol;
    char *data;
} job_t;

char *getUserList(List_t *list, int client);
void printUserList(List_t* list);
user_t *findUserByName(List_t *list, char* name);
user_t *findUserByFd(List_t *list, int fd);
void removeUser(List_t *list, char *name);
char *getUserFromSent(char *body);
char *getMessageFromSent(char *body);
void cleanUsers(List_t *list);
void cleanJob(job_t *job);
void cleanRooms(List_t *list);

#endif