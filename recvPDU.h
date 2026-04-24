#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PAYLOAD_SIZE_BYTES 2

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize, uint8_t *flag);
