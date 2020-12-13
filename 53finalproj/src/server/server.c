#include "server.h"
#include "protocol.h"
#include <pthread.h>
#include <signal.h>
#include "linkedList.h"
#include "helpers.h"
#include <time.h>

const char exit_str[] = "exit";

char buffer[BUFFER_SIZE];
pthread_mutex_t buffer_lock;
int reqcnt = 0;

pthread_mutex_t job_lock;

pthread_mutex_t server_lock;

pthread_mutex_t run_lock;
int usercnt = 0;
int rmcnt = 0;

pthread_mutex_t user_lock;
pthread_mutex_t room_lock;


int listen_fd;

char *auditLog = NULL;

// Audit Log
void writeToAudit(char c[])
{
    FILE *fp;

    char buff[20];
    struct tm *sTm;
    time_t now = time(0);
    sTm = gmtime(&now);
    strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm);

    fp = fopen(auditLog, "a");
    fprintf(fp, "%s", c);
    fprintf(fp, " %s\n", buff);

    fclose(fp);
}

void readFromAudit()
{
    FILE *fp;
    char c;
    fp = fopen(auditLog, "r");
    do
    {
        c = fgetc(fp);
        if (feof(fp))
        {
            break;
        }
        printf("%c", c);
    } while (1);
    fclose(fp);
}

void clearAuditLog()
{
    FILE *fp;
    fp = fopen(auditLog, "w");
    fclose(fp);
}

// User database
List_t *users;

// Room database
List_t *rooms;

// Job buffer/queue
List_t *jobs;

// What happens when Ctrl-C is pressed when the server is running
volatile sig_atomic_t running = 1;
void server_sigint_handler(int sig)
{
    running = 0;
    close(listen_fd);
}

// Starts the server
int server_init(int server_port)
{
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0)
    {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Socket successfully binded\n");
    }

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0)
    {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Server listening on port: %d.. Waiting for connection\n", server_port);
    }
    return sockfd;
}

