#include <srvpoll.h>

// define in main.c to initialize clients
extern ClientState_t clientStates[MAX_CLIENTS];

/**
  * @brief  Initialize the client state array
  * @param states: pointer to the client state array
  */
void srvpoll_initClients(ClientState_t* states) {
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
int srvpoll_findFreeSlot(ClientState_t* states) {
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
int srvpoll_findSlotByFd(ClientState_t* states, int fd) {
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
void poll_loop(unsigned short port, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **sensors, int dbfd) {
    int listen_fd, conn_fd, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;
    int opt = 1;

    // Initialize client states
    srvpoll_initClients(&clientStates);

    // Create listening socket
    if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (0 != setsockopt(listen_fd, SOL_PACKET, SO_REUSEADDR, &opt, sizeof(opt))) {
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

    printf("Listening on port: %d", port);

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