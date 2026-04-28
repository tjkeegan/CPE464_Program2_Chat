#include "handle.h"

struct handleEntry *handleTable;
int tableIndex = 0;
size_t tableSize = 0;
size_t usedSize = 0;

void handleSetUp() {
    handleTable = malloc(ALLOC_SIZE);
    if (handleTable == NULL) {
        perror("Failed to allocate Handle Table");
        exit(1);
    }
    tableSize = ALLOC_SIZE;
}

void handleTeardown() {
    for (int i = 0; i < tableIndex; i++) {
        free(handleTable[i].handle);
    }
    free(handleTable);
}

void addHandle(int socket, char *handle) {  
    char *handleDup = malloc(strlen(handle) + 1); // +1 for '\0'
    if (handleDup == NULL) {
        perror("Failed to allocate handleDup during addHandle()");
        exit(1);
    }
    memcpy(handleDup, handle, strlen(handle) + 1); // +1 for '\0'
    struct handleEntry newEntry = {socket, handleDup};
    size_t newSize = usedSize + sizeof(newEntry);
    if (newSize > tableSize) {
        handleTable = realloc(handleTable, newSize);
        if (handleTable == NULL) {
            perror("Failed to reallocate Handle Table during addHandle()");
            exit(1);
        }
    }
    handleTable[tableIndex] = newEntry;
    tableIndex++;
    usedSize = newSize;
}

int removeHandle(char *handle) {
    for (int i = 0; i < tableIndex; i++) {
        if (!memcmp(handleTable[i].handle, (char *) handle, strlen(handle))) {
            size_t entrySize = sizeof(handleTable[i]);
            free(handleTable[i].handle);
            for (int j = i; j < tableIndex - 1; j++) {
                handleTable[j] = handleTable[j + 1];
            }
            // handleTable = realloc(handleTable, usedSize - entrySize);
            // if (handleTable == NULL) {
            //     perror("Failed to reallocate Handle Table during removeHandle()");
            //     exit(1);
            // }
            usedSize -= entrySize;
            tableIndex--;
            return 0;
        }
    }
    return -1;
}

void printHandleTable() {
    for (int i = 0; i < tableIndex; i++) {
        printf("Handle %d: Name: %s\tSocket: %d\n", i, handleTable[i].handle, handleTable[i].socket);
    }
}

int findSocket(uint8_t *handle, uint8_t handleLen) {
    for (int i = 0; i < tableIndex; i++) {
        if (!memcmp(handleTable[i].handle, (char *) handle, handleLen)) {
            return handleTable[i].socket;
        }
    }
    return -1;
}

int findSocketByIndex(int index) {
    if ((index < tableIndex) && (index >= 0)) {
        return handleTable[index].socket;
    }
    return -1;
}

char *findHandle(int socket) {
    for (int i = 0; i < tableIndex; i++) {
        if (handleTable[i].socket == socket) {
            return handleTable[i].handle;
        }
    }
    return NULL;
}

char *findHandleByIndex(int index) {
    if ((index < tableIndex) && (index >= 0)) {
        return handleTable[index].handle;
    }
    return NULL;
}

int getNumHandles() {
    return tableIndex;
}