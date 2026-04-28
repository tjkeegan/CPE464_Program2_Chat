/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
* Edited by Trevor Keegan
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>

#include "networks.h"
#include "safeUtil.h"
#include "sendPDU.h"
#include "recvPDU.h"
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void sendToServer();
int readFromStdin(uint8_t *buffer);
void checkArgs(int argc, char *argv[]);
void clientControl();

void sendInitPacket(char *argv[]);
void parseStdin(uint8_t *buffer, int sendLen);
void sendMessagePacket(char *destHandle, char *message);
void sendMulticastPacket(uint8_t numHandles, char *destHandles[], char *message);
void sendBroadcastPacket(char *message);
void sendTableRequestPacket();

void determinePacketType(uint8_t *dataBuffer, int bufferLen, int flag);
void recvHandleListLengthPacket(uint8_t *dataBuffer);
void recvHandlePacket(uint8_t *dataBuffer, int bufferLen);
void recvMessagePacket(uint8_t *dataBuffer, int bufferLen);
void recvMulticastPacket(uint8_t *dataBuffer, int bufferLen);
void recvBroadcastPacket();

char *clientHandle; // no '\0'
uint8_t clientHandleLen = 0;
int serverSocket = 0;
int receivingHandleList = 0;

int main(int argc, char *argv[])
{	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	serverSocket = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	// initialize poll set, add main server socket and stdin
	setupPollSet();
	addToPollSet(STDIN_FILENO);
	addToPollSet(serverSocket);

	sendInitPacket(argv);

	clientControl();
	
	return 0;
}

void sendInitPacket(char *argv[]) {
	int bytesSent = 0;

	// CREATE FLAG 1 PDU
	clientHandleLen = strlen(argv[1]); // no '\0'
	uint8_t flag1PDU[sizeof(clientHandleLen) + clientHandleLen];

	// set global variable
	clientHandle = malloc(clientHandleLen);
	if (clientHandle == NULL) {
		perror("Failed to allocate clientHandle");
		exit(1);
	}	
	memcpy(clientHandle, argv[1], clientHandleLen);

	// ASSEMBLE FLAG 1 PDU 
	memcpy(flag1PDU, &clientHandleLen, sizeof(clientHandleLen));
	memcpy(flag1PDU + sizeof(clientHandleLen), argv[1], clientHandleLen);

	// SEND FLAG 1 PDU
	bytesSent = sendPDU(serverSocket, flag1PDU, sizeof(clientHandleLen) + clientHandleLen, 1);
	if (bytesSent < 0) {
		perror("Failed to send Flag 1 PDU in initialPacker()");
		exit(1);
	}
}

void clientControl() {
	int ready_socket = -1;

	while (1) {

		if (!receivingHandleList) {
			printf("$: ");
			fflush(stdout); // forces buffered output to be written to stdout immediately
		}

		// GET READY SOCKET
		ready_socket = pollCall(-1); // blocks until a socket is ready
		if (ready_socket < 0) {
			perror("timeout or poll error");
			exit(1);
		}

		// STDIN READY
		if (ready_socket == STDIN_FILENO) {
			sendToServer();
		}

		// SERVER SOCKET READY
		else {
			uint8_t dataBuffer[MAXBUF];
			int messageLen = 0;
			uint8_t flag = 0;

			// RECEIVE DATA FROM SERVER
			if ((messageLen = recvPDU(serverSocket, dataBuffer, MAXBUF, &flag)) < 0){
				perror("recv call");
				exit(-1);
			}

			// SERVER TERMINATED
			if (messageLen == 0 && flag == 0){
				printf("Server terminated\n");
				removeFromPollSet(serverSocket);
				close(serverSocket);
				exit(1);
			}

			determinePacketType(dataBuffer, messageLen, flag);
			//printf("Socket %d: Message received, length: %d Data: %s\n", serverSocket, messageLen, dataBuffer);
		}
	}
}

void determinePacketType(uint8_t *dataBuffer, int bufferLen, int flag) {
	switch (flag) {
		case 11: // NUMHANDLES PACKET
			receivingHandleList = 1;
			recvHandleListLengthPacket(dataBuffer);
			break;
		case 12: // HANDLE PACKET
			if (!receivingHandleList) {
				perror("Received Handle packet (12) when not requested");
				exit(1);
			}
			recvHandlePacket(dataBuffer, bufferLen);
			break;
		case 13: // HANDLE PACKETS FINISHED
			if (!receivingHandleList) {
				perror("Received a Handle Packets Finished packet (13) when not requested");
				exit(1);
			}
			receivingHandleList = 0;
			break;
		default:
			if (receivingHandleList) {
				perror("Received a non List Handle packet while listing handles");
				exit(1);
			}
			else {
				switch (flag) {
					case 2: // GOOD INIT HANDLE CONFIRMATION
						break;
					case 3: // BAD INIT HANDLE ERROR
						break;
					case 4: // BROADCAST PACKET
						recvBroadcastPacket(dataBuffer, bufferLen);
						break;
					case 5: // MESSAGE PACKET
						recvMessagePacket(dataBuffer, bufferLen);
						break;
					case 6: // MULTICAST PACKET
						recvMulticastPacket(dataBuffer, bufferLen);
						break;
					case 7: // BAD DEST HANDLE ERROR
						break;
				}
			}
	}
}

