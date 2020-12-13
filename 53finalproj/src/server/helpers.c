#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "helpers.h"

petr_header *makeHeader(uint32_t msg_len, uint8_t msg_type)
{
    petr_header *header = calloc(sizeof(petr_header), sizeof(petr_header));
    header->msg_len = msg_len;
    header->msg_type = msg_type;
    return header;
}

/*
 *
 * User methods
 *
 */
void addUser(List_t *list, char *name, int fileDescriptor)
{
    user_t *newUser = calloc(sizeof(user_t), sizeof(user_t));
    newUser->name = name;
    newUser->fd = fileDescriptor;
    insertRear(list, newUser);
}

// HELPER: Finds the user in the given list by their username
user_t *findUserByName(List_t *list, char *name)
{
    node_t *head = list->head;
    while (head != NULL)
    {
        user_t *user = (user_t *)head->value;
        if (strcmp(user->name, name) == 0)
        {
            return user;
        }
        head = head->next;
    }
    return NULL;
}

// Finds the user in the given list by their fd
user_t *findUserByFd(List_t *list, int fd)
{
    node_t *head = list->head;
    while (head != NULL)
    {
        user_t *user = (user_t *)head->value;
        if (user->fd == fd)
        {
            return user;
        }
        head = head->next;
    }
    return NULL;
}

// Remove user from linked list
void removeUser(List_t *list, char *name)
{
    if (name == NULL)
    {
        user_t *user = (user_t *) removeFront(list);
        free(user->name);
        close(user->fd);
        free(user);
    }
    else
    {
        int index = 0;
        node_t *head = list->head;
        while (head != NULL)
        {
            user_t *user = (user_t *)head->value;
            if (strcmp(user->name, name) == 0)
            {
                removeByIndex(list, index);
                free(user->name);
                close(user->fd);
                free(user);
                return;
            }
            else
            {
                index += 1;
                head = head->next;
            }
        }
    }
}

// Free a user from the the user list
void cleanUsers(List_t *list)
{
    while (list->head != NULL)
    {
        removeUser(list, NULL);
    }
}

// Protcool USRLIST
char *getUserList(List_t *list, int client)
{
    char *names = NULL;
    node_t *head = list->head;
    while (head != NULL)
    {
        user_t *user = (user_t *) head->value;
        if (user->fd != client)
        {
            if (names == NULL)
                names = calloc(strlen(user->name) + 1, strlen(user->name) + 1);

            strcat(names, user->name);
            strcat(names, "\n");
        }
        head = head->next;
    }
    return names;
}

