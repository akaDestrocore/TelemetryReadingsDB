#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"

/* Private function prototypes -----------------------------------------------*/
void handleClient(int fd);
int send_req(int fd);


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
    serverInfo.sin_port = htons(8080);

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

    if (STATUS_SUCCESS != send_req(fd)) {
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

    read(fd, buff, sizeof(DbProtocolHdr_t) + sizeof(int));

    DbProtocolHdr_t *msgHdr = buff;
    msgHdr->type = ntohl(msgHdr->type);
    msgHdr->len = ntohs(msgHdr->len);

    int *data = &msgHdr[1];
    *data = ntohl(*data);

    if(MSG_HANDSHAKE_REQ != msgHdr->type) {
        printf("Protocol mismatch.\r\n");
    }

    if (1 != *data) {
        printf("Protocol version mismatch.\r\n");
    }

    printf("Server connected successfully.\r\n");
}

int send_req(int fd) {
    char buff[BUFF_SIZE] = {0};

    DbProtocolHdr_t *hdr = buff;
    hdr->type = MSG_HANDSHAKE_REQ;
    hdr->len = 1;

    // Pack it for the network
    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);

    write(fd, buff, sizeof(DbProtocolHdr_t));

    printf("Server connected successfully.\r\n");

    return 0;
}