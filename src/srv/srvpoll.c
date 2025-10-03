#include <srvpoll.h>

// define in main.c to initialize clients
extern ClientState_t clientStates[MAX_CLIENTS];

/* Private function prototypes -----------------------------------------------*/
// Initialize clients
static void srvpoll_initClients(ClientState_t* states);
// Get the first free slot from the clients that the server is working with
static int srvpoll_findFreeSlot(ClientState_t* states);
// Find the slot number of the client that has data to be read 
static int srvpoll_findSlotByFd(ClientState_t* states, int fd);
// State machine
static void handle_client_fsm(Parse_DbHeader_t *dbhdr, Parse_Sensor_t **ppSensors, ClientState_t *client, int dbfd);
// reply to client's request
static void fsm_reply_hello(ClientState_t *client, DbProtocolHdr_t *hdr);
// reply error to client
static void fsm_reply_err(ClientState_t *client, DbProtocolHdr_t *hdr);
// Reply successfull add 
static void fsm_reply_add(ClientState_t *client, DbProtocolHdr_t *hdr);

/**
  * @brief  Initialize the client state array
  * @param states: pointer to the client state array
  */
static void srvpoll_initClients(ClientState_t* states) {
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
static int srvpoll_findFreeSlot(ClientState_t* states) {
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
static int srvpoll_findSlotByFd(ClientState_t* states, int fd) {
    int i = 0;
    for(; i < MAX_CLIENTS; ++i) {
        if (states[i].fd == fd) {
            return &states[i];
        }
    }
    return -1;
}

/**
  * @brief  Polling routine for the server
  * @param port: port number to listen on
  * @param dbhdr: pointer to the database header structure
  * @param sensors: pointer to the array of sensor structures
  * @param dbfd: file descriptor for the database file
  */
void poll_loop(unsigned short port, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **ppSensors, int dbfd) {
    int listen_fd, conn_fd, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS+1];
    int nfds = 1;
    int opt = 1;

    // Initialize client states
    srvpoll_initClients(&clientStates);

    // Create listening socket
    if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (0 != setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Setup server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Bind socket to port
    if (-1 == bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen on socket
    if (-1 == listen(listen_fd, MAX_CLIENTS)) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port: %d\r\n", port);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
        int ii = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (-1 == clientStates[i].fd) {
                fds[ii].fd = clientStates[i].fd;
                fds[ii].events = POLLIN;
                ii++;
            }
        }

        // Wait for event
        int n_events = poll(fds, nfds, -1);
        if ( -1 == n_events) {
            perror("poll");
            exit(EXIT_FAILURE);
        } 
        
        if (fds[0].revents & POLLIN) {
            if (-1 == (conn_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len))) {
                perror("accept");
                continue;
            }

            printf("New connection established from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            freeSlot = srvpoll_findFreeSlot(&clientStates);
            if (-1 == freeSlot) {
                // No more slots available, close the new connection.
                printf("Server is full. Closing new connection.\r\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_CONNECTED;
                nfds++;
                printf("Slot %d has fd %d\n", freeSlot, clientStates[freeSlot].fd);
            }

            n_events--;
        }

        // Check each client for read/write activity
        for (int i = 0; i <= nfds && n_events > 0; i++) {
            if (fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot = srvpoll_findSlotByFd(fds[i].fd, &clientStates);
                ssize_t bytes_read = read(fd, &clientStates[slot].buffer, sizeof(clientStates));
                if (bytes_read <= 0) {
                    // Connection error
                    close(fd);
                    if (-1 == slot) {
                        printf("File descriptor you're trying to close does not exist.\r\n");
                    } else {
                        clientStates[slot].fd = -1;
                        clientStates[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected.\r\n");
                        nfds--;
                    }
                } else {
                    printf("Received data from client: %s\r\n", clientStates[slot].buffer);
                }
            }
        }
    }
}

/** 
 * Helper functions
*/

static void handle_client_fsm(Parse_DbHeader_t *dbhdr, Parse_Sensor_t **ppSensors, ClientState_t *client, int dbfd) {
    // Casting buffer that was already read
    DbProtocolHdr_t *hdr = (DbProtocolHdr_t *)client->buffer;

    // Unpack
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohl(hdr->len);

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