// Prints list of all users on the server side
void printUserList(List_t *list)
{
    node_t *head = list->head;
    printf("Online users:\n");
    while (head != NULL)
    {
        user_t *user = (user_t *)head->value;
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
char *getUserFromSent(char *body)
{
    const char delimiter[] = "\r";
    return strtok(body, delimiter);
}

// Gets message from USRSEND
char *getMessageFromSent(char *body)
{
    char *message = body;
    while (*message != '\n')
    {
        message += 1;
    }
    return message + 1;
}

char *makeUserMessage(char* username, char *message)
{
    char *recvMessage = calloc(strlen(username) + 2 + strlen(message), strlen(username) + 2 + strlen(message));
    strcpy(recvMessage, username);
    strcat(recvMessage, "\r\n");
    strcat(recvMessage, message);
    return recvMessage;
}

char *makeRoomMessage(char *roomName, char* username, char* message)
{
    char *recvMessage = calloc(strlen(roomName), strlen(roomName));
    strcpy(recvMessage, roomName);
    strcat(recvMessage, "\r\n");
    strcat(recvMessage, username);
    strcat(recvMessage, "\r\n");
    strcat(recvMessage, message);
    return recvMessage;
}
/*
 *
 * Job methods
 *
 */

void addJob(List_t *list, int fileDescriptor, u_int8_t protocol, char *data)
{
    job_t *job = calloc(sizeof(job_t), sizeof(job_t));
    job->fd = fileDescriptor;
    job->protocol = protocol;
    job->data = data;

    // Insert MSG to Job Buffer
    insertRear(list, job);
}

// Free a job
void cleanJob(job_t *job)
{
    if (job->data != NULL)
        free(job->data);
    free(job);
}

void cleanJobs(List_t *list)
{
    while (list->head != NULL)
    {
        job_t *job = (job_t *)removeFront(list);
        cleanJob(job);
    }
}

/*
 *
 * Room methods
 *
 */

void addRoom(List_t *list, char *name, char *host)
{
    List_t *roomUsers = calloc(sizeof(List_t), sizeof(List_t));
    roomUsers->head = NULL;
    roomUsers->length = 0;

    room_t *newRoom = calloc(sizeof(room_t), sizeof(room_t));
    newRoom->roomName = name;
    newRoom->host = host;
    newRoom->users = roomUsers;

    insertRear(list, newRoom);
}

// Find a room from a list of rooms
room_t *findRoom(List_t *list, char *name)
{
    node_t *head = list->head;
    while (head != NULL)
    {
        room_t *roomList = (room_t *)head->value;
        if (strcmp(name, roomList->roomName) == 0)
        {
            return roomList;
        }
        head = head->next;
    }
    return NULL;
}

void joinRoom(List_t *list, char *name)
{
    room_t *room = findRoom(list, name);
}

int findUserInRoom(room_t *room, char *user)
{
    if (strcmp(room->host, user) == 0)
    {
        return room;
    }
    List_t *users = room->users;
    int index = 0;
    node_t *head = users->head;
    while (head != NULL)
    {
        char *username = head->value;
        if (strcmp(username, user) == 0)
        {
            return 1;
        }
        head = head->next;
    }
    return 0;
}

void removeUserFromRoom(room_t *room, char *user)
{
    List_t *users = room->users;
    if (user == NULL)
    {
        char *user = removeFront(users);
        free(user);
    }
    else
    {
        node_t *head = users->head;
        int index = 0;
        while (head != NULL)
        {
            char *username = head->value;
            if (strcmp(username, user) == 0)
            {
                removeByIndex(users, index);
                free(username);
                return;
            }
            else
            {
                index += 1;
                head = head->next;
            }
        }
    }
}

// Protocol RMDELETE
void deleteRoom(List_t *list, char *name)
{
    if (name == NULL)
    {
        room_t *room = (room_t *)removeFront(list);
        free(room->roomName);
        free(room->host);
        while (room->users->head != NULL)
        {
            removeUserFromRoom(room, NULL);
        }
        free(room->users);
        free(room);
    }
    else
    {
        node_t *head = list->head;
        int index = 0;
        while (head != NULL)
        {
            room_t *room = (room_t *)head->value;
            if (strcmp(room->roomName, name) == 0)
            {
                removeByIndex(list, index);
                free(room->roomName);
                free(room->host);
                while (room->users->head != NULL)
                {
                    removeUserFromRoom(room, NULL);
                }
                free(room->users);
                free(room);
                return;
            }
            else
            {
                index += 1;
                head = head->next;
            }
        }
    }
}

// Protocol RMLIST
char *getRoomList(List_t *list)
{
    char *roomlist = NULL;
    node_t *head = list->head;
    while (head != NULL)
    {
        room_t *room = (room_t *) head->value;

        if (roomlist == NULL)
            roomlist = calloc(strlen(room->roomName) + 1, strlen(room->roomName) + 1);

        strcat(roomlist, room->roomName);
        strcat(roomlist, ": ");
        strcat(roomlist, room->host);

        node_t *userHead = room->users->head;

        while (userHead != NULL)
        {
            char *user = (char *) userHead->value;
            strcat(roomlist, ",");
            strcat(roomlist, user);
            userHead = userHead->next;
        }
        strcat(roomlist, "\n");
        head = head->next;
    }
    return roomlist;
}

// Prints room list (room name and users in the room)
void printRooms(List_t *list)
{
    node_t *head = list->head;
    if (head == NULL)
    {
        printf("No Rooms\n");
    }
    else
    {
        while (head != NULL)
        {
            room_t *roomList = (room_t *)head->value;
            printf("Room %s:", roomList->roomName);
            List_t *userList = (List_t *)roomList->users;
            node_t *usernode = userList->head; // pointer to the User Linked List
            while (usernode != NULL)
            {
                user_t *user = (user_t *)usernode->value;
                printf("%s", user->name);
                usernode = usernode->next;
            }
            printf("\n");
            head = head->next;
        }
    }
}

// Clean the room list
void cleanRooms(List_t *list)
{
    while (list->head != NULL)
    {
        deleteRoom(list, NULL);
    }
}