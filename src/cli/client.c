#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "common.h"

#define BUFF_SIZE   4096

/* Private function prototypes -----------------------------------------------*/
static void handle_client(int fd);
static int send_req(int fd);
static int send_sensor(int fd, const char *addstr);
int list_sensors(int fd);


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
    
    int c;
    char *addarg = NULL;
    char *portarg = NULL;
    char *hostarg = NULL;
    uint16_t port = 0;
    bool list = false;


    while (-1 != (c = getopt(argc, argv, "a:p:h:l"))) {
        switch(c) {
            case 'a': {
                addarg = optarg;
                break;
            }
            case 'p': {
                portarg = optarg;
                port = atoi(portarg);
                break;
            }
            case 'h': {
                hostarg = optarg;
                break;
            }
            case 'l':{
                list = true;
                break;
            }
            case '?': {
                printf("Unknown option: %c\r\n", c);
                break;
            }
            default: {
                return -1;
            }
        }
    }

    if (0 == port) {
        printf("Bad port: %s\r\n", portarg);
        return -1;
    }

    if (NULL == hostarg) {
        printf("You need to specify host!\r\n");
        return -1;
    }

    struct sockaddr_in serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_port = htons(port);
    serverInfo.sin_addr.s_addr = inet_addr(hostarg);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd) {
        perror("socket");
        return 0;
    }

    if (-1 == connect(fd, (struct sockaddr *)&serverInfo, sizeof(serverInfo))) {
        perror("connect");
        close(fd);
        return 0;
    }

    if (STATUS_SUCCESS != send_req(fd)) {
        close(fd);
        return -1;
    }

    if (NULL != addarg) {
        send_sensor(fd, addarg);
    }

    if (true == list) {
        list_sensors(fd);
    }

    close(fd);

    return 0;
}

/**
  * @brief  Handle a client connection.
  * @param fd: Client file descriptor.
  */
void handle_client(int fd) {
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

/**
  * @brief  Request sensor data from the server database.
  * @param fd: File descriptor of the client.
  * @retval Status of the request.
  */
int send_req(int fd) {
    char buff[BUFF_SIZE] = {0};

    DbProtocolHdr_t *hdr = buff;
    hdr->type = MSG_HANDSHAKE_REQ;
    hdr->len = 1;

    DbProtocolVer_Req_t *req = (DbProtocolVer_Req_t *)&hdr[1];
    req->version = PROTOCOL_VER;

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);
    req->version = htons(req->version);

    // Write data
    write(fd, buff, sizeof(DbProtocolHdr_t) + sizeof(DbProtocolVer_Req_t));

    // Read response
    read(fd, buff, sizeof(buff));

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);
    
    // if any errors
    if ( MSG_ERROR == hdr->type) {
        printf("Protocol mismatch.\r\n");
        close(fd);
        return STATUS_ERROR;
    }

    printf("Server connected!\r\n");
    return STATUS_SUCCESS;

    return 0;
}


/**
  * @brief  Add a sensor to the database
  * @param fd: File descriptor of the client.
  * @param addstr: Sensor to be added.
  * @retval STATUS_SUCCESS or STATUS_ERROR
  */
int send_sensor(int fd, const char *addstr) {
    char buff[BUFF_SIZE] = {0};

    DbProtocolHdr_t *hdr = buff;
    hdr->type = MSG_SENSOR_ADD_REQ;
    hdr->len = 1;

    DbProtocol_SensorAddReq_t *sensor = (DbProtocol_SensorAddReq_t *)&hdr[1];
    strncpy(sensor->data, addstr, sizeof(sensor->data));

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);

    // Write message
    write(fd, buff, sizeof(DbProtocolHdr_t) + sizeof(DbProtocol_SensorAddReq_t));

    // Read response 
    read(fd, buff, sizeof(buff));

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (MSG_ERROR == hdr->type) {
        printf("Improper format for add sensor\r\n");
        close(fd);
        return STATUS_ERROR;
    }

    if (MSG_SENSOR_ADD_RESP  == hdr->type) {
        printf("Sensor added succesfully.\r\n");
    }

    return STATUS_SUCCESS;
}

/**
  * @brief  List sensors
  * @param fd: File descriptor of the client.
  * @retval STATUS_SUCCESS or STATUS_ERROR
  */
int list_sensors(int fd) {
    char buf[4096] = {0};
    DbProtocolHdr_t *hdr = (DbProtocolHdr_t *)buf;
    hdr->type = MSG_SENSOR_LIST_REQ;
    hdr->len = 0;

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);
    write(fd, buf, sizeof(DbProtocolHdr_t));
    printf("Sent sensor list request to server\n");

    read(fd, hdr, sizeof(DbProtocolHdr_t));
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);
    
    if (hdr->type == MSG_ERROR) {
        printf("Unable to list sensors.\n");
        close(fd);
        return STATUS_ERROR;
    } 

    if (hdr->type == MSG_SENSOR_LIST_RESP) {
        int count = hdr->len;
        printf("Received sensor list response from server. Count: %d\n", count);

        DbProtocol_SensorListResp_t *sensor = (DbProtocol_SensorListResp_t *)&hdr[1];
        unsigned int temp;
        int i = 0;
        
        for (; i < count; i++) {
            read(fd, sensor, sizeof(DbProtocol_SensorListResp_t));
            
            sensor->timestamp = ntohl(sensor->timestamp);
            
            temp = ntohl(*(unsigned int*)&sensor->readingValue);
            sensor->readingValue = *(float*)&temp;
            
            temp = ntohl(*(unsigned int*)&sensor->minThreshold);
            sensor->minThreshold = *(float*)&temp;
            
            temp = ntohl(*(unsigned int*)&sensor->maxThreshold);
            sensor->maxThreshold = *(float*)&temp;
            
            printf("\nSensor %d:\n", i);
            printf("  ID: %s\n", sensor->sensorId);
            printf("  Type: %s\n", sensor->sensorType);
            printf("  I2C Address: 0x%02X\n", sensor->i2cAddr);
            printf("  Timestamp: %s", ctime((time_t*)&sensor->timestamp));
            printf("  Reading: %.2f\n", sensor->readingValue);
            printf("  Flags: 0x%02X", sensor->flags);
            
            if (sensor->flags & 0x01) printf(" ACTIVE");
            if (sensor->flags & 0x02) printf(" ERROR");
            if (sensor->flags & 0x04) printf(" CALIBRATED");
            
            printf("\n");
            printf("  Location: %s\n", sensor->location);
            printf("  Thresholds: Min=%.2f, Max=%.2f\n", 
                   sensor->minThreshold, sensor->maxThreshold);
        }
    }

    return STATUS_SUCCESS;
}