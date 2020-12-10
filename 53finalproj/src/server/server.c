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
pthread_mutex_t send_lock;

int total_num_msg = 0;
int listen_fd;

char *auditLog = NULL;

// Audit Log
void writeToAudit(char c[]){
    FILE *fp;
    
    char buff[20];
    struct tm *sTm;
    time_t now = time (0);
    sTm = gmtime (&now);
    strftime (buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm);
    
    fp = fopen(auditLog,"a");
    fprintf(fp,"%s", c);
    fprintf(fp," %s\n", buff);
    
    fclose(fp);
}

void readFromAudit(){
    FILE *fp;
    char c;
    fp = fopen(auditLog,"r");
    do {
      c = fgetc(fp);
      if( feof(fp) ) {
         break ;
      }
      printf("%c", c);
    } while(1);
    fclose(fp);
}

void clearAuditLog(){
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
void sigint_handler(int sig) {
    printf("shutting down server\n");
    cleanUsers(users);
    free(users);
    cleanJobs(jobs);
    free(jobs);
    cleanRooms(rooms);
    free(rooms);
    close(listen_fd);
    exit(0);
}

// Starts the server
int server_init(int server_port) {
    int sockfd;
    struct sockaddr_in servaddr;
    

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    } else
        printf("Socket successfully created\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    } else{
        printf("Socket successfully binded\n");
        
    }
    
    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    } else{
        printf("Server listening on port: %d.. Waiting for connection\n", server_port);
    }
    return sockfd;
}

