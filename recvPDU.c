#include "recvPDU.h"

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize, uint8_t *flag) {
    // receive payload size [2 bytes], error check, and check if socket closed
    uint16_t payload_len;
    // use &payload_len because recv asks for a pointer, use MSG_WAITALL to block until all bytes are received
    ssize_t bytes_received = recv(clientSocket, &payload_len, sizeof(payload_len), MSG_WAITALL);
    if (bytes_received < 0) {
        perror("error receiving payload length");
        exit(1);
    }
    if (bytes_received == 0) {
        perror("connection closed");
        return 0;
    }

    // adjust payload_len for network byte order and subtract 2 bytes for payload size
    payload_len = ntohs(payload_len) - PAYLOAD_SIZE_BYTES;

    // check if bufferSize is large enough for payload
    if (payload_len > bufferSize) {
        perror("payload bigger than buffer");
        exit(1);
    }

    // receive payload into buffer, error check, and check if socket closed
    bytes_received = recv(clientSocket, dataBuffer, payload_len, MSG_WAITALL);
    if (bytes_received < 0) {
        perror("error receiving payload length");
        exit(1);
    }
    if (bytes_received == 0) {
        perror("connection closed");
        return 0;
    }

    // return size of payload
    *flag = dataBuffer[0];
    return bytes_received;
}