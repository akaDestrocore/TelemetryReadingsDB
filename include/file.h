#ifndef _FILE_H
#define _FILE_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Open file as read-write
int open_file_rw(char *filename);

#endif /* _FILE_H */