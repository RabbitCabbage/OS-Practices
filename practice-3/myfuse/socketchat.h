#ifndef SOCKETCHAT_H
#define SOCKETCHAT_H
void server(char *buffer, int size, int rw);
void client(char *buffer, int size, int rw);
int set_block_flag(int fd, int blocking);
#endif