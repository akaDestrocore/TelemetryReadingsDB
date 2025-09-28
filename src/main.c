#include <stdio.h>

#include "file.h"
#include "parse.h"

int main(int argc, char *argv[]) {
    int fd, numSensors = 0;

    if (2 != argc) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 0;
    }

    fd = open_file_rw(argv[1]);
    if (-1 == fd) {
        return -1;
    }

    if (0 != parse_file_header(fd, &numSensors)) {
        return -1;
    }

    printf("Number of sensors stored: %d\r\n", numSensors);

    return 0;
}