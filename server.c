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
	handleInit();

	// start looping for processing client connections and data
	serverControl(mainServerSocket);

	close(mainServerSocket);

	return 0;
}

void serverControl(int mainServerSocket) {
	int ready_socket = -1;	// socket descriptor for poll return
	int clientSocket = 0;   //socket descriptor for the client socket
	// keep server running until ctl+C
	while (1) {
		// check sockets with no timeout (==-1) and error check
		ready_socket = pollCall(-1);
		if (ready_socket < 0) {
			perror("timeout or poll error");
			exit(1);
		}

		// main server socket
		if (ready_socket == mainServerSocket) {
			// wait for client to connect
			clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
			// add new client socket to poll set
			addToPollSet(clientSocket);
		}

		// client sockets
		else {
			// get data from client
			recvFromClient(ready_socket);
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

	if (messageLen > 0)
	{
		printf("Message received on socket %d, Flag: %d length: %d Data: %s\n", flag, clientSocket, messageLen, dataBuffer);
		
		switch(flag) {
			case 1:
				// add to 
				break;
			default:
				break;
		}
		// send it back to client (just to test sending is working... e.g. debugging)
		// messageLen = safeSend(clientSocket, dataBuffer, messageLen, 0);
		// printf("Socket %d: msg sent: %d bytes, text: %s\n", clientSocket, messageLen, dataBuffer);
	}
	else
	{
		printf("Socket %d: Connection closed by other side\n", clientSocket);
		// remove from poll set and close socket
		removeFromPollSet(clientSocket);
		close(clientSocket);
	}
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

