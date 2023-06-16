#include "coroutine.h"
#include "task_list.h"
#include "task_array.h"

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "pthread.h"

struct task_struct{
    cid_t coroutine_id;
    struct task_struct *parent_coroutine;
    int (*routine)(void);
    int status;
    int return_value;
    jmp_buf env;
    struct task_list waiting_list;
    char *stack_pointer;
    struct list_node* node_ptr;
};

__thread struct co_manager{
    int inited;
    struct task_struct *current_task;
    struct task_struct *main_task;
    struct task_struct *coroutines;// begin from the first coroutine
    struct task_struct coroutine_array[MAXN+1];// begin from main coroutine
    struct node nodes[MAXN+1];
    struct task_struct* available_array[MAXN+1];
    int total;
    int available;
    int unfinished;
} manager;

void init_manager(){
    manager.coroutine_array[0] = (struct task_struct){.coroutine_id = -1,.routine = NULL,.status = RUNNING};
    manager.main_task = manager.current_task = manager.coroutine_array;
    manager.coroutines = manager.coroutine_array+1;
    manager.available_array[0] = manager.main_task;
    manager.total = 0; 
    manager.available = 1;
    manager.unfinished = 0;
    manager.inited = 1;
}

void run_coroutine(struct task_struct *task){
    // printf("funcrtion: run_coroutine\n");
    // printf("current coroutine: %lld, ", manager.current_task->coroutine_id);
    // printf("run_coroutine: %lld\n", task->coroutine_id);
    struct task_struct * last_task = manager.current_task;
    task->status = RUNNING;
    manager.current_task = task;
    if(setjmp(last_task->env) == 0){
        // printf("setjump in run_coroutine\n");
        char *new_sp = task->stack_pointer+65536;
        int (*routine)(void) = task->routine;
        // printf("run_coroutine: %lld\n", task->coroutine_id);
        asm __volatile__(
            "movq %0, %%rsp\n\t"
            "call *%1   \n\t"
            "mov %%rax, %%rdi  \n\t"
            "call afterrun    \n\t"
            ::"r"(new_sp), "r"(routine)
        );
    }
    // printf("I am back to run_coroutine\n");
    // printf("current coroutine: %lld\n", manager.current_task->coroutine_id);
}

void switch_to_another(){
    // printf("funcrtion: switch_to_another\n");
    struct task_struct* next_task = manager.available_array[0];
    if(next_task->coroutine_id == manager.current_task->coroutine_id){
        if(manager.available>1){
            next_task = manager.available_array[1];
        }
        else {
            // printf("no available coroutine\n");
        }
    }
    // printf("switch_to_another: %lld\n", next_task->coroutine_id);
    if(next_task->status == WAITING)run_coroutine(next_task);
    else {
        manager.current_task = next_task;
        // printf("longjump_to: %lld\n", next_task->coroutine_id);
        longjmp(next_task->env, 1);
    }
}

void afterrun(int retval){
    // printf("funcrtion: afterrun\n");
    manager.current_task->return_value = retval;
    // printf("the return value of coroutine %lld is %d\n", manager.current_task->coroutine_id, retval);
    manager.current_task->status = FINISHED;
    manager.unfinished--;
    // those waiting for current task will be available, add to the available array of the manager
    add_all_to_array(manager.available_array, &manager.available, &manager.current_task->waiting_list);
    // todo 要把main加回去，导致main的available_array[0]不是main了！！！！！
    // and this task will be removed from the available list of managernnn
    remove_from_array(manager.available_array, &manager.available, manager.current_task);
    // printf("afterrun: %lld\n", manager.current_task->coroutine_id);
    switch_to_another();
}

int co_start(int (*routine)(void)){
    if(!manager.inited){
        init_manager();
    }
    struct task_struct *new_task = &manager.coroutines[manager.total];
    new_task->coroutine_id = manager.total;
    new_task->routine = routine;
    new_task->stack_pointer = (char*)malloc(65536);
    new_task->waiting_list.head = NULL;
    new_task->parent_coroutine = manager.current_task;
    new_task->status = WAITING;
    add_to_array(manager.available_array, &manager.available, new_task);
    manager.total++;
    manager.unfinished++;
    cid_t cid = new_task->coroutine_id;
    run_coroutine(new_task);
    // printf("co_start: %lld finished\n", cid);
    return cid;
}

int co_getid(){
    return (int)manager.current_task->coroutine_id;
}

int co_getret(int cid){
    return manager.coroutines[cid].return_value;
}

int is_parent_of(struct task_struct *task){
    if(manager.current_task==task||manager.current_task==task->parent_coroutine)return 1;
    if(task->parent_coroutine!=NULL)return is_parent_of(task->parent_coroutine);
    return 0;
}
int co_status(int cid){
    if(is_parent_of(&manager.coroutines[cid]))  return manager.coroutines[cid].status;
    else return UNAUTHORIZED;
}

int co_yield(){
    struct task_struct *task = manager.current_task;
    int x = setjmp(task->env);
    // printf("setjump in co_yield\n");
    if(x==0){
        switch_to_another();
    }
    return 0;
}

int co_waitall(){
    while(manager.unfinished>0){
        co_yield();
    }
    return 0;
}

int co_wait(int cid){
    struct task_struct *cur, *task;
    int flag=0;
    cur = manager.current_task;
    task = &manager.coroutines[cid];
    if(task->status!=FINISHED){
        add_to_list(&task->waiting_list, cur);
        remove_from_array(manager.available_array, &manager.available, cur);
        flag = 1;
    }
    if(flag==0)return 0;
    else{
        int x = setjmp(cur->env);
        // printf("setjump in co_wait\n");
        if(x==0){
            switch_to_another();
        }
    }
    return 0;
}