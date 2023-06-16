#ifndef TASK_LIST_H
#define TASK_LIST_H
#include <stdlib.h>

struct node{
    struct node* next;
    struct node* prev;
    struct task_struct* c;
};

struct task_list {
    struct node* head;
};

void add_to_list(struct task_list* l, struct task_struct* c) {
    struct node* tmp = (struct node*)malloc(sizeof(struct node));
    tmp->c = c;
    tmp->prev = NULL;
    tmp->next = l->head;
    if(l->head != NULL) l->head->prev = tmp;
    l->head = tmp;
}

struct task_struct* pop(struct task_list* l) {
    if (l->head == NULL) return NULL;
    struct node* head = l->head;
    l->head = head->next;
    struct task_struct* c = head->c;
    free(head);
    l->head->prev = NULL;
    return c;
}

void remove_from_list(struct task_list* l, struct node* n){
    if(n->prev != NULL) n->prev->next = n->next;
    if(n->next != NULL) n->next->prev = n->prev;
    if(n == l->head) l->head = n->next;
    free(n);
}
#endif