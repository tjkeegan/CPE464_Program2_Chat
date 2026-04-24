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

void sendToServer(int serverSocket);
int readFromStdin(uint8_t *buffer);
void checkArgs(int argc, char *argv[]);
void clientControl(int serverSocket);

void sendInitPacket(char *argv[], int serverSocket);
void parseStdin(uint8_t *buffer, int sendLen, int serverSocket);
void sendMessagePacket(char *destHandle, char *message, int serverSocket);
void sendBroadcastPacket(char *message, int serverSocket);
void listHandlesPacket(int serverSocket);

void determinePacketType(uint8_t *dataBuffer, int bufferLen, int flag);
void recvMessagePacket(uint8_t *dataBuffer, int bufferLen);

char *clientHandle;

int main(int argc, char *argv[])
{
	int serverSocket = 0;         //socket descriptor
	
	checkArgs(argc, argv);

	/* set up the TCP Client socket  */
	serverSocket = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	// initialize poll set, add main server socket and stdin
	setupPollSet();
	addToPollSet(STDIN_FILENO);
	addToPollSet(serverSocket);

	sendInitPacket(argv, serverSocket);

	clientControl(serverSocket);
	
	return 0;
}

void sendInitPacket(char *argv[], int serverSocket) {
	int bytesSent = 0;

	// CREATE FLAG 1 PDU
	uint8_t handleLen = strlen(argv[1]) + 1; // +1 for '\0'
	uint8_t flag1PDU[sizeof(handleLen) + handleLen];

	// set global variable
	clientHandle = malloc(handleLen);
	if (clientHandle == NULL) {
		perror("Failed to allocate clientHandle");
		exit(1);
	}	
	memcpy(clientHandle, argv[1], handleLen);

	// ASSEMBLE FLAG 1 PDU 
	memcpy(flag1PDU, &handleLen, sizeof(handleLen));
	memcpy(flag1PDU + sizeof(handleLen), argv[1], handleLen);

	// SEND FLAG 1 PDU
	bytesSent = sendPDU(serverSocket, flag1PDU, sizeof(handleLen) + handleLen, 1);
	if (bytesSent < 0) {
		perror("Failed to send Flag 1 PDU in initialPacker()");
		exit(1);
	}
}

