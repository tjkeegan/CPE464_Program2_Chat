#include "handle.h"

struct handleEntry *handleTable;
int tableIndex = 0;
size_t tableSize = 0;
size_t usedSize = 0;

void handleInit() {
    handleTable = malloc(ALLOC_SIZE);
    if (handleTable == NULL) {
        perror("Failed to allocate Handle Table");
        exit(1);
    }
    tableSize = ALLOC_SIZE;
}

void addHandle(int socket, char *handle) {
    struct handleEntry newEntry = {socket, handle};
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
        if (handleTable[i].handle == handle) {
            size_t entrySize = sizeof(handleTable[i]);
            for (int j = i; j < tableIndex - 1; j++) {
                handleTable[j] = handleTable[j + 1];
            }
            handleTable = realloc(handleTable, usedSize - entrySize);
            if (handleTable == NULL) {
                perror("Failed to reallocate Handle Table during removeHandle()");
                exit(1);
            }
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