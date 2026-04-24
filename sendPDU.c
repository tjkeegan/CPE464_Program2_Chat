#include "sendPDU.h"

int sendPDU(int socketNumber, uint8_t *dataBuffer, int lengthOfData, uint8_t flag){
    // create PDU [2 bytes payload length; n bytes payload]
    int lengthOfPDU = lengthOfData + PAYLOAD_LENGTH_SIZE + FLAG_SIZE;
    uint8_t pdu[lengthOfPDU];
    uint16_t lengthOfPDU_net = htons(lengthOfPDU);
    memcpy(pdu, &lengthOfPDU_net, PAYLOAD_LENGTH_SIZE);
    memcpy(pdu + PAYLOAD_LENGTH_SIZE, &flag, FLAG_SIZE);
    memcpy(pdu + PAYLOAD_LENGTH_SIZE + FLAG_SIZE, dataBuffer, lengthOfData);

    // send PDU and error check (flags set to 0 makes send() equivalent to write())
    ssize_t bytes_sent = send(socketNumber, pdu, lengthOfPDU, 0);
    if (bytes_sent < 0) {
        perror("error sending pdu");
        exit(1);
    }

    // return size of payload (total PDU bytes - 2 byte size)
    return bytes_sent - PAYLOAD_LENGTH_SIZE - FLAG_SIZE;
}