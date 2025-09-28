#ifndef _FILE_H
#define _FILE_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

// create new database
int file_createDb(char *pFilename);
// open existing database
int file_openDb(char *pFilename);

#endif /* _FILE_H */