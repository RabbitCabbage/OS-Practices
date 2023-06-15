/* YOUR CODE HERE */
#ifndef COROUTINE_H
#define COROUTINE_H

typedef long long cid_t;
#include <ucontext.h>
#define MAXN (50000)
#define UNAUTHORIZED (-1)
#define FINISHED (2)
#define RUNNING (1)

struct task_struct{
    cid_t coroutine_id;
    struct task_struct *parent_coroutine;
    int (*routine)(void);
    int status;
    int return_value;
    struct task_struct* wait_for;
    int *stack_pointer;
    struct list_node* node_ptr;
    ucontext_t context;
};

int co_start(int (*routine)(void));
int co_getid();
int co_getret(int cid);
int co_yield();
int co_waitall();
int co_wait(int cid);
int co_status(int cid);

#endif
