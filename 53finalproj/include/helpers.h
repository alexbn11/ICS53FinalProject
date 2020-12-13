// File to help manage stuff
#ifndef HELPERS_H
#define HELPERS_H

#include "server.h"
#include "linkedList.h"
#include "protocol.h"

// Holds User information (username, file descriptor)
typedef struct User
{
    char *name;
    int fd;

} user_t;

// Holds Room information (room name, creator, current users in the room)
typedef struct Room
{
    char *roomName;
    char *host;
    List_t *users; // List_t of char*

} room_t;

// Holds Job information
typedef struct Job
{
    int fd;
    uint8_t protocol;
    char *data;

} job_t;

petr_header *makeHeader(uint32_t msg_len, u_int8_t msg_type);

// User methods
void addUser(List_t *list, char *name, int fileDescriptor);
user_t *findUserByName(List_t *list, char *name);
user_t *findUserByFd(List_t *list, int fd);
void removeUser(List_t *list, char *name);
void cleanUsers(List_t *list);
char *getUserList(List_t *list, int client);
void printUserList(List_t *list);

// Message methods
char *getUserFromSent(char *body);
char *getMessageFromSent(char *body);
char *makeUserMessage(char* username, char *message);
char *makeRoomMessage(char *roomName, char* username, char* message);

// Job methods
void addJob(List_t *list, int fileDescriptor, u_int8_t protocol, char *data);
void cleanJob(job_t *job);
void cleanJobs(List_t *list);

// Room methods
void addRoom(List_t *list, char *name, char *host);
room_t *findRoom(List_t *list, char *name);
// void joinRoom(room_t *room, char *name);
room_t *findFirstRoomWithUser(List_t *list, char *user);
int findUserInRoom(room_t *room, char *user);
void removeUserFromRoom(room_t *room, char *user);
void deleteRoom(List_t *list, char *name);
char *getRoomList(List_t *list);
void printRooms(List_t *list);
void cleanRooms(List_t *list);

#endif