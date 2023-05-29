#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "socketchat.h"

int port = 4450;

int set_block_flag(int fd, int blocking)
{
    if (fd < 0) return 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return 0;
    }
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    int ret = fcntl(fd, F_SETFL, flags);
    if (ret < 0) {
        perror("fcntl");
        return 0;
    }
    return 1;
}

int server_accept(){
    int sockfd, connfd;
    struct sockaddr_in addr;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 初始化地址结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);   // 指定端口号
    addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定套接字到地址
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));

    // 监听套接字
    listen(sockfd, 5);

    printf("Waiting for client to connect...\n");
    // 接受连接请求，并处理客户端发送的数据
    connfd = accept(sockfd, NULL, NULL);
    if(connfd > 0) printf("New client connected\n");
    else {
        printf("accept error\n");
        close(sockfd);
        return -1;
    }
    printf("set non-blocking flag\n");
    set_block_flag(sockfd, 0);
    set_block_flag(connfd, 0);
    return connfd;
}

// 0 for read and 1 for write
int server_rw(char *buffer, int size, int rw, int connfd) {
    // 处理客户端发送的数据
    if(rw==0){
        memset(buffer, 0, size);
        printf("Waiting for client to send data...\n");
        ssize_t nread = read(connfd, buffer, size);
        if(nread < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) printf("no data to read\n");
            else printf("read error");
            return -1;
        }
        else{
            // printf("server received %ld bytes: %s\n", nread, buffer);
            return nread;
        }
    } else {
        ssize_t nwrite = write(connfd, buffer, size);
        if(nwrite < 0) {
            printf("write error");
            return -1;
        } else {
            // printf("server sent %ld bytes: %s\n", nwrite, buffer);
            return nwrite;
        }
    }
}

int client_connect(){
    int sockfd;
    struct sockaddr_in addr;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("socket error\n");
        return -1;
    }
    // 初始化地址结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);   // 指定端口号
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // 连接到服务器
    printf("Connecting to server...\n");
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    printf("Connected to server\n");

    // 设置套接字为非阻塞模式
    set_block_flag(sockfd, 0);
    return sockfd;
}

int client_rw(char *buffer, int size, int rw, int sockfd){
    // 处理缓冲区中的数据
    if(rw==0){
        memset(buffer, 0, size);
        printf("Waiting for server to send data...\n");
        ssize_t nread = read(sockfd, buffer, size);
        if (nread < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) printf("no data to read\n");
            else printf("read error");
            return -1;
        }       
        else {
            // printf("client received %ld bytes: %s\n", nread, buffer);
            return nread;
        }
    } else {
        ssize_t nwrite = write(sockfd, buffer, size);
        if(nwrite < 0) {
            printf("write error");
            return -1;
        } else{
            // printf("client sent %ld bytes: %s\n", nwrite, buffer);
            return nwrite;
        }
    }
}
