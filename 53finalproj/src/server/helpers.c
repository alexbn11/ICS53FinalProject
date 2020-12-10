#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "helpers.h"
#include "linkedList.h"

/*
 *
 * User methods
 *
 */
void addUser(List_t *list, char *name, int fileDescriptor) {

}

// HELPER: Finds the user in the given list by their username
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

// Free a user from the the user list
void cleanUsers(List_t *list) {
    while (list->head != NULL) {
        removeUser(list, NULL);
    }
}

// Protcool USRLIST
char *getUserList(List_t *list, int client) {
    char *names = NULL;
    node_t *head = list->head;
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        if (user->fd != client) {
            if (names == NULL) {
                names = calloc(strlen(user->name) + 1, strlen(user->name) + 1);;
            }
            strcat(names, user->name);
            strcat(names, "\n");
        }
        head = head->next;
    }
    return names;
}

// Prints list of all users on the server side
void printUserList(List_t* list) {
    node_t *head = list->head;
    printf("Online users:\n");
    while (head != NULL) {
        user_t *user = (user_t *) head->value;
        printf("%s %d\n", user->name, user->fd);
        head = head->next;
    }
}

/*
 *
 * Message methods
 *
 */
// Gets user from USRSEND
char *getUserFromSent(char *body) {
    const char delimiter[] = "\r";
    return strtok(body, delimiter);
}

// Gets message from USRSEND
char *getMessageFromSent(char *body) {
    char *message = body;
    while (*message != '\n') {
        message += 1;
    }
    return message + 1;
}

/*
 *
 * Job methods
 *
 */

// Free a job
void cleanJob(job_t *job) {
    if (job->data != NULL)
        free(job->data);
    free(job);
}

void cleanJobs(List_t *list) {
    while (list->head != NULL) {
        job_t *job = (job_t *) removeFront(list);
        cleanJob(job);
    }
}


/*
 *
 * Room methods
 *
 */

void addRoom(List_t *list, char *name, char *host) {
    List_t *roomUsers = malloc(sizeof(List_t));
    roomUsers->head = NULL;
    roomUsers->length = 0;

    room_t *newRoom = malloc(sizeof(room_t));
    newRoom->roomName = name;
    newRoom->host = host;
    newRoom->users = roomUsers;

    insertRear(list, newRoom);
}

 // Find a room from a list of rooms
room_t *findRoom(List_t *list, char *name) {
    node_t *head = list->head;
    while (head != NULL) {
        room_t *roomList = (room_t *) head->value;
        if (strcmp(name, roomList->roomName) == 0) {
            return roomList;
        }
        head = head->next;
    }
    return NULL;
}

void joinRoom(List_t *list, char *name) {
    room_t *room = findRoom(list, name);
}

room_t *findUserInRoom(room_t *room, char *user) {
    if (strcmp(room->host, user) == 0) {
        return room;
    }
    List_t *users = room->users;
    int index = 0;
    node_t *head = users->head;
    while (head != NULL) {
        char *username = head->value;
        if (strcmp(username, user) == 0) {
            return room;
        }
        head = head->next;
    }
    return NULL;
}

void removeUserFromRoom(room_t *room, char *user) {
    List_t *users = room->users;
    if (user == NULL) {
        char *user = removeFront(users);
        free(user);
    }
    else {
        node_t *head = users->head;
        int index = 0;
        while (head != NULL) {
            char *username = head->value;
            if (strcmp(username, user) == 0) {
                head = head->next;
                removeByIndex(users, index);
                free(username);
                return;
            }
            else {
                index += 1;
                head = head->next;
            }
        }
    }
}

// Protocol RMDELETE
void deleteRoom(List_t *list, char *name) {
    if (name == NULL) {
        room_t *room = (room_t *) removeFront(list);
        free(room->roomName);
        free(room->host);
        while (room->users->head != NULL) {
            removeUserFromRoom(room, NULL);
        }
        free(room->users);
        free(room);
    }
    else {
        node_t *head = list->head;
        int index = 0;
        while (head != NULL) {
            room_t *room = (room_t *) head->value;
            if (strcmp(room->roomName, name) == 0) {
                removeByIndex(list, index);
                free(room->roomName);
                free(room->host);
                while (room->users->head != NULL) {
                    removeUserFromRoom(room, NULL);
                }
                free(room->users);
                free(room);
                return;
            }
            else {
                index += 1;
                head = head->next;
            }
        }
    }
}


// Protocol RMLIST
char *getRoomList(List_t *list) {
    char *rooms = NULL;
    node_t *head = list->head;
    while (head != NULL) {
        room_t *room = (room_t *) head->value;
        if (rooms == NULL) {
            rooms = calloc(strlen(room->roomName), strlen(room->roomName));
        }
        strcat(rooms, room->roomName);
        strcat(rooms, ": ");
        strcat(rooms, room->host);

        List_t *users = room->users;
        node_t *userHead = users->head;
        
        while (userHead != NULL) {
            char *user = userHead->value;
            rooms = realloc(rooms, strlen(rooms) + strlen(user) + 2);
            strcat(rooms, ", ");
            strcat(rooms, user);
            userHead = userHead->next;
        }
        strcat(rooms, "\n");
        head = head->next;
    }
    return rooms;
}

// Prints room list (room name and users in the room)
void printRooms(List_t *list) {
    node_t *head = list->head;
    if (head == NULL) {
        printf("No Rooms\n");
    }
    else {
        while (head != NULL) {
            room_t *roomList = (room_t *) head->value;
            List_t *userList = (List_t *) roomList->users;
            node_t *usernode = userList->head; // pointer to the User Linked List
            printf("Room %s:", roomList->roomName);
            printf("Room %s:", roomList->roomName);
            while (usernode != NULL) {
                user_t *user = (user_t *) usernode->value;
                printf("%s", user->name);
                usernode = usernode->next;
            }

            head = head->next;
        }
    }
}


// Clean the room list
void cleanRooms(List_t *list) {
    while (list->head != NULL) {
        deleteRoom(list, NULL);
    }
}