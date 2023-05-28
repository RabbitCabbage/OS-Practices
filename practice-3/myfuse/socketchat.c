// using socket to communicate between different fuses

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define SOCK_PATH "/tmp/myfuse.sock"
#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    int s, t, len;
    struct sockaddr_un remote;
    char str[BUF_SIZE];

    if (argc != 2) {
        printf("Usage: %s <string>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    printf("Connected.\n");

    if (send(s, argv[1], strlen(argv[1]), 0) == -1) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    if ((t = recv(s, str, BUF_SIZE, 0)) > 0) {
        str[t] = '\0';
        printf("echo> %s\n", str);
    } else {
        if (t < 0) perror("recv");
        else printf("Server closed connection\n");
        exit(EXIT_FAILURE);
    }

    close(s);

    return 0;
}