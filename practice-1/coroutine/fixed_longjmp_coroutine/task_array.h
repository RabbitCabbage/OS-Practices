#ifndef TASK_array_H
#define TASK_array_H

#include <stdlib.h>
#include <assert.h>

void add_to_array(struct task_struct** array, int* size, struct task_struct* c) {
    array[*size] = c;
    ++*size;
}

void remove_from_array(struct task_struct** array, int* size, struct task_struct* c) {
    int i = 0;
    while (array[i] != c && i < *size) {
        i++;
    }
    assert(i < *size && array[i]==c);
    while (i + 1 < *size) {
        array[i] = array[i + 1];
        i++;
    }
    *size = *size - 1;
}

void add_all_to_array(struct task_struct** array, int* size, struct task_list* l) {
    struct node* n = l->head;
    while (n != NULL) {
        add_to_array(array, size, n->c);
        n = n->next;
    }
}

#endif