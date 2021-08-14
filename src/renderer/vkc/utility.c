#include "utility.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>

char* read_file_to_buffer(const char filename[], uint64_t* size) {
    FILE* fd = fopen(filename, "rb");
    if(!fd) {
        log_error("read_file_to_buffer: File %s not found\n", filename);
        return 0;
    }

    fseek(fd, 0, SEEK_END);
    *size = ftell(fd);
    char* buffer = malloc(*size+1);
    if (!buffer) {
        log_error("read_file_to_buffer: malloc(...) failed!\n", filename);
        return 0;
    }

    fseek(fd, 0, SEEK_SET);
    fread(buffer, *size, 1, fd);
    fclose(fd);

    buffer[*size] = 0;

    return buffer;
}
