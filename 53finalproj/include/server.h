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

void run_server(int server_port, int numThreads);

#endif
