#include "parse.h"

/**
 * @brief  Parses the header of a database file.
 * @param fd: [in] File descriptor
 * @param  numSensorsOut: [in] Number of sensors stored in the database
 * @return 0 on success, -1 otherwise.
 * @note   This function is a wrapper around the standard C function `read`
 */
int parse_file_header(int fd, int *numSensorsOut) {
    if (-1 == fd) {
        printf("Bad file descriptor provided\r\n");
        return STATUS_ERROR;
    }

    struct DB_Header_t header = {0};
    if (read(fd, &header, sizeof(struct DB_Header_t)) != sizeof(header)) {
        printf("Error reading from file\r\n");
        return STATUS_ERROR;
    }

    *numSensorsOut = header.count;
    return STATUS_SUCCESS;
}