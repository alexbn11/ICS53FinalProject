#include "chat.h"
#include "debug.h"
#include "protocol.h"
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

// Couple example ANSI color codes
#define YELLOW "\x1B[1;33m"
#define BLUE "\x1B[1;34m"

// you can customize this print here!
void print_username(char *name)
{
    char buff[20];
    time_t now = time(NULL);//gets raw time Jan 1, 1970 00:00:00 UTC
    struct tm *UTC;
    UTC = localtime(&now);//gets local time
    strftime(buff, sizeof(buff), "Sent: %H:%M %p", UTC);//formats time

    printf(YELLOW "%*s" KNRM, 80, name);
    printf("%*s", 80, buff);
    printf("\n");
    fflush(stdout);
}

// you can customize this print here!
void print_msg(char *msg)
{
    printf(BLUE "%*s" KNRM, 80, msg);
    printf("\n");
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    int sockfd = atoi(argv[1]);  // path to the socket file
    char *my_username = argv[2]; // the username used by the client
    char *chatname = argv[3];    // the name of the chat or room you are in
    int is_room = atoi(argv[4]); // is this a chat room or a dm

    // we are going to poll on the stdin and the socket.
    // this is an example of i/o multiplexing using poll
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    printf("       _.---._    /\\\\");      printf(YELLOW "          ____       __       " KNRM);   printf(BLUE" ________          __\n"KNRM);
    printf("    ./'       \"--`\\//");      printf(YELLOW"         / __ \\___  / /______"KNRM);     printf(BLUE" / ____/ /_  ____ _/ /_\n"KNRM);
    printf("  ./              o \\");       printf(YELLOW"        / /_/ / _ \\/ __/ ___/"KNRM);     printf(BLUE"/ /   / __ \\/ __ `/ __/\n"KNRM);
    printf(" /./\\  )______   \\__ \\");    printf(YELLOW"      / ____/  __/ /_/ /"KNRM);           printf(BLUE"   / /___/ / / / /_/ / /_  \n"KNRM);
    printf("./  / /\\ \\   | \\ \\  \\ \\");printf(YELLOW"    /_/    \\___/\\__/_/"KNRM);           printf(BLUE"    \\____/_/ /_/\\__,_/\\__/\n"KNRM);
    printf("   / /  \\ \\  | |\\ \\  \\7\n");
    printf("    \"     \"    \"  \"     \n");

    printf("Welcome to Petr Chat\n");
    printf("================================================================================\n");

    while (1)
    {
        // poll on the two descriptors we care about until some event occurs (no timing out)
        int ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            perror("poll error");
            exit(EXIT_FAILURE);
        }
        // we have to loop over all the descriptors to find which one had the event
        for (int i = 0; i < 2; i++)
        {
            // if the file descriptor encountered an error or a hangup, close everything but stay open
            if (fds[i].revents & POLLERR || fds[i].revents & POLLHUP)
            {
                close(STDIN_FILENO);
                close(sockfd);
                // go to sleep forever
                sigset_t mask;
                sigemptyset(&mask);
                sigsuspend(&mask);
                exit(1);
            }
            if (fds[i].revents & POLLIN && fds[i].fd == STDIN_FILENO)
            {
                char *buf = NULL;
                size_t n = 0;
                ssize_t len = getline(&buf, &n, stdin);
                if (len < 0)
                {
                    perror("getline error");
                    fatal("Unexpected getline failure.\n");
                }
                // send the line including the null terminator
                if (send(sockfd, buf, len + 1, 0) < 0)
                {
                    error("Send failed\n");
                }
                free(buf);
            }
            if (fds[i].revents & POLLIN && fds[i].fd == sockfd)
            {
                // first get the user
                char *from_user = NULL;
                size_t n = 0;
                ssize_t len = getdelimfd(&from_user, &n, '\n', sockfd);
                if (len < 0)
                {
                    perror("getdelimfd error");
                    fatal("Unexpected getdelimfd failure.\n");
                }
                // the from_user string will have \r\n in it if the protocol
                // is implemented correctly
                print_username(from_user);
                free(from_user);
                // then get the rest of message
                char *msg = NULL;
                len = getdelimfd(&msg, &n, '\0', sockfd);
                if (len < 0)
                {
                    perror("getdelimfd error");
                    fatal("Unexpected getdelimfd failure.\n");
                }
                // the msg string will have a null terminator if the protocol
                // is implemented correctly
                print_msg(msg);
                free(msg);
            }
        }
    }
    return EXIT_SUCCESS;
}