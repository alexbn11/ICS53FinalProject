#include "linkedList.h"
/*
    What is a linked list?
    A linked list is a set of dynamically allocated nodes, arranged in
    such a way that each node contains one value and one pointer.
    The pointer always points to the next member of the list.
    If the pointer is NULL, then it is the last node in the list.

    A linked list is held using a local pointer variable which
    points to the first item of the list. If that pointer is also NULL,
    then the list is considered to be empty.
    -------------------------------               ------------------------------              ------------------------------
    |HEAD                         |             \ |              |             |            \ |              |             |
    |                             |-------------- |     DATA     |     NEXT    |--------------|     DATA     |     NEXT    |
    |-----------------------------|             / |              |             |            / |              |             |
    |LENGTH                       |               ------------------------------              ------------------------------
    |PRINTER                      |
    |DELETER                      |
    -------------------------------

*/

void insertFront(List_t* list, void* valref) {
    if (list->length == 0)
        list->head = NULL;

    node_t** head = &(list->head);
    node_t* new_node;
    new_node = malloc(sizeof(node_t));

    new_node->value = valref;

    new_node->next = *head;
    *head = new_node;
    list->length++; 
}

void insertRear(List_t* list, void* valref) {
    if (list->length == 0) {
        insertFront(list, valref);
        return;
    }

    node_t* head = list->head;
    node_t* current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    current->next = malloc(sizeof(node_t));
    current->next->value = valref;
    current->next->next = NULL;
    list->length++;
}


void* removeFront(List_t* list) {
    node_t** head = &(list->head);
    void* retval = NULL;
    node_t* next_node = NULL;

    if (list->length == 0) {
        return NULL;
    }

    next_node = (*head)->next;
    retval = (*head)->value;
    list->length--;

    node_t* temp = *head;
    *head = next_node;
    free(temp);

    return retval;
}

void* removeRear(List_t* list) {
    if (list->length == 0) {
        return NULL;
    } else if (list->length == 1) {
        return removeFront(list);
    }

    void* retval = NULL;
    node_t* head = list->head;
    node_t* current = head;

    while (current->next->next != NULL) { 
        current = current->next;
    }

    retval = current->next->value;
    free(current->next);
    current->next = NULL;

    list->length--;

    return retval;
}

/* indexed by 0 */
void* removeByIndex(List_t* list, int index) {
    if (list->length <= index) {
        return NULL;
    }

    node_t** head = &(list->head);
    void* retval = NULL;
    node_t* current = *head;
    node_t* prev = NULL;
    int i = 0;

    if (index == 0) {
        retval = (*head)->value;
        
		node_t* temp = *head;
        *head = current->next;
        free(temp);
        
		list->length--;
        return retval;
    }

    while (i++ != index) {
        prev = current;
        current = current->next;
    }

    prev->next = current->next;
    retval = current->value;
    free(current);

    list->length--;

    return retval;
}

void deleteList(List_t* list) {
    if (list->length == 0)
        return;
    while (list->head != NULL){
        removeFront(list);
    }
    list->length = 0;
}

void printList(List_t* list, char mode){
    node_t *head = list->head;
    while (head != NULL)
    {
        printf("%p\n", head->value);
        //fprintf(stderr, "\n");
        head = head->next;
    }
}
