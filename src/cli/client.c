#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2){
        printf("Usage: %s <host ip>\r\n", argv[0]);
        return 0;
    }

    struct sockaddr_in serverInfo = {0};

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);
    serverInfo.sin_port = htons(5555);

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == fd) {
        perror("socket");
        return -1;
    }

    if (-1 == connect(fd, (struct sockaddr *)&serverInfo, sizeof(serverInfo))) {
        perror("connect");
        close(fd);
        return -1;
    }

    return 0;
}