// Job thread
void *job_thread(void *threadId){
    long tid;
    tid = (long)threadId;
    
    printf("Job thread created!\n");
    pthread_detach(pthread_self());
    while (1) {
        pthread_mutex_lock(&job_lock);
        if (jobs->length > 0) {
            job_t *currJob = (job_t *) removeFront(jobs);
            printf("Protocol: %d\n", currJob->protocol);
            if (currJob->protocol == USRLIST) { // protocol USRLIST (COMPLETE)
                char *userList = getUserList(users, currJob->fd);
                uint32_t size = 0;

                if (userList != NULL) 
                    size = strlen(userList) + 1;

                petr_header res = {size, USRLIST};
                wr_msg(currJob->fd, &res, userList);

                if (userList != NULL)
                    free(userList);
            }
            else if (currJob->protocol == USRSEND) { // protocol USRSEND
                user_t *fromUser = findUserByFd(users, currJob->fd);

                char *receiver = getUserFromSent(currJob->data);
                user_t *toUser = findUserByName(users, receiver);
                if (toUser != NULL && toUser->fd != currJob->fd) {
                    char *message = getMessageFromSent(currJob->data);

                    char* recvMessage = calloc(strlen(fromUser->name) + 2 + strlen(message), strlen(fromUser->name) + 2 + strlen(message));
                    strcpy(recvMessage, fromUser->name);
                    strcat(recvMessage, "\r\n");
                    strcat(recvMessage, message);

                    uint32_t size = strlen(recvMessage) + 1;
                    petr_header sentMessage = {size, USRRECV};
                    wr_msg(toUser->fd, &sentMessage, recvMessage);

                    petr_header res = {0, OK};
                    wr_msg(currJob->fd, &res, NULL);
                    
                    if (recvMessage != NULL)
                        free(recvMessage);
                }
                else {
                    petr_header res = {0, EUSREXISTS};
                    wr_msg(currJob->fd, &res, NULL);
                }
            }
            else if (currJob->protocol == RMCREATE) { // protocol RMCREATE
                char *roomName = calloc(strlen(currJob->data), strlen(currJob->data));
                strcpy(roomName, currJob->data);
                
                if (*roomName == 0) { // checks if null room name (DOESN'T WORK)
                    petr_header res = {0, ESERV};
                    wr_msg(currJob->fd, &res, NULL);
                }
                else if (findRoom(rooms, roomName) == NULL) {
                    user_t *user = findUserByFd(users, currJob->fd);
                    char *host = calloc(strlen(user->name), strlen(user->name));
                    strcpy(host, user->name);
                    
                    addRoom(rooms, roomName, host);

                    petr_header res = {0, OK};
                    wr_msg(currJob->fd, &res, NULL);
                }
                else {
                    petr_header res = {0, ERMEXISTS};
                    wr_msg(currJob->fd, &res, NULL);
                }
            }
            else if (currJob->protocol == RMJOIN) { // protocol RMJOIN
                char *roomName = currJob->data;
                if (*roomName == 0) { // checks if null room name (DOESN'T WORK)
                    petr_header res = {0, ESERV};
                    wr_msg(currJob->fd, &res, NULL);
                }
                else {
                    room_t *room = findRoom(rooms, roomName);
                    if (room == NULL) {
                        petr_header res = {0, ERMNOTFOUND};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                    else if (room->users->length + 1 == ROOM_LIMIT) {
                        petr_header res = {0, ERMFULL};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                    else {
                        user_t *user = findUserByFd(users, currJob->fd);
                        if (findUserInRoom(room, user->name) == NULL) {
                            char *name = calloc(strlen(user->name), strlen(user->name));
                            strcpy(name, user->name);
                            insertRear(room->users, name);
                        }
                        petr_header res = {0, OK};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                }
            }
            else if (currJob->protocol == RMLIST) { // protocol RMLIST
                char *roomList = getRoomList(rooms);
                uint32_t size = 0;

                if (roomList != NULL) { // checks if null room name (DOESN'T WORK)
                    size = strlen(roomList) + 1;
                }

                petr_header res = {size, RMLIST};
                wr_msg(currJob->fd, &res, roomList);

                if (roomList != NULL)
                    free(roomList);
            }
            else if (currJob->protocol == RMLEAVE) { // protocol RMLEAVE
                char *roomName = currJob->data;
                if (roomName == 0) {
                    petr_header res = {0, ERMNOTFOUND};
                    wr_msg(currJob->fd, &res, NULL);
                }
                else {
                    char *requester = findUserByFd(users, currJob->fd)->name;
                    room_t *room = findRoom(rooms, roomName);
                    if (room == NULL) {
                        petr_header res = {0, ERMDENIED};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                    else if (strcmp(requester, room->host) == 0) {
                        petr_header res = {0, ERMDENIED};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                    else {
                        removeUserFromRoom(room, requester);
                        petr_header res = {0, OK};
                        wr_msg(currJob->fd, &res, NULL);
                        
                    }
                }
            }
            else if (currJob->protocol == RMDELETE) { // this doesn't work
                char *roomName = currJob->data;
                if (roomName == 0) {
                    petr_header res = {0, ERMNOTFOUND};
                    wr_msg(currJob->fd, &res, NULL);
                }
                else {
                    char *requester = findUserByFd(users, currJob->fd)->name;
                    room_t *room = findRoom(rooms, roomName);
                    if (room == NULL) {
                        petr_header res = {0, ERMDENIED};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                    else if (strcmp(requester, room->host) != 0) {
                        petr_header res = {0, ERMDENIED};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                    else {
                        node_t *head = room->users->head;
                        petr_header res = {strlen(roomName), RMCLOSED};
                        while (head != NULL) {
                            char *name = head->value;
                            user_t *currUser = findUserByName(users, name);
                            wr_msg(currUser->fd, &res, roomName);
                            head = head->next;
                        }
                        deleteRoom(rooms, roomName);
                        res = (petr_header) {0, OK};
                        wr_msg(currJob->fd, &res, NULL);
                    }
                }
            }
            else if (currJob->protocol == RMSEND) {
                user_t *fromUser = findUserByFd(users, currJob->fd);

                char *roomName = getUserFromSent(currJob->data);
                room_t *room = findRoom(rooms, roomName);
                if (room != NULL) {
                    char *message = getMessageFromSent(currJob->data);

                    char* recvMessage = calloc(strlen(roomName) + strlen(fromUser->name) + 2 + strlen(message), 
                                               strlen(roomName) + strlen(fromUser->name) + 2 + strlen(message));
                    strcpy(recvMessage, roomName);
                    strcat(recvMessage, "\r\n");
                    strcat(recvMessage, fromUser->name);
                    strcat(recvMessage, "\r\n");
                    strcat(recvMessage, message);
                    printf("%s\n", recvMessage);

                    uint32_t size = strlen(recvMessage) + 1;

                    node_t *head = room->users->head;
                    petr_header res = {size, RMRECV};

                    if (currJob->fd != findUserByName(users, room->host)->fd) {
                        wr_msg(findUserByName(users, room->host)->fd, &res, recvMessage);
                    }

                    while (head != NULL) {
                        char *name = head->value;
                        user_t *currUser = findUserByName(users, name);
                        if (currUser->fd != currJob->fd) {
                            wr_msg(currUser->fd, &res, recvMessage);
                        }
                        head = head->next;
                    }

                    res = (petr_header) {0, OK};
                    wr_msg(currJob->fd, &res, NULL);
                    
                    if (recvMessage != NULL)
                        free(recvMessage);
                }
                else {
                    petr_header res = {0, EUSREXISTS};
                    wr_msg(currJob->fd, &res, NULL);
                }
            }
            // else if (currJob->protocol == LOGOUT) {
            //     user_t *user = findUserByFd(users, currJob->fd);
            //     node_t *head = rooms->head;
            //     while (head != NULL) {
            //         room_t *room = head->value;
            //         if (findUserInRoom(room, user->name) != NULL) {
            //             if 
            //         }

            //     }

            // }
            cleanJob(currJob);
        }
        pthread_mutex_unlock(&job_lock);
    }
    pthread_exit(NULL);
}

// Client thread
void *client_thread(void *clientfd) {
    int client_fd = *(int *)clientfd;
    free(clientfd);
    int received_size;
    fd_set read_fds;
    
    int retval;
   
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);
        if (retval != -1 && !FD_ISSET(client_fd, &read_fds)) {
            printf("Error with select() function\n");
            break;
        }

        pthread_mutex_lock(&buffer_lock);
        reqcnt += 1;
        if (reqcnt == 1)
            pthread_mutex_lock(&job_lock);
        pthread_mutex_unlock(&buffer_lock);

        bzero(buffer, BUFFER_SIZE);
        received_size = read(client_fd, buffer, sizeof(buffer));
        if (received_size < 0) {
            printf("Receiving failed\n");
            break;
        } 
        else if (received_size == 0) {
            continue;
        }

        // Read MSG from Client
        petr_header *petrHeader = (petr_header *) buffer;
        char *messageBody = NULL;
        if (petrHeader->msg_len != 0) {
            messageBody = calloc(petrHeader->msg_len, petrHeader->msg_len);
            memcpy(messageBody, &buffer[8], petrHeader->msg_len);
        }

        job_t *job = malloc(sizeof(job_t));
        job->fd = client_fd;
        job->protocol = petrHeader->msg_type;
        job->data = messageBody;

        // Insert MSG to Job Buffer
        insertRear(jobs, job);

        pthread_mutex_lock(&buffer_lock);
        reqcnt -= 1;
        if (reqcnt == 0)
            pthread_mutex_unlock(&job_lock);
        pthread_mutex_unlock(&buffer_lock);
         
    }
    char c[] = "Terminating Thread";
    writeToAudit(c);
    pthread_exit(NULL);
    return NULL;
}

// Main thread (server)
void run_server(int server_port, int numThreads) {
    
    // Set up user database
    users = malloc(sizeof(List_t));
    users->head = NULL;
    users->length = 0;

    // Sets up jobs database
    jobs = malloc(sizeof(List_t));
    jobs->head = NULL;
    jobs->length = 0;

    // Sets up rooms database
    rooms = malloc(sizeof(List_t));
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

    while (1) {
        // Wait connection from client
        printf("Wait for new client connection\n");
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA *)&client_addr, (socklen_t*)&client_addr_len);

        char *c = malloc(256);
        strcpy(c, "New client connection");
        writeToAudit(c);

        if (*client_fd < 0) {  // If client connection fails
            strcpy(c, "Client unable to connect");
            free(c);
            writeToAudit(c);

            printf("Server acccept failed\n");
            exit(EXIT_FAILURE);
        }
        else { // Accept connection
            printf("Client connetion accepted\n");

            strcpy(c, "Client message accepted");
            writeToAudit(c);

            // Read header information from client
            bzero(buffer, BUFFER_SIZE);
            int size = read(*client_fd, buffer, sizeof(buffer));
            petr_header *req = (petr_header *) buffer;

            // Check if client is trying to LOGIN
            if (req->msg_type == LOGIN) {

                strcpy(c, "New LOGIN message from client");
                writeToAudit(c);

                // Get user name that client wants to use
                char *name = malloc(req->msg_len);
                memcpy(name, &buffer[8], req->msg_len);

                // Verify user name
                if (*name == 0) { // checks if null user name (DOESN'T WORK)
                    petr_header res = {0, ESERV};
                    wr_msg(*client_fd, &res, NULL);
                }
                else if (findUserByName(users, name) == NULL) { // If username not already taken
                    // Add to user list
                    user_t *newUser = malloc(sizeof(user_t));
                    newUser->name = name;
                    newUser->fd = *client_fd;
                    insertRear(users, newUser);

                    // Respond with OK message from server
                    petr_header res = {0, OK};
                    wr_msg(*client_fd, &res, NULL);

                    printf("New user online: %s\n", name);

                    // Writing to audit log
                    strcpy(c, "Client successfully login as: ");
                    strcat(c, name);
                    strcat(c, "");
                    writeToAudit(c);
                    
                    // Spawn client thread
                    pthread_create(&tid, NULL, client_thread, (void *)client_fd); 

                    strcpy(c, "Making client thread.");
                    writeToAudit(c);
                    free(c);
                }
                else { // If it is already taken
                    strcpy(c, "Client rejected because username already exists\n");
                    writeToAudit(c);
                    free(c);

                    // Reject login
                    petr_header res = {0, EUSREXISTS};
                    wr_msg(*client_fd, &res, NULL);

                    // Close connection
                    close(*client_fd);
                    free(client_fd);
                }
                bzero(buffer, BUFFER_SIZE);        
            }
            else {
                printf("No LOGIN message from client. Exiting.\n");

                strcpy(c, "No LOGIN message from client.");
                writeToAudit(c);
                free(c);

                close(*client_fd);
            }
        }
    }
    bzero(buffer, BUFFER_SIZE);
    close(listen_fd);
    return;
}

int main(int argc, char *argv[]) {
    int opt;

    unsigned int port = 0;
    unsigned int n = 2;
    while ((opt = getopt(argc, argv, "hj:")) != -1) {
        switch (opt) {
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
   
    //optind intialized to 1 by the system, 
    //getopt updates it when it processes the option characters
    //argv[i]: ./bin/petr_server 3200 
    port = atoi(*(argv + optind));
   
    //argv[i+1]: ./bin/petr_server 3200 audit.txt
    if(*(argv + optind + 1) != NULL){
        strcpy(auditLog, *(argv + optind + 1));
    }
    else {
        auditLog = "auditLog.txt";
    }

    if (port == 0) {
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
        exit(EXIT_FAILURE);
    }

    if (n == 0) {
        fprintf(stderr, "ERROR: Number of job threads for server is not given\n");
        fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
        exit(EXIT_FAILURE);
    }

    clearAuditLog();
    run_server(port, n);

    return 0;
}
