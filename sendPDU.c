#include "sendPDU.h"

int sendPDU(int socketNumber, uint8_t *dataBuffer, int lengthOfData, uint8_t flag){
    
    // CALCULATE PDU HEADER
    uint16_t lengthOfPDU = lengthOfData + LENGTH_OF_PDU_SIZE + FLAG_SIZE;
    uint16_t lengthOfPDU_net = htons(lengthOfPDU);
    uint8_t pdu[lengthOfPDU];

    // CREATE PDU
    memcpy(pdu, &lengthOfPDU_net, LENGTH_OF_PDU_SIZE);
    memcpy(pdu + LENGTH_OF_PDU_SIZE, &flag, FLAG_SIZE);
    if (lengthOfData > 0) {
        memcpy(pdu + LENGTH_OF_PDU_SIZE + FLAG_SIZE, dataBuffer, lengthOfData);
    }

    // SEND PDU
    ssize_t bytes_sent = send(socketNumber, pdu, lengthOfPDU, 0);
    if (bytes_sent < 0) {
        perror("error sending pdu");
        exit(1);
    }

    // RETURN PAYLOAD SIZE
    return bytes_sent - LENGTH_OF_PDU_SIZE - FLAG_SIZE;
}
