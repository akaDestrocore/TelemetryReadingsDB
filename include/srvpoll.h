#ifndef _SRVPOLL_H
#define _SRVPOLL_H

#include <poll.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>
#include "parse.h"

#define     MAX_CLIENTS     256
#define     BUFF_SIZE       4096
#define     PORT            8080

typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_HELLO,
    STATE_MSG
} State_e;

typedef struct {
    int fd;
    State_e state;
    char buffer[BUFF_SIZE];
} ClientState_t;

// Polling routine for the server
void poll_loop(unsigned short port, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **ppSensors, int dbfd);

#endif /* _SRVPOLL_H */