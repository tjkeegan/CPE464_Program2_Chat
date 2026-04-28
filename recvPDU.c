#include "recvPDU.h"

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize, uint8_t *flag) {

    uint16_t payload_len = 0;
    uint8_t pduHeader[HEADER_SIZE];

    // RECEIVE PDU HEADER
    ssize_t bytes_received = recv(clientSocket, &pduHeader, sizeof(pduHeader), MSG_WAITALL);
    if (bytes_received < 0) {
        perror("error receiving payload length");
        exit(1);
    }
    if (bytes_received == 0) {
        *flag = 0;
        return 0;
    }

    // PARSE PDU HEADER
    memcpy(flag, pduHeader + LENGTH_OF_PDU_SIZE, FLAG_SIZE);
    memcpy(&payload_len, pduHeader, LENGTH_OF_PDU_SIZE);
    payload_len = ntohs(payload_len) - LENGTH_OF_PDU_SIZE - FLAG_SIZE;

    // check if bufferSize is large enough for payload
    if (payload_len > bufferSize) {
        printf("%d", payload_len);
        perror("payload bigger than buffer");
        exit(1);
    }

    if (payload_len == 0) {
        return 0;
    }

    // RECEIVE PAYLOAD
    bytes_received = recv(clientSocket, dataBuffer, payload_len, MSG_WAITALL);
    if (bytes_received < 0) {
        perror("error receiving payload length");
        exit(1);
    }
    if (bytes_received == 0) {
        printf("connection closed");
        *flag = 0;
        return 0;
    }

    // RETURN SIZE OF PAYLOAD
    return bytes_received;
}
