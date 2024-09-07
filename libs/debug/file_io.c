#include "file_io.h"

// Function to write a buffer to a file
int file_io_write_buffer_to_file(const char *file_path, const char *buffer, size_t buffer_size) {
    FILE *file = fopen(file_path, "wb");
    if (!file) {
        perror("Error opening file for writing");
        return -1;
    }

    size_t written_size = fwrite(buffer, 1, buffer_size, file);
    fclose(file);

    if (written_size != buffer_size) {
        perror("Error writing to file");
        return -1;
    }

    return 0; // Success
}

// Function to read a buffer from a file
char *file_io_read_buffer_from_file(const char *file_path, size_t *buffer_size) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        perror("Error opening file for reading");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *buffer_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(*buffer_size);
    if (!buffer) {
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, *buffer_size, file);
    fclose(file);

    if (read_size != *buffer_size) {
        perror("Error reading from file");
        free(buffer);
        return NULL;
    }

    return buffer; // Success
}
