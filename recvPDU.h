#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define LENGTH_OF_PDU_SIZE 2
#define FLAG_SIZE 1
#define HEADER_SIZE (LENGTH_OF_PDU_SIZE + FLAG_SIZE)

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize, uint8_t *flag);
