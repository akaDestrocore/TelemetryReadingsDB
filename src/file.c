#include "file.h"

/**
 * @brief  Opens a file for reading and writing.
 * @param  filename: [in] Pointer to a null-terminated string containing the file name.
 * @return 0 on success, -1 otherwise.
 * @note   This function is a wrapper around the standard C function `open`.
 */
int open_file_rw(char *filename) {
    int fd = open(filename, O_RDWR);

    if (-1 == fd) {
        perror("open");
        return fd;
    }

    return fd;
}