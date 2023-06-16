#ifndef SOCKETCHAT_H
#define SOCKETCHAT_H
int set_block_flag(int fd, int blocking);
int server_accept();
int server_rw(char *buffer, int size, int rw, int connfd);
int client_connect();
int client_rw(char *buffer, int size, int rw, int sockfd);
#endif