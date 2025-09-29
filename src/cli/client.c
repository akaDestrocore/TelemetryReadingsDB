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

/**
  * @brief  Handle a client connection.
  * @param fd: Client file descriptor.
  */
void handleClient(int fd) {
    char buff[4096] = {0};

    read(fd, buff, sizeof(MsgHdr_t) + sizeof(int));

    MsgHdr_t *msgHdr = buff;
    msgHdr->type = ntohl(msgHdr->type);
    msgHdr->len = ntohs(msgHdr->len);

    int *data = &msgHdr[1];
    *data = ntohl(*data);

    if(MSG_HELLO_REQ != msgHdr->type) {
        printf("Protocol mismatch.\r\n");
    }

    if (1 != *data) {
        printf("Protocol version mismatch.\r\n");
    }

    printf("Server connected successfully.\r\n");
}