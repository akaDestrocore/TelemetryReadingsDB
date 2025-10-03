#include "srvpoll.h"

// define in main.c to initialize clients
extern ClientState_t clientStates[MAX_CLIENTS];

/* Private variables ---------------------------------------------------------*/
static volatile bool keep_running = true;

/* Private function prototypes -----------------------------------------------*/
// Initialize clients
static void init_clients(ClientState_t* states);
// Get the first free slot from the clients that the server is working with
static int find_free_slot(ClientState_t* states);
// Find the slot number of the client that has data to be read 
static int find_slot_by_fd(ClientState_t* states, int fd);
// State machine
static void handle_client_fsm(Parse_DbHeader_t *dbhdr, Parse_Sensor_t **ppSensors, ClientState_t *client, int dbfd);
// reply to client's request
static void fsm_reply_hello(ClientState_t *client, DbProtocolHdr_t *hdr);
// reply error to client
static void fsm_reply_err(ClientState_t *client, DbProtocolHdr_t *hdr);
// Reply successfull add 
static void fsm_reply_add(ClientState_t *client, DbProtocolHdr_t *hdr);
// List all sensors in database
static void fsm_reply_list(ClientState_t *client, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **sensors);
// Handle client's request
static void handle_signal(int sig);
// listen for incoming connections
static int setup_server_socket(unsigned short port);

/**
  * @brief  Initialize the client state array
  * @param states: pointer to the client state array
  */
static void init_clients(ClientState_t* states) {
    int i = 0;
    for(; i < MAX_CLIENTS; ++i) {
        states[i].fd = -1;
        states[i].state = STATE_NEW;
        memset(states[i].buffer, '\0', BUFF_SIZE);
    }

    return;
}

/**
  * @brief  Find a free slot in the client state array
  * @param states: pointer to the client state array
  * @retval the index of the free slot
  */
static int find_free_slot(ClientState_t* states) {
    int i = 0;
    for(; i < MAX_CLIENTS; ++i) {
        if (-1 == states[i].fd) {
            return i;
        }
    }
    return -1;
}

/**
  * @brief  Find a slot in the client state array by fd
  * @param states: pointer to the client state array
  * @param fd: file descriptor
  * @retval slot index
  */
static int find_slot_by_fd(ClientState_t *states, int fd) {
    int i = 0;
    for (; i < MAX_CLIENTS; i++) {
        if (states[i].fd == fd) {
            return i;
        }
    }

    return -1;
}

/**
  * @brief  Polling routine for the server
  * @param port: port number to listen on
  * @param dbhdr: pointer to the database header structure
  * @param ppSensors: pointer to the array of sensors
  * @param dbfd: file descriptor for the database file
  */
void poll_loop(unsigned short port, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **sensors, int dbfd) {
    int conn_fd, freeSlot;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int i, listen_fd;
    int n_events;
    int nfds;
    struct pollfd fds[MAX_CLIENTS + 1];
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    init_clients(clientStates);
    
    listen_fd = setup_server_socket(port);
    printf("  Listening on: 0.0.0.0:%d\r\n", port);

    
    while (true == keep_running) {
        int i, poll_idx = 1;
        memset(fds, 0, sizeof(struct pollfd) * (MAX_CLIENTS + 1));
        
        fds[0].fd = listen_fd;
        fds[0].events = POLLIN;
        
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clientStates[i].fd != -1) {
                fds[poll_idx].fd = clientStates[i].fd;
                fds[poll_idx].events = POLLIN;
                poll_idx++;
            }
        }
        
        nfds = poll_idx;
        
        n_events = poll(fds, nfds, 30000);
        
        if (n_events < 0) {
            perror("poll");
            break;
        }
        
        if (n_events == 0) {
            printf("Poll timeout - no activity\r\n");
            continue;
        }

        if (fds[0].revents & POLLIN) {
            if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
                perror("accept");
                continue;
            }
        
            printf("New connection from %s:%d\r\n", 
                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
            freeSlot = find_free_slot(clientStates);
            if (freeSlot == -1) {
                printf("Server full: closing new connection\r\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_HELLO;
                printf("Client connected in slot %d with fd %d\r\n", freeSlot, conn_fd);
                nfds++;
            }
            n_events--;
        }
        
        for (i = 1; i <= nfds && n_events > 0; i++) {
            if (fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot = find_slot_by_fd(clientStates, fd);
                ssize_t bytes_read = read(fd, &clientStates[slot].buffer, sizeof(clientStates[slot].buffer));
                if (bytes_read <= 0) {
                    close(fd);

                    if (-1 != slot) {
                        clientStates[slot].fd = -1;
                        clientStates[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected\n");
                        nfds--;
                    }
                } else {
                   handle_client_fsm(dbhdr, sensors, &clientStates[slot], dbfd);
                }
            }
        }

        if (true != keep_running) {
            printf("Shutting down server...\n");
            break;
        }
    }
    
    printf("Closing server socket...\n");
    close(listen_fd);
    return;
}

/** 
 * Helper functions
*/

