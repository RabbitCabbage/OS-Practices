/* YOUR CODE HERE */
#include <setjmp.h>
#include "coroutine.h"
#include "coroutine_vector.h"
#include "yielded_list.h"
#include <stdio.h>

struct task_struct* current_coroutine = NULL;
cid_t coroutine_count = 0;
struct coroutine_vector* coroutines = NULL;
struct list* yield_list = NULL;

void __attribute__((constructor)) init_containers(){
        printf("initializing containers\n");
        coroutines = (struct coroutine_vector*)malloc(sizeof(struct coroutine_vector));
        coroutines->array_pointer = (struct task_struct *)malloc(100*sizeof(struct task_struct));
        coroutines->size = 0;
        coroutines->capacity = 100;
        yield_list = (struct list*)malloc(sizeof(struct list));
        yield_list->head_node = (struct list_node *)malloc(sizeof(struct list_node));//create head
        yield_list->head_node->prev = NULL;
        yield_list->head_node->next = NULL;//init head
        current_coroutine = add_coroutine(coroutines,-1,current_coroutine,NULL);
        yield_list->head_node->coroutine_task = current_coroutine;
        yield_list->head_node->coroutine_id = -1;
        yield_list->tail_node = yield_list->head_node;
        printf("end initializing containers\n");
}

void __attribute__((destructor)) destroy_containers(){
        free((void *)coroutines->array_pointer);
        free((void *)coroutines);
        free((void *)yield_list->head_node);
        free((void *)yield_list);
}

int afterrun(int ret_val){
    printf("after run\n");
    printf("current_coroutine->coroutine_id = %lld\n",current_coroutine->coroutine_id);
    current_coroutine->status = FINISHED;
    current_coroutine->return_value = ret_val;
    printf("delete current coroutine from yield list\n");
    delete_coroutine_yieldlist(yield_list,current_coroutine);
    // wait relationship will be checked if the coroutine is finished
    // because we cannot record all the coroutines which wait for the current coroutine
    // we can easily check whether one coroutine is waiting for other coroutines
    // free((void *)current_coroutine->stack_pointer);// cannot free because we are still using this stack
    cid_t finished = current_coroutine->coroutine_id;
    printf("find next coroutine\n");
    current_coroutine = find_first_coroutine(yield_list);
    printf("after run %lld, come to %lld\n",finished,current_coroutine->coroutine_id);
    return finished;
}

int co_start(int (*routine)(void)){ 
    printf("the very beginning of co_start\n");
    printf("current_coroutine->coroutine_id = %lld\n",current_coroutine->coroutine_id);
    if(setjmp(current_coroutine->env)==0){
        // not from the coroutine, conduct the starting of the coroutine
        printf("co_start\n");
        struct task_struct *new_coroutine = add_coroutine(coroutines,coroutine_count++,current_coroutine,routine);
        if(current_coroutine->coroutine_id!=-1 && current_coroutine->status!=FINISHED)
            add_yielded_coroutine(yield_list,current_coroutine->coroutine_id,current_coroutine);
        current_coroutine = new_coroutine;
        // malloc the stack
        int *new_sp = (int *)malloc(1024*sizeof(int));
        current_coroutine->stack_pointer = new_sp;
        printf("coroutine_id = %lld\n",current_coroutine->coroutine_id);
        cid_t finished;
        asm __volatile__(
            "movq %1, %%rsp\n\t"
            "call *%2   \n\t"
            "mov %%rax, %%rdi  \n\t"
            "call afterrun    \n\t"
            "mov %%rax, %0\n\t"
           :"=r"(finished)
           :"r"(new_sp), "r"(routine)
        );
        return finished;
    } else {
        // from some coroutine
        printf("from some coroutine\n");
        return current_coroutine->coroutine_id;
    }
}

int co_getid(){
    return current_coroutine->coroutine_id;
}

int co_getret(int cid){
    return get_coroutine(coroutines,cid)->return_value;
}

int co_yield(){
    printf("co_yield\n");
    // print out the info for the current coroutine env
    printf("current_coroutine->coroutine_id = %lld\n",current_coroutine->coroutine_id);
    printf("current_coroutine->env = %p\n",current_coroutine->env);
    if(setjmp(current_coroutine->env)==0){
        printf("yield from %lld\n",current_coroutine->coroutine_id);
        struct task_struct *yield_to = find_first_coroutine(yield_list);
        add_yielded_coroutine(yield_list,current_coroutine->coroutine_id,current_coroutine);
        if(yield_to->coroutine_id==current_coroutine->coroutine_id){
            yield_to = yield_list->head_node->coroutine_task;
        }
        current_coroutine = yield_to;
        printf("yield to %lld\n",yield_to->coroutine_id);
        printf("longjmp\n");
        longjmp(current_coroutine->env,1);
    } else {   
        return current_coroutine->coroutine_id;
    }
}

int co_waitall(){
    while(yield_list->head_node->next!=NULL){
        co_yield();
    }
    return 0;
}

int co_wait(int cid){
    //current coroutine should wait for cid
    printf(" coroutine %lld waits for coroutine %d.\n",current_coroutine->coroutine_id,cid);
    current_coroutine->wait_for = get_coroutine(coroutines,cid);
    printf("wait for %lld\n",current_coroutine->wait_for->coroutine_id);
    printf("the status of the coroutine waited for is %d\n",current_coroutine->wait_for->status);
    if(current_coroutine->wait_for->status == RUNNING)
         co_yield();
    printf("current coroutine %lld is running\n",current_coroutine->coroutine_id);
    printf("the list node of the current coroutine is %lld\n",(long long)current_coroutine->node_ptr);
    return current_coroutine->coroutine_id;
}

int co_status(int cid){
    return get_coroutine(coroutines,cid)->status;
}