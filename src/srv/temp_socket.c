#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    struct sockaddr_in serverInfo = {0};
    struct sockaddr_in clientInfo = {0};

    int clientSize = 0;

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = 0;
    serverInfo.sin_port = htons(5555);

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (-1 == fd) {
        perror("socket");
        return -1;
    }

    // Bind
    if (-1 == bind(fd, (struct sockaddr *)&serverInfo, sizeof(serverInfo))) {
        perror("bind");
        close(fd);
        return -1;
    }

    // Listen
    if (-1 == listen(fd, 0)) {
        perror("listen");
        close(fd);
        return -1;
    }

    // Accept
    int clifd = accept(fd, (struct sockaddr *)&clientInfo, &clientSize);

    if (-1 == clifd) {
        perror("accept");
        close(fd);
        return -1;
    }

    close(clifd);

    return 0;
}