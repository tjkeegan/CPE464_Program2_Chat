#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ALLOC_SIZE 10000 // 10KBytes

// handle entry: [socket uint32_t | handle size uint8_t | handle char*]

struct handleEntry {
    int socket;
    char *handle;
};

void handleSetUp();
void addHandle(int socket, char *handle);
int removeHandle(char *handle);
void printHandleTable();
int findSocket(uint8_t *handle, uint8_t handleLen);
char *findHandle(int socket);
char *findHandleByIndex(int index);
int getNumHandles();