void clientControl(int serverSocket) {
	int ready_socket = -1;

	while (1) {

		printf("$: ");
		fflush(stdout); // forces buffered output to be written to stdout immediately

		// GET READY SOCKET
		ready_socket = pollCall(-1); // blocks until a socket is ready
		if (ready_socket < 0) {
			perror("timeout or poll error");
			exit(1);
		}

		// STDIN READY
		if (ready_socket == STDIN_FILENO) {
			sendToServer(serverSocket);
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
			if (messageLen == 0){
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
	// DETERMINE PACKET TYPE
	switch (flag) {
		case 2: // GOOD INIT HANDLE CONFIRMATION
			break;
		case 3: // BAD INIT HANDLE ERROR
			break;
		case 4: // BROADCAST PACKET
			break;
		case 5: // MESSAGE PACKET
			recvMessagePacket(dataBuffer, bufferLen);
			break;
		case 6: // MULTICAST PACKET
			break;
		case 7: // BAD DEST HANDLE ERROR
			break;
		case 11: // HANDLE LIST LENGTH
			break;
		case 12: // HANDLE LIST ITEM
			break;
		case 13: // HANDLE LIST FINISHED
			break;
	}
}

void recvMessagePacket(uint8_t *dataBuffer, int bufferLen) {
	
	// PARSE SENDER HANDLE
	uint8_t senderHandleLen = dataBuffer[0];
	uint8_t senderHandle[senderHandleLen];
	memcpy(senderHandle, dataBuffer + sizeof(senderHandleLen), senderHandleLen);

	// PARSE DEST HANDLE LENGTH
	uint8_t destHandleLen = dataBuffer[senderHandleLen + 2]; // +2 for senderHandleLen and numDest
	
	// PARSE MESSAGE
	uint8_t messageOffset = sizeof(senderHandleLen) + senderHandleLen + 1 + sizeof(destHandleLen) + destHandleLen;
	int messageLen = bufferLen - messageOffset;
	uint8_t message[messageLen];
	memcpy(message, dataBuffer + messageOffset, messageLen);

	// PRINT MESSAGE
	printf("\n%s: %s\n", senderHandle, message);
}

void sendToServer(int serverSocket)
{
	uint8_t buffer[MAXBUF];   //data buffer
	int sendLen = 0;        //amount of data to send

	sendLen = readFromStdin(buffer);

	parseStdin(buffer, sendLen, serverSocket);
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

void parseStdin(uint8_t *buffer, int sendLen, int serverSocket) {
	//uint8_t numHandles = 0;
	char *handle;
	char *message;
	//char *destHandle;
	
	if (sendLen < 2) {
		printf("usage:");
	}
	else {
		char *command = strtok((char *) buffer, " ");
		switch (command[1]) {
			case 'M':
			case 'm':
				handle = strtok(NULL, " ");
				message = strtok(NULL, "");
				sendMessagePacket(handle, message, serverSocket);
				break;
			case 'B':
			case 'b':
				message = strtok(NULL, "");
				broadcastPacket(message, serverSocket);
				break;
			case 'C':
			case 'c':
				/*numHandles = (uint8_t)atoi(strtok(NULL, " "));
				destHandle = strtok(NULL, " ");
				// find a way to parse rest of handles into an array
				multiCastPacket();*/
				break;
			case 'L':
			case 'l':
				listHandlesPacket(serverSocket);
				break;
			default:
				break;
		}
	}
}

void sendMessagePacket(char *destHandle, char *message, int serverSocket) {
	// CREATE PDU
	uint8_t sendHandleLen = strlen(clientHandle);
	uint8_t numDest = 1;
	uint8_t destHandleLen = strlen(destHandle);
	uint8_t messageLen = strlen(message);

	int pduSize = sizeof(sendHandleLen) + sendHandleLen + sizeof(numDest) + sizeof(destHandleLen) + destHandleLen + messageLen;
	uint8_t pdu[pduSize];

	// ASSEMBLE PDU
	memcpy(pdu, &sendHandleLen, sizeof(sendHandleLen));
	memcpy(pdu + sizeof(sendHandleLen), clientHandle, sendHandleLen);
	memcpy(pdu + sizeof(sendHandleLen) + sendHandleLen, &numDest, sizeof(numDest));
	memcpy(pdu + sizeof(sendHandleLen) + sendHandleLen + sizeof(numDest), &destHandleLen, sizeof(destHandleLen));
	memcpy(pdu + sizeof(sendHandleLen) + sendHandleLen + sizeof(numDest) + sizeof(destHandleLen), destHandle, destHandleLen);
	memcpy(pdu + sizeof(sendHandleLen) + sendHandleLen + sizeof(numDest) + sizeof(destHandleLen) + destHandleLen, message, messageLen);

	// SEND PDU
	safeSendPDU(serverSocket, pdu, pduSize, 5);
}


void sendBroadcastPacket(char *message, int serverSocket) {
	// CREATE PDU
	uint8_t sendHandleLen = strlen(clientHandle);
	uint8_t pduLen = sizeof(sendHandleLen) + sendHandleLen + strlen(message);
	uint8_t pdu[pduLen];

	// ASSEMBLE PDU
	memcpy(pdu, &sendHandleLen, sizeof(sendHandleLen));
	memcpy(pdu + sizeof(sendHandleLen), clientHandle, sendHandleLen);
	memcpy(pdu + sizeof(sendHandleLen) + sendHandleLen, message, strlen(message));

	// SEND PDU
	safeSendPDU(serverSocket, pdu, pduLen, 4);
}
/*
void multiCastPacket() {
	// CREATE PDU

	// ASSEMBLE PDU

	// SEND PDU
	//safeSendPDU();
}
*/

void listHandlesPacket(int serverSocket) {
	// SEND PDU
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