// Job thread
void *job_thread()
{
    pthread_detach(pthread_self());
    printf("Job thread created!\n");
    while (running)
    {
        job_t *currJob = NULL;
        pthread_mutex_lock(&job_lock);
        if (jobs->length > 0)
        {
            currJob = (job_t *) removeFront(jobs);
            printf("Protocol: %d\n", currJob->protocol);
        }
        pthread_mutex_unlock(&job_lock);

        // // Should writer or reader get priority over linked list
        if (currJob != NULL)
        {
            if (currJob->protocol == USRLIST) // protocol USRLIST (COMPLETE)
            { 
                char *userList = getUserList(users, currJob->fd);
                uint32_t size = 0;

                if (userList != NULL)
                    size = strlen(userList) + 1;

                petr_header *res = makeHeader(size, USRLIST);
                wr_msg(currJob->fd, res, userList);
                free(res);

                if (userList != NULL)
                    free(userList);
            }
            else if (currJob->protocol == USRSEND) // protocol USRSEND
            { 
                user_t *fromUser = findUserByFd(users, currJob->fd);

                char *receiver = getUserFromSent(currJob->data);
                user_t *toUser = findUserByName(users, receiver);
                if (toUser != NULL && toUser->fd != currJob->fd)
                {
                    char *message = getMessageFromSent(currJob->data);
                    char *recvMessage = makeUserMessage(fromUser->name, message);

                    uint32_t size = strlen(recvMessage) + 1;
                    petr_header *res = makeHeader(size, USRRECV);
                    wr_msg(toUser->fd, res, recvMessage);
                    res->msg_len = 0;
                    res->msg_type = OK;
                    wr_msg(currJob->fd, res, NULL);
                    free(res);

                    if (recvMessage != NULL)
                        free(recvMessage);
                }
                else
                {
                    petr_header *res = makeHeader(0, EUSRNOTFOUND);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
            }
            else if (currJob->protocol == RMCREATE) // protocol RMCREATE
            { 
                // Writer gets priority over room linked list and user linked list
                char *roomName = calloc(strlen(currJob->data), strlen(currJob->data));
                strcpy(roomName, currJob->data);

                if (*roomName == 0) // checks if null room name (DOESN'T WORK)
                { 
                    petr_header *res = makeHeader(0, ESERV);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
                else if (findRoom(rooms, roomName) == NULL)
                {
                    user_t *user = findUserByFd(users, currJob->fd);
                    char *host = calloc(strlen(user->name), strlen(user->name));
                    strcpy(host, user->name);

                    addRoom(rooms, roomName, host);

                    petr_header *res = makeHeader(0, OK);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
                else
                {
                    petr_header *res = makeHeader(0, ERMEXISTS);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
            }
            else if (currJob->protocol == RMJOIN) // protocol RMJOIN
            { 
                char *roomName = currJob->data;
                if (*roomName == 0) // checks if null room name (DOESN'T WORK)
                { 
                    petr_header *res = makeHeader(0, ESERV);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
                else
                {
                    room_t *room = findRoom(rooms, roomName);
                    if (room == NULL)
                    {
                        petr_header *res = makeHeader(0, ERMNOTFOUND);
                        wr_msg(currJob->fd, res, NULL);
                        free(res);
                    }
                    else if (room->users->length + 1 == ROOM_LIMIT)
                    {
                        petr_header *res = makeHeader(0, ERMFULL);
                        wr_msg(currJob->fd, res, NULL);
                        free(res);
                    }
                    else
                    {
                        user_t *user = findUserByFd(users, currJob->fd);
                        if (!findUserInRoom(room, user->name))
                        {
                            char *name = calloc(strlen(user->name), strlen(user->name));
                            strcpy(name, user->name);
                            insertRear(room->users, name);
                        }
                        petr_header *res = makeHeader(0, OK);
                        wr_msg(currJob->fd, res, NULL);
                        free(res);   
                    }
                }
            }
            else if (currJob->protocol == RMLIST) // protocol RMLIST (this leaks, need to fix)
            {
                char *roomList = getRoomList(rooms);
                uint32_t size = 0;

                if (roomList != NULL)
                    size = strlen(roomList) + 1; // need to change this

                petr_header *res = makeHeader(size, RMLIST);
                wr_msg(currJob->fd, res, roomList);
                // printf("%s\n", roomList);
                free(res);

                if (roomList != NULL)
                    free(roomList);
            }
            else if (currJob->protocol == RMLEAVE) // protocol RMLEAVE
            { 
                char *roomName = currJob->data;
                if (roomName == 0)
                {
                    petr_header *res = makeHeader(0, ERMNOTFOUND);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
                else
                {
                    char *requester = findUserByFd(users, currJob->fd)->name;
                    room_t *room = findRoom(rooms, roomName);
                    if (room == NULL || strcmp(requester, room->host) == 0)
                    {
                        petr_header *res = makeHeader(0, ERMDENIED);
                        wr_msg(currJob->fd, res, NULL);
                        free(res);
                    }
                    else
                    {
                        removeUserFromRoom(room, requester);
                        petr_header *res = makeHeader(0, OK);
                        wr_msg(currJob->fd, res, NULL);
                        free(res);
                    }
                }
            }
            else if (currJob->protocol == RMDELETE) // protocol RMDELETE
            {
                char *roomName = currJob->data;
                if (roomName == 0) // This checks if room is empty string with null terminator (I don't quite if this works) 
                {
                    petr_header *res = makeHeader(0, ERMNOTFOUND);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
                else
                {
                    char *requester = findUserByFd(users, currJob->fd)->name;
                    room_t *room = findRoom(rooms, roomName);
                    if (room == NULL || strcmp(requester, room->host) != 0) // If room is not in the list or the request is not the host
                    {
                        petr_header *res = makeHeader(0, ERMDENIED);
                        wr_msg(currJob->fd, res, NULL);
                        free(res);
                    }
                    else // Deleting Room
                    {
                        node_t *head = room->users->head;
                        petr_header *res = makeHeader(strlen(roomName) + 1, RMCLOSED);
                        while (head != NULL)
                        {
                            char *name = head->value;
                            user_t *currUser = findUserByName(users, name);
                            wr_msg(currUser->fd, res, roomName);
                            head = head->next;
                        }

                        deleteRoom(rooms, roomName);

                        res->msg_len = 0;
                        res->msg_type = OK;
                        wr_msg(currJob->fd, res, NULL);
                    }
                }
            }
            else if (currJob->protocol == RMSEND) // protocol RMSEND
            { 
                user_t *fromUser = findUserByFd(users, currJob->fd);

                char *roomName = getUserFromSent(currJob->data);
                room_t *room = findRoom(rooms, roomName);
                if (room != NULL) // room is in list of rooms
                {
                    petr_header *res = makeHeader(0, OK);
                    wr_msg(currJob->fd, res, NULL);

                    char *message = getMessageFromSent(currJob->data);
                    char *recvMessage = makeRoomMessage(roomName, fromUser->name, message);

                    uint32_t size = strlen(recvMessage) + 1;

                    res->msg_len = size;
                    res->msg_type = RMRECV;

                    node_t *head = room->users->head;

                    if (currJob->fd != findUserByName(users, room->host)->fd)
                        wr_msg(findUserByName(users, room->host)->fd, res, recvMessage);

                    while (head != NULL)
                    {
                        char *name = head->value;
                        user_t *currUser = findUserByName(users, name);
                        if (currUser->fd != currJob->fd)
                        {
                            wr_msg(currUser->fd, res, recvMessage);
                        }
                        head = head->next;
                    }
                    
                    free(res);
                    
                    if (recvMessage != NULL)
                        free(recvMessage);
                }
                else
                {
                    petr_header *res = makeHeader(0, ERMNOTFOUND);
                    wr_msg(currJob->fd, res, NULL);
                    free(res);
                }
            }
            else if (currJob->protocol == LOGOUT) // protocol LOGOUT
            { 
                user_t *user = findUserByFd(users, currJob->fd);
                node_t *head = rooms->head;
                while (head != NULL)
                {
                    room_t *room = (room_t *)head->value;
                    if (findUserInRoom(room, user->name))
                    {
                        if (strcmp(room->host, user->name) == 0)
                        {
                            node_t *userHead = room->users->head;
                            petr_header *res = makeHeader(strlen(room->roomName) + 1, RMCLOSED);
                            while (userHead != NULL)
                            {
                                char *name = userHead->value;
                                user_t *currUser = findUserByName(users, name);
                                wr_msg(currUser->fd, res, room->roomName);
                                userHead = userHead->next;
                            }
                            head = head->next;
                            deleteRoom(rooms, room->roomName);
                            free(res);
                        }
                        else
                        {
                            removeUserFromRoom(room, user->name);
                            head = head->next;
                        }
                    }
                    else
                    {
                        head = head->next;
                    }
                }
                petr_header *res = makeHeader(0, OK);
                wr_msg(currJob->fd, res, NULL);
                free(res);
                removeUser(users, user->name);
            }
            cleanJob(currJob);
        }
    }
    printf("Job thread killed!\n");
    pthread_exit(NULL);
}

// Client thread
void *client_thread(void *clientfd)
{
    pthread_detach(pthread_self());
    printf("Making client thread\n");
    int client_fd = *(int *) clientfd;
    free(clientfd);
    int received_size;
    fd_set read_fds;

    int retval;

    while (running)
    {
        bzero(buffer, BUFFER_SIZE);

        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);
        // printf("client_fd\n");
        if (retval != -1 && !FD_ISSET(client_fd, &read_fds))
        {
            printf("Error with select() function\n");
            break;
        }

        pthread_mutex_lock(&buffer_lock);
        reqcnt += 1;
        if (reqcnt == 1)
            pthread_mutex_lock(&job_lock);
        pthread_mutex_unlock(&buffer_lock);

        received_size = read(client_fd, buffer, sizeof(buffer));
        if (received_size < 0)
        {
            printf("Client message failed to send. Disconnecting client.\n");
            pthread_mutex_lock(&buffer_lock);
            reqcnt -= 1;
            if (reqcnt == 0)
                pthread_mutex_unlock(&job_lock);
            pthread_mutex_unlock(&buffer_lock);
            break;
        }
        else if (received_size == 0)
        {
            printf("Client disconnected\n");
            pthread_mutex_lock(&buffer_lock);
            reqcnt -= 1;
            if (reqcnt == 0)
                pthread_mutex_unlock(&job_lock);
            pthread_mutex_unlock(&buffer_lock);
            break;
        }

        // Read MSG from Client
        petr_header *petrHeader = (petr_header *) buffer;
        char *messageBody = NULL;
        if (petrHeader->msg_len != 0)
        {
            messageBody = calloc(petrHeader->msg_len, petrHeader->msg_len);
            strcpy(messageBody, &buffer[8]);
        }

        pthread_mutex_lock(&buffer_lock);
        addJob(jobs, client_fd, petrHeader->msg_type, messageBody);
        pthread_mutex_unlock(&buffer_lock);

        pthread_mutex_lock(&buffer_lock);
        reqcnt -= 1;
        if (reqcnt == 0)
            pthread_mutex_unlock(&job_lock);
        pthread_mutex_unlock(&buffer_lock);
    }

    writeToAudit("Terminating Thread");
    printf("Killing client thread\n");

    // Need to delete user from room and delete room
    removeUser(users, findUserByFd(users, client_fd)->name);

    pthread_exit(NULL);
    return NULL;
}

// Main thread (server)
void run_server(int server_port, int numThreads)
{

    // Set up user database
    users = calloc(sizeof(List_t), sizeof(List_t));
    users->head = NULL;
    users->length = 0;

    // Sets up jobs database
    jobs = calloc(sizeof(List_t), sizeof(List_t));
    jobs->head = NULL;
    jobs->length = 0;

    // Sets up rooms database
    rooms = calloc(sizeof(List_t), sizeof(List_t));
    rooms->head = NULL;
    rooms->length = 0;

    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    pthread_t tid;

    // Creates the job threads on start up
    int i;
    for (i = 0; i < numThreads; i++)
        pthread_create(&tid, NULL, job_thread, NULL);

    while (1)
    {
        signal(SIGINT, server_sigint_handler);

        // Wait connection from client
        printf("Wait for new client connection\n");
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA *) &client_addr, (socklen_t *) &client_addr_len);
        writeToAudit("New client connection");

        pthread_mutex_lock(&server_lock);
        usercnt += 1;
        if (usercnt == 1)
            pthread_mutex_lock(&user_lock);
        pthread_mutex_unlock(&server_lock);

        if (*client_fd < 0) // If client connection fails
        { 
            free(client_fd);
            writeToAudit("Client unable to connect");
            printf("Server shutting down\n");
            break;
        }
        else // Accept connection
        { 
            char *c = malloc(256);
            printf("Client connetion accepted\n");

            writeToAudit("Client message accepted");

            // Read header information from client
            bzero(buffer, BUFFER_SIZE);
            int size = read(*client_fd, buffer, sizeof(buffer));
            petr_header *req = (petr_header *) buffer;

            // Check if client is trying to LOGIN
            if (req->msg_type == LOGIN)
            {

                writeToAudit("New LOGIN message from client");

                // Get user name that client wants to use
                char *name = calloc(req->msg_len, req->msg_len);
                strcpy(name, &buffer[8]);

                // Verify user name
                if (*name == 0) // checks if null user name (DOESN'T WORK)
                { 
                    petr_header *res = makeHeader(0, ESERV);
                    wr_msg(*client_fd, res, NULL);
                    free(name);
                }
                else if (findUserByName(users, name) == NULL) // If username not already taken
                { 
                    // Add to user list
                    addUser(users, name, *client_fd);

                    // Respond with OK message from server
                    petr_header *res = makeHeader(0, OK);
                    wr_msg(*client_fd, res, NULL);
                    free(res);

                    printf("New user online: %s\n", name);

                    // Writing to audit log
                    strcpy(c, "Client successfully login as: ");
                    strcat(c, name);
                    strcat(c, "");
                    writeToAudit(c);
                    free(c);

                    // Spawn client thread
                    // free(client_fd);
                    pthread_create(&tid, NULL, client_thread, (void *) client_fd);
                    writeToAudit("Making client thread.");
                }
                else
                { // If it is already taken  
                    free(c);
                    writeToAudit("Client rejected because username already exists");

                    // Reject login
                    petr_header *res = makeHeader(0, EUSREXISTS);
                    wr_msg(*client_fd, res, NULL);

                    // Close connection
                    close(*client_fd);
                    free(client_fd);
                    free(name);
                }
                bzero(buffer, BUFFER_SIZE);
            }
            else
            {
                printf("No LOGIN message from client. Exiting.\n");
                writeToAudit("No LOGIN message from client.");
                close(*client_fd);
            }
        }
        pthread_mutex_lock(&server_lock);
        usercnt -= 1;
        if (usercnt == 0)
            pthread_mutex_unlock(&user_lock);
        pthread_mutex_unlock(&server_lock);
    }
    bzero(buffer, BUFFER_SIZE);
    close(listen_fd);
    return;
}

int main(int argc, char *argv[])
{
    int opt;

    unsigned int port = 0;
    unsigned int n = 2;
    while ((opt = getopt(argc, argv, "hj:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "-h                Displays this help menu, and returns EXIT_SUCCESS\n");
            fprintf(stderr, "-j N              Number of job threads. Default to 2.\n");
            fprintf(stderr, "AUDIT_FILENAME    File to output Audit Log messages to.\n");
            fprintf(stderr, "PORT_NUMBER       Port number to listen to.\n");
            exit(EXIT_SUCCESS);
        case 'j':
            n = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
            exit(EXIT_FAILURE);
        }
    }

    // optind intialized to 1 by the system,
    // getopt updates it when it processes the option characters
    // argv[i]: ./bin/petr_server 3200
    if (*(argv + optind) != NULL)
    {
        port = atoi(*(argv + optind));
    }
    else
    {
        fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
        exit(EXIT_FAILURE);
    }

    // argv[i+1]: ./bin/petr_server 3200 audit.txt
    if (*(argv + optind + 1) != NULL) // create custom audit log or default
        auditLog =  *(argv + optind + 1);
    else
        auditLog = "auditLog.txt";

    if (port == 0)
    {
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
        exit(EXIT_FAILURE);
    }

    if (n == 0)
    {
        fprintf(stderr, "ERROR: Number of job threads for server is not given\n");
        fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
        exit(EXIT_FAILURE);
    }

    clearAuditLog(); // clear previous audits before start
    run_server(port, n);

    printf("Cleaning up.\n");
    cleanUsers(users);
    free(users);
    cleanRooms(rooms);
    free(rooms);
    cleanJobs(jobs);
    free(jobs);

    return 0;
}