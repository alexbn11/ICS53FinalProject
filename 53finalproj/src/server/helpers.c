#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "helpers.h"
#include "linkedList.h"


char *getUserList(List_t *list, int client) {
    char *names = NULL;
    node_t *head = list->head;
    int i = 0;
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        if (user->fd != client) {
            if (names == NULL) {
                names = malloc(sizeof(user->name));
                strcpy(names, user->name);
                strcat(names, "\n");
            }
            else {
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

// Finds the user in the given list
user_t *findUser(List_t *list, char* name) {
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

// Clean the user list
void cleanUsers(List_t *list) {
    while (list->head != NULL) {
        removeUser(list, NULL);
    }
}

// Clean the room list
void cleanRooms(List_t *list) {
}