static void handle_client_fsm(Parse_DbHeader_t *dbhdr, Parse_Sensor_t **ppSensors, ClientState_t *client, int dbfd) {
    // Casting buffer that was already read
    DbProtocolHdr_t *hdr = (DbProtocolHdr_t *)client->buffer;

    // Unpack
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (STATE_HELLO == client->state) {
        // Hello message received, check for the length of the message.
        if (MSG_HANDSHAKE_REQ != hdr->type || 1 != hdr->len) {
            printf("Didn't receive MSG_HELLO from client.\r\n");
            fsm_reply_err(client, hdr);
            return;
        }

        DbProtocolVer_Req_t* hello_protocol = (DbProtocolVer_Req_t*)&hdr[1];
        hello_protocol->version = ntohs(hello_protocol->version);
        if (PROTOCOL_VER != hello_protocol->version) {
            printf("Protocol mismatch.\r\n");
            fsm_reply_err(client, hdr);
            return;
        }

        fsm_reply_hello(client, hdr);
        client->state = STATE_MSG;
        printf("Client promoted to STATE_MSG\r\n");
    }

    if (STATE_MSG == client->state) {
        if (MSG_SENSOR_ADD_REQ == hdr->type) {
            DbProtocol_SensorAddReq_t *sensor = (DbProtocolVer_Req_t *)&hdr[1];

            printf("Adding sensor: %s\r\n", sensor->data);
            if (STATUS_SUCCESS != parse_addSensor(dbhdr, ppSensors, sensor->data)) {
                fsm_reply_err(client, hdr);
                return;
            } else {
                fsm_reply_add(client, hdr);
                parse_outputFile(dbfd, dbhdr, *ppSensors);
            }
        }

        if (MSG_SENSOR_LIST_REQ == hdr->type) {
            printf("Listing all sensors\r\n");
            fsm_reply_list(client, dbhdr, ppSensors);
        }
    }

}

static void fsm_reply_hello(ClientState_t *client, DbProtocolHdr_t *hdr) {
    hdr->type = htonl(MSG_HANDSHAKE_RESP);
    hdr->len = htons(1);
    DbProtocolVer_Resp_t* hello_protocol = (DbProtocolVer_Resp_t*)&hdr[1];
    hello_protocol->version = htons(PROTOCOL_VER);
    write(client->fd, hdr, sizeof(DbProtocolHdr_t) + sizeof(DbProtocolVer_Resp_t));
}

static void fsm_reply_err(ClientState_t *client, DbProtocolHdr_t *hdr) {
    hdr->type = htonl(MSG_ERROR);
    hdr->len = htons(0);
    write(client->fd, hdr, sizeof(DbProtocolHdr_t));
}

static void fsm_reply_add(ClientState_t *client, DbProtocolHdr_t *hdr) {
    hdr->type = htonl(MSG_SENSOR_ADD_RESP);
    hdr->len = htons(0);
    write(client->fd, hdr, sizeof(DbProtocolHdr_t));
}

static void fsm_reply_list(ClientState_t *client, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **sensors) {
    DbProtocolHdr_t *hdr = (DbProtocolHdr_t*)client->buffer;
    DbProtocol_SensorListResp_t *resp = (DbProtocol_SensorListResp_t *)&hdr[1];
    unsigned int temp;
    int i = 0;
    
    hdr->type = htonl(MSG_SENSOR_LIST_RESP);
    hdr->len = htons(dbhdr->count);
    write(client->fd, hdr, sizeof(DbProtocolHdr_t));

    for (; i < dbhdr->count; i++) {
        strncpy(resp->sensorId, (*sensors)[i].sensorId, sizeof(resp->sensorId));
        strncpy(resp->sensorType, (*sensors)[i].sensorType, sizeof(resp->sensorType));
        resp->i2cAddr = (*sensors)[i].i2cAddr;
        resp->timestamp = htonl((*sensors)[i].timestamp);
        
        temp = htonl(*(unsigned int*)&(*sensors)[i].readingValue);
        resp->readingValue = *(float*)&temp;
        
        resp->flags = (*sensors)[i].flags;
        strncpy(resp->location, (*sensors)[i].location, sizeof(resp->location));
        
        temp = htonl(*(unsigned int*)&(*sensors)[i].minThreshold);
        resp->minThreshold = *(float*)&temp;
        
        temp = htonl(*(unsigned int*)&(*sensors)[i].maxThreshold);
        resp->maxThreshold = *(float*)&temp;
        
        write(client->fd, resp, sizeof(DbProtocol_SensorListResp_t));
    }
}


static void handle_signal(int sig) {
    printf("Received signal %d, shutting down...\r\n", sig);
    keep_running = false;
}

static int setup_server_socket(unsigned short port) {
    int listen_fd;
    struct sockaddr_in server_addr;
    int opt = 1;

    // Create new socket for listening
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Prepare server addr
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    /* Bind socket to port */
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections (max 15)
    if (listen(listen_fd, 15) < 0) {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    
    return listen_fd;
}
