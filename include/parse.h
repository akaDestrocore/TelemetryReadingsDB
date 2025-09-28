#ifndef _PARSE_H
#define _PARSE_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "common.h"

struct DB_Header_t {
    unsigned short version;
    unsigned short count;
};

int parse_file_header(int fd, int *numSensorsOut);

#endif /* _PARSE_H */