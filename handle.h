#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#define ALLOC_SIZE 10000 // 10KBytes

// handle entry: [socket uint32_t | handle size uint8_t | handle char*]

struct handleEntry {
    int socket;
    char *handle;
};

void handleInit();
void addHandle(int socket, char *handle);
int removeHandle(char *handle);
// int findSocket();
// char *findHandle();