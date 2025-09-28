#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include "common.h"
#include "file.h"
#include "parse.h"

/* Private function prototypes -----------------------------------------------*/
void print_usage(char *argv[]);

/**
  * @brief  The application entry point.
  * @param argc: [in] Number of command-line arguments
  * @param argv: [in] Array of pointers to the command-line argument strings
  * @retval int
  */
int main(int argc, char *argv[]) {
    int c;
    bool newFile = false;
    char *pFilepath = NULL;
    char *pAddString = NULL;
    int dbfd = -1;
    Parse_DbHeader_t *pDbhdr = NULL;
    Parse_Sensor_t *pSensors = NULL;

    while (-1 != (c = getopt(argc, argv, "nf:a:"))) {
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
            case 'a':{
                pAddString = optarg;
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
        print_usage(argv);

        return 0;
    }

    if (true == newFile) {
        dbfd = file_createDb(pFilepath);
        if (STATUS_ERROR == dbfd)
        {
            printf("Unable to create database file\r\n");
            return -1;
        }

        if (STATUS_ERROR == parse_createDbHeader(dbfd, &pDbhdr))
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

        if (STATUS_ERROR == parse_validateDbHeader(dbfd, &pDbhdr))
        {
            printf("Failed to validate database header\r\n");
            return -1;
        }
    }

    if (STATUS_SUCCESS != parse_readSensors(dbfd, pDbhdr, &pSensors))
    {
        printf("Failed to read sensors");
        return 0;
    }

    printf("New file: %d\r\n", newFile);
    printf("Filepath: %s\r\n", pFilepath);

    parse_outputFile(dbfd, pDbhdr, pSensors);

    return 0;
}

void print_usage(char *argv[]) {
    printf("\r\nUsage: %s -f -n <database file>\r\n", argv[0]);
    printf("\t -n - create new database file\r\n");
    printf("\t -f - (required) path to database file\r\n");

    return;
}