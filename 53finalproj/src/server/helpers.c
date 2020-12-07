#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "helpers.h"
#include "linkedList.h"

char *getUserList(List_t *list, int client) {
    char *names = NULL;
    node_t *head = list->head;
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        if (user->fd != client) {
            if (names == NULL) {
                names = calloc(strlen(user->name) + 1, strlen(user->name) + 1);
                strcat(names, user->name);
                strcat(names, "\n");
            }
            else {
                names = realloc(names, strlen(names) + strlen(user->name) + 1);
                strcat(names, user->name);
                strcat(names, "\n");
            }
        }
        head = head->next;
    }
    return names;
}

void printUserList(List_t* list) {
    node_t *head = list->head;
    printf("Online users:\n");
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        printf("%s %d\n", user->name, user->fd);
        head = head->next;
    }
}

// Finds the user in the given list by their username
user_t *findUserByName(List_t *list, char* name) {
    node_t *head = list->head;
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        if (strcmp(user->name, name) == 0) {
            return user;
        }
        head = head->next;
    }
    return NULL;
}

// Finds the user in the given list by their fd
user_t *findUserByFd(List_t *list, int fd) {
    node_t *head = list->head;
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        if (user->fd == fd) {
            return user;
        }
        head = head->next;
    }
    return NULL;
}

// Remove user from linked list
void removeUser(List_t *list, char *name) {
    if (name == NULL) {
        user_t *user = (user_t *) removeFront(list);
        free(user->name);
        close(user->fd);
        free(user);
    }
    else {
        int index = 0;
        node_t *head = list->head;
        while (head != NULL) {
            user_t *user = (user_t *) head->value;
            if (strcmp(user->name, name) == 0) {
                head = head->next;
                removeByIndex(list, index);
                free(user->name);
                close(user->fd);
                free(user);
            }
            else {
                index += 1;
                head = head->next;
            }
        }
    }
}

char *getUserFromSent(char *body) {
    const char delimiter[] = "\r";
    return strtok(body, delimiter);
}

char *getMessageFromSent(char *body) {
    char *message = body;
    while (*message != '\n') {
        message += 1;
    }
    return message + 1;
}

// Clean the user list
void cleanUsers(List_t *list) {
    while (list->head != NULL) {
        removeUser(list, NULL);
    }
}

void cleanJob(job_t *job) {
    free(job->data);
    job->data = NULL;
    free(job);
}

// Clean the room list
void cleanRooms(List_t *list) {
}