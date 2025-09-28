#include "file.h"

/**
 * @brief  Creates a new database file with the given filename.
 * @param  pFilename: [in] Filename of the new database file
 * @return 0 on success, -1 otherwise.
 * @note  This function tries to open a file as read only mode. If the file
 *          already exists, it returns an error.  Otherwise, creates a new 
 *          database file
 */
int file_createDb(char *pFilename)
{
    int fd = -1;

    fd = open(pFilename, O_RDONLY);
    if (-1 != fd)
    {
        close(fd);
        printf("File already exists\r\n");
        return STATUS_ERROR;
    }

    fd = open(pFilename, O_RDWR | O_CREAT, 0644);
    if (-1 == fd)
    {
        perror("open");
        return STATUS_ERROR;
    }

    return fd;
}

/**
 * @brief  Opens an existing database file.
 * @param  pFilename: [in] Filename of the database file
 * @return 0 on success, -1 otherwise.
 * @note  This function tries to open a file as read/write mode. If the file
 *          does not exist, it returns an error.
 */
int file_openDb(char *pFilename)
{
    int fd = -1;

    fd = open(pFilename, O_RDWR, 0644);
    if (-1 == fd)
    {
        perror("open");
        return STATUS_ERROR;
    }

    return fd;
}