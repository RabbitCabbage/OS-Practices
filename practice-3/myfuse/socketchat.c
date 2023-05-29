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

int set_block_flag(int fd, int blocking)
{
    if (fd < 0) return 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return 0;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? 1 : 0;
}

// 0 for read and 1 for write
void server(char *buffer, int size, int rw) {
    int sockfd, connfd;
    struct sockaddr_in addr;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 初始化地址结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);   // 指定端口号
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
        return;
    }
    set_block_flag(connfd, 0);

    // 处理客户端发送的数据
    if(rw==0){
        memset(buffer, 0, size);
        ssize_t nread = 0;
        int attempts = 0;
        while (nread <= 0 && attempts < 10) {
            nread = read(connfd, buffer, size);
            if (nread == -1) {
                if (errno == EAGAIN) {
                    struct timeval tv;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    select(1, NULL, NULL, NULL, &tv);
                    attempts++;
                } else {
                    perror("read error");
                    break;
                }
            }
        }
        printf("server received %ld bytes: %s\n", nread, buffer);
    } else {
        ssize_t nwrite = 0;
        int attempts = 0;
        while (nwrite <= 0 && attempts < 10) {
            nwrite = write(connfd, buffer, size);
            if (nwrite == -1) {
                if (errno == EAGAIN) {
                    struct timeval tv;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    select(1, NULL, NULL, NULL, &tv);
                    attempts++;
                } else {
                    perror("write error");
                    break;
                }
            }
        }
        printf("server sent %ld bytes: %s\n", nwrite, buffer);
    }

    // 关闭连接
    close(connfd);
    printf("Client disconnected\n");
    // 关闭套接字
    close(sockfd);
}

void client(char *buffer, int size, int rw){
    int sockfd;
    struct sockaddr_in addr;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 初始化地址结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);   // 指定端口号
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    // 连接到服务器
    connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));

    // 设置套接字为非阻塞模式
    set_block_flag(sockfd, 0);
    
    // 处理缓冲区中的数据
    if(rw==0){
        memset(buffer, 0, size);
        ssize_t nread = 0;
        int attempts = 0;
        while (nread <= 0 && attempts < 10) {
            nread = read(sockfd, buffer, size);
            if (nread == -1) {
                if (errno == EAGAIN) {
                    struct timeval tv;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    select(1, NULL, NULL, NULL, &tv);
                    attempts++;
                } else {
                    perror("read error");
                    break;
                }
            }
        }
        printf("client received %ld bytes: %s\n", nread, buffer);
    } else {
        ssize_t nwrite = 0;
        int attempts = 0;
        while (nwrite <= 0 && attempts < 10) {
            nwrite = write(sockfd, buffer, size);
            if (nwrite == -1) {
                if (errno == EAGAIN) {
                    struct timeval tv;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    select(1, NULL, NULL, NULL, &tv);
                    attempts++;
                } else {
                    perror("write error");
                    break;
                }
            }
        }
        printf("client sent %ld bytes: %s\n", nwrite, buffer);
    }

    // 关闭连接
    close(sockfd);
}
