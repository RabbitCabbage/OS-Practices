#ifndef COROUTINE_VECTOR_H
#define COROUTINE_VECTOR_H
#include "coroutine.h"
#include <stdlib.h>
#include <stdio.h>

struct coroutine_vector{
    struct task_struct* array_pointer;
    int size;
    int capacity;
};


//add a node when a new coroutine is created
//if mem is not enough, realloc
struct task_struct* add_coroutine(struct coroutine_vector *vector,cid_t coroutine_id, struct task_struct* current_coroutine,int (*routine)(void)){
        long long index = coroutine_id + 1;
        if(index>=vector->capacity){
            vector->capacity = vector->capacity*2;
            vector->array_pointer = (struct task_struct *)realloc(vector->array_pointer, vector->capacity*sizeof(struct task_struct));
        }
        (*(struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index)).coroutine_id = coroutine_id;
        (*(struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index)).parent_coroutine = current_coroutine;
        (*(struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index)).routine = routine;
        (*(struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index)).status = RUNNING;
        (*(struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index)).node_ptr = NULL;
        (*(struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index)).wait_for = NULL;
        return (struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index);
}

void finish_coroutine(struct coroutine_vector*vector,cid_t coroutine_id){
        long long index = coroutine_id + 1;
        (*(struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index)).status = FINISHED;
    }
    
struct task_struct* get_coroutine(struct coroutine_vector*vector,cid_t coroutine_id){
        long long index = coroutine_id + 1;
        return (struct task_struct*)((long long)vector->array_pointer+sizeof(struct task_struct)*index);
    }

#endif