void recvHandleListLengthPacket(uint8_t *dataBuffer) {
	uint32_t numHandles_net;
	memcpy(&numHandles_net, dataBuffer, sizeof(numHandles_net));
	uint32_t numHandles = ntohl(numHandles_net);
	printf("Number of clients: %d\n", numHandles);
}

void recvHandlePacket(uint8_t *dataBuffer, int bufferLen) {
	uint8_t handleLen = dataBuffer[0];
	char handle[handleLen + 1]; // with '\0'
	memcpy(handle, dataBuffer + sizeof(handleLen), handleLen);
	handle[handleLen] = '\0';
	printf("   %s\n", handle);
}

void recvMessagePacket(uint8_t *dataBuffer, int bufferLen) {
	
	// PARSE SENDER HANDLE
	uint8_t senderHandleLen = dataBuffer[0];
	char senderHandle[senderHandleLen + 1]; // with '\0'
	memcpy(senderHandle, dataBuffer + sizeof(senderHandleLen), senderHandleLen);
	senderHandle[senderHandleLen] = '\0';

	// PARSE DEST HANDLE LENGTH
	uint8_t destHandleLen = dataBuffer[senderHandleLen + 2]; // +2 for senderHandleLen and numDest
	
	// PARSE MESSAGE
	uint8_t messageOffset = sizeof(senderHandleLen) + senderHandleLen + 1 + sizeof(destHandleLen) + destHandleLen;
	int messageLen = bufferLen - messageOffset;
	char message[messageLen];
	memcpy(message, dataBuffer + messageOffset, messageLen);

	// PRINT MESSAGE
	printf("\n%s: %s\n", senderHandle, message);
}

void recvMulticastPacket(uint8_t *dataBuffer, int bufferLen) {

	// PARSE SENDER HANDLE
	uint8_t senderHandleLen = dataBuffer[0];
	uint8_t senderHandle[senderHandleLen + 1];
	memcpy(senderHandle, dataBuffer + sizeof(senderHandleLen), senderHandleLen);
	senderHandle[senderHandleLen] = '\0';
	
	// PARSE PAST DEST HANDLES
	uint8_t numHandles = dataBuffer[sizeof(senderHandleLen) + senderHandleLen];
	int offset = sizeof(senderHandleLen) + senderHandleLen + sizeof(numHandles);
	for (int i = 0; i < numHandles; i++) {
		offset += (dataBuffer[offset] + 1);
	}

	// PARSE MESSAGE
	uint8_t messageLen = bufferLen - offset;
	uint8_t message[messageLen];
	memcpy(message, dataBuffer + offset, messageLen);

	// PRINT MESSAGE
	printf("\n%s: %s\n", senderHandle, message);
}

void recvBroadcastPacket(uint8_t *dataBuffer, int bufferLen){
	// PARSE SENDER HANDLE
	uint8_t senderHandleLen = dataBuffer[0];
	char senderHandle[senderHandleLen + 1]; // with '\0'
	memcpy(senderHandle, dataBuffer + sizeof(senderHandleLen), senderHandleLen);
	senderHandle[senderHandleLen] = '\0';

	// PARSE MESSAGE
	uint8_t messageOffset = sizeof(senderHandleLen) + senderHandleLen;
	int messageLen = bufferLen - messageOffset;
	char message[messageLen];
	memcpy(message, dataBuffer + messageOffset, messageLen);

	// PRINT MESSAGE
	printf("\n%s: %s\n", senderHandle, message);
}

