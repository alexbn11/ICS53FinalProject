#include "server.h"
#include "protocol.h"
#include <pthread.h>
#include <signal.h>
#include "linkedList.h"

const char exit_str[] = "exit";

char buffer[BUFFER_SIZE];
pthread_mutex_t buffer_lock;

int total_num_msg = 0;
int listen_fd;

// Audit Log
List_t *auditLog;

// User database
List_t *users;

// Room database 
List_t *rooms; 

// Job buffer/queue
List_t *jobs;

// What happens when Ctrl-C is pressed when the server is running
void sigint_handler(int sig) {
    printf("shutting down server\n");
    close(listen_fd);
    char c[] = "shutting down server";
    // insertRear(auditLog, (char *) c);
    exit(0);
}

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
    } else
        printf("Socket successfully binded\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    } else{
        printf("Server listening on port: %d.. Waiting for connection\n", server_port);
        char c[] = "initalizing server: s";
        //insertRear(auditLog, (char *) c);
    }
    return sockfd;
}

// Processes client (NEED TO CHANGE THIS)
void *process_client(void *clientfd_ptr) {
    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);
    int received_size;
    fd_set read_fds;

    int retval;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);
        if (retval != 1 && !FD_ISSET(client_fd, &read_fds)) {
            printf("Error with select() function\n");
            break;
        }

        pthread_mutex_lock(&buffer_lock);

        bzero(buffer, BUFFER_SIZE);
        received_size = read(client_fd, buffer, sizeof(buffer));

        if (received_size < 0) {
            printf("Receiving failed\n");
            break;
        } else if (received_size == 0) {
            continue;
        }

        petr_header *petrHeader = (petr_header *) buffer;
        printf("MSG_LEN: %d\n", petrHeader->msg_len);
        printf("MSG_TYPE: %d\n", petrHeader->msg_type);
        char messageBody[petrHeader->msg_len];
        memcpy(messageBody, &buffer[8], petrHeader->msg_len);
        printf("MSG_BODY: %s\n", messageBody);

        if (strncmp(exit_str, buffer, sizeof(exit_str)) == 0) {
            printf("Client exit\n");
            break;
        }

        total_num_msg++;
        // print buffer which contains the client contents
        // printf("Receive message from client: %s\n", buffer);
        // printf("Total number of received messages: %d\n", total_num_msg);

        sleep(1); //mimic a time comsuming process

        // and send that buffer to client
        // int ret = write(client_fd, buffer, received_size);


        int ret = 0;
        // LOGIN request
        if (petrHeader->msg_type == LOGIN) {
            // TODO: check if username is not taken

            // adds user to user list and sends OK
            petr_header petr = {0, OK};
            petrHeader = &petr;
            ret = wr_msg(client_fd, petrHeader, buffer);
        }
        
        pthread_mutex_unlock(&buffer_lock);

        if (ret < 0) {
            printf("Sending failed\n");
            break;
        }
        printf("Send the message back to client: %s\n", buffer);
    }
    // Close the socket at the end
    printf("Close current client connection\n");
    close(client_fd);
    return NULL;
}

// Main thread (server)
void run_server(int server_port) {
    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    pthread_t tid;

    while (1) {
        // Wait and Accept the connection from client
        printf("Wait for new client connection\n");
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA *)&client_addr, (socklen_t*)&client_addr_len);
        if (*client_fd < 0) {
            printf("Server acccept failed\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Client connetion accepted\n");
            pthread_create(&tid, NULL, process_client, (void *)client_fd);
        }
    }
    bzero(buffer, BUFFER_SIZE);
    close(listen_fd);
    return;
}

int main(int argc, char *argv[]) {
    auditLog = malloc(sizeof(List_t));
    users = malloc(sizeof(List_t));
    int opt;

    unsigned int port = 0;
    while ((opt = getopt(argc, argv, "hjp:")) != -1) {
        switch (opt) {
        case 'h':
            fprintf(stderr, "./bin/petr_server [-h] [-j N] PORT_NUMBER AUDIT_FILENAME\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "-h                Displays this help menu, and returns EXIT_SUCCESS\n");
            fprintf(stderr, "-j N              Number of job threads. Default to 2.\n");
            fprintf(stderr, "AUDIT_FILENAME    File to output Audit Log messages to.\n");
            fprintf(stderr, "PORT_NUMBER       Port number to listen to.\n");
            exit(EXIT_SUCCESS);
        case 'p':
            port = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (port == 0) {
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "Server Application Usage: %s -p <port_number>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    run_server(port);

    return 0;
}