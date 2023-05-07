#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void fail(const char* message, const char* function, int line){
    printf("[x] Test failed at %s: %d: %s\n", function, line, message);
    exit(-1);
}

#endif