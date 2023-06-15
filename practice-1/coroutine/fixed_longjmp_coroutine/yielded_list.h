#ifndef YIELDED_LIST_H
#define YIELDED_LIST_H
#include "coroutine.h"
#include <stdlib.h>
#include <stdio.h>

// yielded list, all the coroutines are ready to be resumed
struct list_node{
    cid_t coroutine_id;
    struct task_struct *coroutine_task;
    struct list_node *prev;
    struct list_node *next;
};

struct list{
    struct list_node* head_node;
    struct list_node* tail_node;
};

    //used to add node to the last, 
    //when a coroutine is yielded, we put it on the end of the list
void add_yielded_coroutine(struct list * yielded_list,cid_t coroutine_id, struct task_struct* coroutine_task){
        if(coroutine_task->node_ptr!=NULL)return;//have added to the list, not to add again
        struct list_node * new_node =  (struct list_node *)malloc(sizeof(struct list_node));
        new_node->prev = yielded_list->tail_node;
        new_node->coroutine_id = coroutine_id;
        new_node->coroutine_task = coroutine_task;
        new_node->next = NULL;
        yielded_list->tail_node->next = new_node;
        yielded_list->tail_node =new_node;
        coroutine_task->node_ptr = new_node;
}
    
void delete_coroutine_yieldlist(struct list * yielded_list,struct task_struct* coroutine_task){
        struct list_node* to_delete = coroutine_task->node_ptr;
        if(to_delete == NULL)return;// not in yield list, not to delete
        to_delete->prev->next = to_delete->next;
        if(to_delete->next != NULL)to_delete->next->prev = to_delete->prev;
        if(yielded_list->tail_node==to_delete)yielded_list->tail_node = to_delete->prev;
        free(to_delete);
        coroutine_task->node_ptr = NULL;
    }
    
    // after yielding run the first node of the list
struct task_struct* find_first_coroutine(struct list * yielded_list){
        // we add to the end and then yield, so the list should not be empty
        if(yielded_list->head_node->next==NULL){
            return yielded_list->head_node->coroutine_task;
        }
        struct list_node* to_run = yielded_list->head_node->next;
        while(1){
            if(to_run->coroutine_task->status==RUNNING &&(to_run->coroutine_task->wait_for==NULL||to_run->coroutine_task->wait_for->status==FINISHED))
                break;
            else if(to_run->next==NULL)
                return yielded_list->head_node->coroutine_task;
            else to_run = to_run->next;
        }
        struct task_struct *pointer = to_run->coroutine_task;
        // to_run->prev->next = to_run->next;
        // if(to_run->next != NULL)to_run->next->prev = to_run->prev;
        // free(to_run);
        return pointer;
    }

#endif
