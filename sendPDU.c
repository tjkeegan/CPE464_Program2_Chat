#include "sendPDU.h"

int sendPDU(int socketNumber, uint8_t *dataBuffer, int lengthOfData, uint8_t flag){
    // create PDU [2 bytes payload length; n bytes payload]
    lengthOfData += PAYLOAD_SIZE_BYTES;
    uint8_t pdu[lengthOfData];
    uint16_t lengthOfData_net = htons(lengthOfData);
    memcpy(pdu, &lengthOfData_net, PAYLOAD_SIZE_BYTES);
    memcpy(pdu + FLAG_SIZE, &flag, FLAG_SIZE);
    memcpy(pdu + PAYLOAD_SIZE_BYTES, dataBuffer, lengthOfData - PAYLOAD_SIZE_BYTES - FLAG_SIZE);

    // send PDU and error check (flags set to 0 makes send() equivalent to write())
    ssize_t bytes_sent = send(socketNumber, pdu, lengthOfData, 0);
    if (bytes_sent < 0) {
        perror("error sending pdu");
        exit(1);
    }

    // return size of payload (total PDU bytes - 2 byte size)
    return bytes_sent - PAYLOAD_SIZE_BYTES;
}