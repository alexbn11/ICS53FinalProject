#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "helpers.h"
#include "linkedList.h"


void printUserList(List_t* list) {
    node_t *head = list->head;
    printf("Online users:\n");
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        printf("    %s\n", user->name);
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

void cleanUsers(List_t *list) {
    while (list->head != NULL) {
        removeUser(list, NULL);
    }
}

void cleanRooms(List_t *list) {
}