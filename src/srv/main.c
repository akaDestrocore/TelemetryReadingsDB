#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "common.h"
#include "file.h"
#include "parse.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <poll.h>

/* Private define ------------------------------------------------------------*/
#define MAX_CLIENTS 256

/* Global variables ----------------------------------------------------------*/
ClientState_t clientStates[MAX_CLIENTS];

/* Private function prototypes -----------------------------------------------*/
void printUsage(char *argv[]);
void init_clients(void);
int find_free_slot(void);
int find_slot_by_fd(int fd);
void poll_loop(unsigned short port, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **sensors, int dbfd);

/**
  * @brief  The application entry point.
  * @param argc: [in] Number of command-line arguments
  * @param argv: [in] Array of pointers to the command-line argument strings
  * @retval int
  */
int main(int argc, char *argv[]) {
    char *pFilepath = NULL;
    char *pPortArg = NULL;
    unsigned short port = 0;
    bool newFile = false;
    bool list = false;
    int c;

    int dbfd = -1;
    struct Parse_DbHeader_t *pDbHdr = NULL;
    struct Parse_Sensor_t *pSensors = NULL;

    while (-1 != (c = getopt(argc, argv, "nf:p:"))) {
        switch (c)
        {
            case 'n':{
                newFile = true;
                break;
            }
            case 'f':{
                pFilepath = optarg;
                break;
            }
            case 'p':{
                pPortArg = optarg;
                port = atoi(pPortArg);
                if (0 == pPortArg) {
                    printf("Incorrect port value!\r\n");
                }
                break;
            }
            case '?':{
                printf("Unknown option -%c\r\n", c);
                break;
            }
            default:{
                return -1;
            }
        }
    }

    if (NULL == pFilepath) {
        printf("File path is a required argument!\r\n");
        printUsage(argv);

        return 0;
    }

    if (true == newFile) {
        dbfd = file_createDb(pFilepath);
        if (STATUS_ERROR == dbfd)
        {
            printf("Unable to create database file\r\n");
            return -1;
        }

        if (STATUS_ERROR == parse_createDbHeader(dbfd, &pDbHdr))
		{
			printf("Failed to create database header\n");
			return -1;
		}
    } else {
        dbfd = file_openDb(pFilepath);
        if (STATUS_ERROR == dbfd)
        {
            printf("Unable to open database file\r\n");
            return -1;
        }

        if (STATUS_ERROR == parse_validateDbHeader(dbfd, &pDbHdr))
        {
            printf("Failed to validate database header\r\n");
            return -1;
        }
    }

    if (STATUS_SUCCESS != parse_readSensors(dbfd, pDbHdr, &pSensors))
    {
        printf("Failed to read sensors");
        return 0;
    }

    poll_loop(port, pDbHdr, pSensors, dbfd);

    parse_outputFile(dbfd, pDbHdr, &pSensors);

    return 0;
}

/**
  * @brief  Print usage information for the application
  * @param argv: [in] Array of pointers to the command-line argument strings
  */
void printUsage(char *argv[]) {
    
    printf("\r\nUsage: %s -f -n <database file>\r\n", argv[0]);
    printf("\t -n - create new database file\r\n");
    printf("\t -f - (required) path to database file\r\n");

    return;
}

void init_clients(void) {
    
    int i = 0;

    for (; i < MAX_CLIENTS; i++) {
        clientStates[i].fd = -1;
        clientStates[i].state = STATE_NEW;
        memset(clientStates[i].buffer, '\0', BUFF_SIZE);
    }
}

int find_free_slot(void) {
    
    int i = 0;
    
    for (; i < MAX_CLIENTS; i++) {
        if (-1 == clientStates[i].fd) {
            return i;
        }
    }
    return -1;
}

int find_slot_by_fd(int fd) {
    
    int i = 0;
    
    for (; i < MAX_CLIENTS; i++) {
        if (clientStates[i].fd == fd) {
            return i;
        }
    }
    return -1;
}

void poll_loop(unsigned short port, Parse_DbHeader_t *dbhdr, Parse_Sensor_t **sensors, int dbfd) {
    int listen_fd, conn_fd, freeSlot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int opt = 1;
    
    // Initialize client states
    init_clients();

    if (-1 == (listen_fd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror ("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // Setup server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
        int ii = 1;
        int i = 0;
        for (; i < MAX_CLIENTS; i++) {
            if (-1 != clientStates[i].fd) {
                // offset by 1 for listen_fd
                fds[ii].fd = clientStates[i].fd;
                fds[ii].events = POLLIN;
                i++;
            }
        }

        // Wait for an event on one of the sockets
        // -1 for no timeouts
        int n_events = poll(fds, nfds, -1);
        if (- 1 == n_events) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // Check for new connections
        if (fds[0].revents & POLLIN) {
            if (-1 == (conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len))) {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            printf("New connection from %s:%d\r\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            freeSlot = find_free_slot();
            if (-1 == freeSlot) {
                printf("Server full: closing new connections.\r\n");
                close(conn_fd);
            } else {
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_NEW;
                nfds++;
                printf("Slot %d has fd %d\r\n", freeSlot, clientStates[freeSlot]);
            }

            n_events--;
        }

        // Check each client for read/write activity
        for (int i = 0; i <= nfds && n_events > 0; i++) {
            if (fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot = find_slot_by_fd(fd);
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