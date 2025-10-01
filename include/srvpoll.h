#ifndef _SRVPOLL_H
#define _SRVPOLL_H

#include <poll.h>
#include <arpa/inet.h>
#include <sys/time.h>
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

// Initialize clients
void srvpoll_initClients(ClientState_t* states);
// Get the first free slot from the clients that the server is working with
int srvpoll_findFreeSlot(ClientState_t* states);
// Find the slot number of the client that has data to be read 
int srvpoll_findSlotByFd(ClientState_t* states, int fd);
// Polling routine for the server
void poll_loop(unsigned short port, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **sensors, int dbfd);

#endif /* _SRVPOLL_H */