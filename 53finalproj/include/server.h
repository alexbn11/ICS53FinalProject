#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "linkedList.h"

#define BUFFER_SIZE 1024
#define SA struct sockaddr

#define ROOM_LIMIT 5

// Holds User information (username, file descriptor)
struct User {
    char *name;
    int fileDescriptor;
} user_t;

// Holds Room information (room name, creator, current users in the room)
struct Room {
    char *roomName;
    char *host;
    List_t currentUsers;
} room_t;

// Holds Job information
struct Job {
    uint8_t protocol;
    char *data;
} job_t;

void run_server(int server_port);

#endif
