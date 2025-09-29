#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"

/* Private function prototypes -----------------------------------------------*/
void handleClient(int fd);


/**
  * @brief  The application entry point.
  * @param argc: [in] Number of command-line arguments
  * @param argv: [in] Array of pointers to the command-line argument strings
  * @retval int
  */
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

    while (1) {
        // Accept
        int clifd = accept(fd, (struct sockaddr *)&clientInfo, &clientSize);

        if (-1 == clifd) {
            perror("accept");
            close(fd);
            return -1;
        }

        handleClient(clifd);

        close(clifd);
    }

    return 0;
}

/**
  * @brief  Handle a client connection.
  * @param fd: Client file descriptor.
  */
void handleClient(int fd) {
    char buff[4096] = {0};

    MsgHdr_t *msgHdr = buff;
    msgHdr->type = htonl(MSG_HELLO_REQ);
    msgHdr->len = sizeof(int);
    int actualLen = msgHdr->len;
    msgHdr->len = htons(msgHdr->len);

    int *data = (int*)msgHdr[1];
    *data = htonl(1);

    write(fd, msgHdr, sizeof(MsgHdr_t) + actualLen);
}