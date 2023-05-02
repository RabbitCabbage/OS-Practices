#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// 1 Loc here: declare mutex
pthread_mutex_t mutex;
void *thread1(void* dummy){
    int i;
    // 1 Loc: mutex operation
    pthread_mutex_lock(&mutex);
    printf("This is thread 1!\n");
    for(i = 0; i < 20; ++i){
        printf("H");
        printf("e");
        printf("l");
        printf("l");
        printf("o");
        printf("W");
        printf("o");
        printf("r");
        printf("l");
        printf("d");
        printf("!");
    }
    // 1 Loc: mutex operation
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *thread2(void* dummy){
    int i;
    // 1 Loc: mutex operation
    pthread_mutex_lock(&mutex);
    printf("This is thread 2!\n");
    for(i = 0; i < 20; ++i){
        printf("A");
        printf("p");
        printf("p");
        printf("l");
        printf("e");
        printf("?");
    }
    // 1 Loc: mutex operation
    pthread_mutex_unlock(&mutex);
    return NULL;
}
int main(){
    pthread_t pid[2];
    int i;
    // 3 Locs here: create 2 thread using thread1 and thread2 as function.
    pthread_create(&pid[0], NULL, thread1, NULL);
    pthread_create(&pid[1], NULL, thread2, NULL);
    // mutex initialization
    pthread_mutex_init(&mutex, NULL);
    for(i = 0; i < 2; ++i){
        // 1 Loc code here: join thread
        pthread_join(pid[i], NULL);
    }
    return 0;
}
