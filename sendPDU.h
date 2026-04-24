#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PAYLOAD_SIZE_BYTES 2
#define FLAG_SIZE 1

int sendPDU(int socketNumber, uint8_t * dataBuffer, int lengthOfData, uint8_t flag);