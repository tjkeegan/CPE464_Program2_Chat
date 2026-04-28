/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
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
#include "recvPDU.h"
#include "sendPDU.h"
#include "pollLib.h"
#include "handle.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void serverControl();

void recvInitPacket(int clientSocket, uint8_t *dataBuffer);
void recvMessagePacket(uint8_t *dataBuffer, int bufferLen);
void recvBroadcastPacket(int clientSocket, uint8_t *dataBuffer, int bufferLen);
void recvMulticastPacket(int clientSocket, uint8_t *dataBuffer, int messageLen);
void recvTableRequestPacket(int clientSocket);

int main(int argc, char *argv[])
{
	int mainServerSocket = 0;   //socket descriptor for the server socket
	int portNumber = 0;
	
	portNumber = checkArgs(argc, argv);
	
	//create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	// initialize poll set and add main server socket to it
	setupPollSet();
	addToPollSet(mainServerSocket);

	handleSetUp();

	// start looping for processing client connections and data
	serverControl(mainServerSocket);

	close(mainServerSocket);

	return 0;
}

void serverControl(int mainServerSocket) {
	int readySocket = -1;	// socket descriptor for poll return
	int clientSocket = 0;   //socket descriptor for the client socket
	
	while (1) {
		
		// GET READY SOCKET
		readySocket = pollCall(-1); // blocks until a socket is ready
		if (readySocket < 0) {
			perror("timeout or poll error");
			exit(1);
		}

		// MAIN SERVER SOCKET READY
		if (readySocket == mainServerSocket) {
			clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG); // wait for client to connect
			addToPollSet(clientSocket); // add new client socket to poll set
		}

		// CLIENT SOCKET READY
		else {
			recvFromClient(readySocket); // get data from client
		}
	}
}

void recvFromClient(int clientSocket)
{
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	uint8_t flag = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF, &flag)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0 || flag != 0)
	{
		//printf("Message received on socket %d, Flag: %d length: %d Data: %s\n", clientSocket, flag, messageLen, (char *)(dataBuffer + 1));
		
		switch(flag) {
			case 1: // INIT PACKET
				recvInitPacket(clientSocket, dataBuffer);
				break;
			case 4: // BROADCAST PACKET
				recvBroadcastPacket(clientSocket, dataBuffer, messageLen);
				break;
			case 5: // MESSAGE PACKET
				recvMessagePacket(dataBuffer, messageLen);
				break;
			case 6: // MULTICAST PACKET
				recvMulticastPacket(clientSocket, dataBuffer, messageLen);
				break;
			case 10: // HANDLE LIST REQUEST
				recvTableRequestPacket(clientSocket);
				break;
			
			default:
				break;
		}
	}
	else
	{
		printf("Socket %d: Connection closed by other side\n", clientSocket);
		// remove from poll set and close socket
		char *clientHandle = findHandle(clientSocket);
		removeHandle(clientHandle); // error check
		removeFromPollSet(clientSocket);
		close(clientSocket);
	}
}

void recvInitPacket(int clientSocket, uint8_t *dataBuffer) {
	uint8_t handleLen = dataBuffer[0];
	char handle[handleLen + 1];
	memcpy(handle, dataBuffer + sizeof(handleLen), handleLen);
	handle[handleLen] = '\0';
	addHandle(clientSocket, handle);
}

void recvMessagePacket(uint8_t *dataBuffer, int bufferLen) {
	uint8_t sendHandleLen = dataBuffer[0];
	uint8_t destHandleLen = dataBuffer[sendHandleLen + 2]; // +2 for handleLen and numDest values
	uint8_t destHandle[destHandleLen];
	memcpy(destHandle, dataBuffer + sizeof(sendHandleLen) + sendHandleLen + 1 + sizeof(destHandleLen), destHandleLen);
	int destSocket = findSocket(destHandle, destHandleLen);
	safeSendPDU(destSocket, dataBuffer, bufferLen, 5);
}

void recvBroadcastPacket(int clientSocket, uint8_t *dataBuffer, int bufferLen) {
	int numHandles = getNumHandles();
	for (int i = 0; i < numHandles; i++) {
		int destSocket = findSocketByIndex(i);
		if (destSocket != clientSocket) {
			safeSendPDU(destSocket, dataBuffer, bufferLen, 4);
		}
	}
}

void recvMulticastPacket(int clientSocket, uint8_t *dataBuffer, int bufferLen) {
	uint8_t sendHandleLen = dataBuffer[0];
	uint8_t numHandles = dataBuffer[sizeof(sendHandleLen) + sendHandleLen];
	int offset = sizeof(sendHandleLen) + sendHandleLen + sizeof(numHandles);
	int destSocket = 0;
	uint8_t destHandlesLen[numHandles];
	uint8_t *destHandles[numHandles];
	for (int i = 0; i < numHandles; i++) {
		destHandlesLen[i] = dataBuffer[offset];
		offset += sizeof(destHandlesLen[i]);
		destHandles[i] = dataBuffer + offset;
		offset += destHandlesLen[i];
		destSocket = findSocket(destHandles[i], destHandlesLen[i]); // error check this
		safeSendPDU(destSocket, dataBuffer, bufferLen, 6);
	}
}

void recvTableRequestPacket(int clientSocket) {

	// SEND NUMHANDLES PDU (FLAG 11)
	uint32_t numHandles = getNumHandles();
	uint32_t numHandles_net = htonl(numHandles);
	uint8_t *numHandles_send = (uint8_t *) &numHandles_net;
	safeSendPDU(clientSocket, numHandles_send, sizeof(numHandles_net), 11);

	// SEND HANDLES PDUS (FLAG 12)
	for (int i = 0; i < numHandles; i++) {
		char *handle = findHandleByIndex(i);
		uint8_t handleLen = strlen(handle); // no '\0'
		uint8_t pdu[sizeof(handleLen) + handleLen];
		pdu[0] = handleLen;
		memcpy(pdu + sizeof(handleLen), handle, handleLen);
		safeSendPDU(clientSocket, pdu, sizeof(handleLen) + handleLen, 12);
	}

	// SEND FINISHED PDU (FLAG 13)
	safeSendPDU(clientSocket, NULL, 0, 13);
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}