void sendToServer()
{
	uint8_t buffer[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send

	sendLen = readFromStdin(buffer);

	parseStdin(buffer, sendLen);
}

int readFromStdin(uint8_t * buffer)
{
	char aChar = 0;
	int inputLen = 0;        

	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;

	return inputLen;
}

void parseStdin(uint8_t *buffer, int sendLen) {
	if (sendLen < 2) {
		printf("usage:");
	}
	else {
		char *command = strtok((char *) buffer, " ");
		switch (command[1]) {
			case 'M':
			case 'm': {
				char *handle = strtok(NULL, " ");
				char *message = strtok(NULL, "");
				sendMessagePacket(handle, message);
				break;
			}
			case 'B':
			case 'b': {
				char *message = strtok(NULL, "");
				sendBroadcastPacket(message);
				break;
			}
			case 'C':
			case 'c': { // brackets to make it ok to declare variables
				uint8_t numHandles = (uint8_t)atoi(strtok(NULL, " "));
				char *destHandles[numHandles];
				for (int i = 0; i < numHandles; i++) {
					destHandles[i] = strtok(NULL, " ");
				}
				char *message = strtok(NULL, "");
				sendMulticastPacket(numHandles, destHandles, message);
				break;
			}
			case 'L':
			case 'l':
				sendTableRequestPacket();
				break;
			default:
				break;
		}
	}
}

void sendMessagePacket(char *destHandle, char *message) {
	// CREATE PDU
	uint8_t numDest = 1;
	uint8_t destHandleLen = strlen(destHandle);
	uint8_t messageLen = strlen(message) + 1; // with '\0'

	int pduSize = sizeof(clientHandleLen) + clientHandleLen + sizeof(numDest) + sizeof(destHandleLen) + destHandleLen + messageLen;
	uint8_t pdu[pduSize];

	// ASSEMBLE PDU
	memcpy(pdu, &clientHandleLen, sizeof(clientHandleLen));
	memcpy(pdu + sizeof(clientHandleLen), clientHandle, clientHandleLen);
	memcpy(pdu + sizeof(clientHandleLen) + clientHandleLen, &numDest, sizeof(numDest));
	memcpy(pdu + sizeof(clientHandleLen) + clientHandleLen + sizeof(numDest), &destHandleLen, sizeof(destHandleLen));
	memcpy(pdu + sizeof(clientHandleLen) + clientHandleLen + sizeof(numDest) + sizeof(destHandleLen), destHandle, destHandleLen);
	memcpy(pdu + sizeof(clientHandleLen) + clientHandleLen + sizeof(numDest) + sizeof(destHandleLen) + destHandleLen, message, messageLen);

	// SEND PDU
	safeSendPDU(serverSocket, pdu, pduSize, 5);
}

void sendMulticastPacket(uint8_t numHandles, char *destHandles[], char *message) {
	// CREATE PDU
	uint8_t destHandlesLen[numHandles];
	int totalHandlesLen = 0;
	for (int i = 0; i < numHandles; i++) {
		uint8_t len = (uint8_t) strlen(destHandles[i]);
		destHandlesLen[i] = len;
		totalHandlesLen += len;
	}
	uint8_t messageLen = strlen(message) + 1; // with '\0'
	size_t pduLen = sizeof(clientHandleLen) + clientHandleLen + sizeof(numHandles) + numHandles + totalHandlesLen + messageLen;
	uint8_t pdu[pduLen];

	// ASSEMBLE PDU
	memcpy(pdu, &clientHandleLen, sizeof(clientHandleLen));
	memcpy(pdu + sizeof(clientHandleLen), clientHandle, clientHandleLen);
	memcpy(pdu + sizeof(clientHandleLen) + clientHandleLen, &numHandles, sizeof(numHandles));
	int offset = sizeof(clientHandleLen) + clientHandleLen + sizeof(numHandles);
	for (int i = 0; i < numHandles; i++) {
		memcpy(pdu + offset, &destHandlesLen[i], sizeof(destHandlesLen[i]));
		offset += sizeof(destHandlesLen[i]);
		memcpy(pdu + offset, destHandles[i], destHandlesLen[i]);
		offset += destHandlesLen[i];
	}
	memcpy(pdu + offset, message, messageLen);

	// SEND PDU
	safeSendPDU(serverSocket, pdu, pduLen, 6);
}

void sendBroadcastPacket(char *message) {
	// CREATE PDU
	int messageLen = strlen(message) + 1; // send message with '\0'
	uint8_t pduLen = sizeof(clientHandleLen) + clientHandleLen + messageLen;
	uint8_t pdu[pduLen];

	// ASSEMBLE PDU
	memcpy(pdu, &clientHandleLen, sizeof(clientHandleLen));
	memcpy(pdu + sizeof(clientHandleLen), clientHandle, clientHandleLen);
	memcpy(pdu + sizeof(clientHandleLen) + clientHandleLen, message, messageLen);

	// SEND PDU
	safeSendPDU(serverSocket, pdu, pduLen, 4);
}

void sendTableRequestPacket() {
	// SEND PDU
	receivingHandleList = 1;
	safeSendPDU(serverSocket, NULL, 0, 10);
}

void checkArgs(int argc, char * argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle host-name port-number \n", argv[0]);
		exit(1);
	}
}
