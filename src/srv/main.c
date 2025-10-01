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
#include "srvpoll.h"


/* Private define ------------------------------------------------------------*/
#define MAX_CLIENTS 256

/* Global variables ----------------------------------------------------------*/
ClientState_t clientStates[MAX_CLIENTS];

/* Private function prototypes -----------------------------------------------*/
void printUsage(char *argv[]);
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