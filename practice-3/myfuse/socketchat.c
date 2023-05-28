#include <sys/socket.h>
#include <netinet/in.h>
#include "socketchat.h"

int set_block_flag(int fd, int blocking)
{
    if (fd < 0) return 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return 0;
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? 1 : 0;
}

void server() {
    int sockfd;
    struct sockaddr_in addr;

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // 初始化地址结构体
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);       // 申请一个未使用的本地端口
    addr.sin_addr.s_addr = INADDR_ANY;

    // 绑定套接字到地址
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));

    // 获取绑定后的地址和端口号
    struct sockaddr_in local_addr;
    socklen_t addrlen = sizeof(local_addr);
    getsockname(sockfd, (struct sockaddr*)&local_addr, &addrlen);
    printf("Local address: %s:%d\n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
    
    set_block_flag(sockfd, 0);

    // 监听套接字
    listen(sockfd, 5);

    // 接受连接
    int connfd;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    while (1) {
        connfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_addrlen);
        if (connfd != -1) {
            printf("Client address: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            set_block_flag(connfd, 0);
            break;
        }
    }
    fd_set readfds = allfds;
    int ret = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (ret < 0) {
        // 发生错误，退出循环
        break;
    }

    for (int i = 0; i <= maxfd; ++i) {
        if (FD_ISSET(i, &readfds)) {
            if (i == sockfd) {
                // 处理新的连接请求
            } else {
                // 处理已连接的客户端 Socket 上的数据
                char buf[1024];
                ssize_t nread = recv(i, buf, sizeof(buf), MSG_DONTWAIT);
                if (nread > 0) {
                    // 将收到的数据添加到缓冲区中
                    strncat(buffer, buf, nread);
                } else if (nread == 0 || (nread < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
                    // 客户端关闭了连接或发生错误，从监听列表中移除该 Socket
                    FD_CLR(i, &allfds);
                    close(i);
                }
            }
        }
    }

}

void server(){

}
