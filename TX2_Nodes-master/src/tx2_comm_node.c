/**
 * @file tx2_comm_node.c
 * @author Patrick Henz
 * @date 4-8-2019
 * @brief Communication node for TX2.
 * @details Communication node for the TX2. This node is responsible for all communication
 *  	    with the outside world. This, at the moment, only happens with controller.c and 
 *  	    logWrite.c. Any control or information messages are passed through here. TCP sockets
 *  	    are used to allow tx2_comm_node.c and conroller.c/logWriter.c to communicate with 
 *  	    one another. No other external libraries are used; only preexisting functionality
 *  	    within Linux. 
**/

#include <stdio.h>
#include "../include/Messages.h"
#include "../include/CommController.h"
#include <unistd.h>
#include <signal.h>

#define NOTHING_FROM_CLIENT_TIMEOUT 300 
#define NOTHING_FROM_CLIENT_AFTER_SOCKET_CHECK_TIMEOUT 60

#define DEBUG /**< This compiles the program for debug mode. There are certain macros and print
		   statements that are included when DEBUG is defined. Comment out for non-debug
		   commpilation. */

#define SOCKET_OK "OK"
#define PORT 5000 /**< Port number used for communication */
#define ADD_POR_REUSE (SO_REUSEADDR | SO_REUSEPORT)

#ifdef DEBUG
#define WASD_PRESS(c) (c == 'w' || c == 'W' ||\
		       c == 'a' || c == 'A' ||\
		       c == 's' || c == 'S' ||\
		       c == 'd' || c == 'D') /**< Debug macro to check incoming key presses from
		       				  the controller.c program. */
#endif

int main(int argc, char ** argv)
{
#ifdef DEBUG
	printf("staring communication node\n");
#endif
	fd_set rdfs;
	int status;
	int tcpSocket;
	int oldTcpSocket;
	int masterRead;
	int masterWrite;
	int nbytes;
	int readFds[2];
	int i;
	int killMessageReceived;
	unsigned int socketCheckSent;
	unsigned int wakeCount;

	Message commInMessage;
	Message commOutMessage;

	// make sure master has given us enough pipes
	if (argc != 3) {
		printf("Error starting Comm Node\n");
		return -1;
	}

	// assign the pipes
	masterRead = atoi(argv[1]);
	masterWrite = atoi(argv[2]);

	// initialize TCP socket, store as oldTcpSocket
	// this is used in the scenario that the clinet disconnects
	// and a new socket is needed.
	tcpSocket = InitializeComm(PORT);
	oldTcpSocket = tcpSocket;

	readFds[0] = masterRead;
	readFds[1] = tcpSocket;

	// setup SetAndWait
	SetupSetAndWait(readFds, 2);

	socketCheckSent = 0;

	killMessageReceived = 0;
	wakeCount = 0;

	//  main while loop
	while(!killMessageReceived) {
		// wait until fd is available or timeout has occured
		if (SetAndWait(&rdfs, 1, 0) < 0 ) {
			printf("SET AND WAIT ERROR COMM\n");
		}		

		// check fds, see if we can read from one
		for (i = 0; i < 2; i++) {
			if (!FD_ISSET(readFds[i], &rdfs)) {
				continue;
			}

			// new message from controller
			if (readFds[i] == tcpSocket) {
				// reset wake count, we know we are connected to the client
				wakeCount = 0;

				// read the incoming message
				CommRead(&commInMessage);

				//  the only time this should happen if the client sent a 
				//  request to the TX2 with the TX2 sending a socket check at 
				//  the same time. We should have already processed the 
				//  request from the client. See below.
				if (commInMessage.messageType == OKMessage) {
					socketCheckSent = 0;
					printf("OK received, reseting alarm value\n");
				} else if (commInMessage.messageType == ClientDisconnect) {
					// clinet has disconnected purposefully
					printf("client requsting disconnect\n");
					//get new socket
					tcpSocket = EstablishSocket();
					// modify fds for SetAndWait
					ModifySetAndWait(oldTcpSocket, tcpSocket);
					oldTcpSocket = tcpSocket;
				} else {
					// the message is not for the comm node, but rather for
					// another TX2 node. Send it of to master so it can figure out
					// what to do with it.
					commInMessage.source = TX2Comm;
					write(masterWrite, &commInMessage, sizeof(commInMessage));
				}
			} else if (readFds[i] == masterRead) {
				// the message is coming from master, not the controller
				read(masterRead, &commOutMessage, sizeof(commOutMessage));

				// tx2_cam_node.cpp has a new picture for us to send over to the client
				if (commOutMessage.messageType == CamMessage) {
					printf("send image..\n");
					CommImageWrite(&commOutMessage);
				} else if (commOutMessage.messageType == KillMessage) {
					// master has sent us a kill message
					killMessageReceived = 1;
					CloseSocket();
					close(masterWrite);
					close(masterRead);
				} else {
					// this is here for future implementation. There isn't anything
					// that is sent back to the client as of 12-12-2019, apart from
					// socket check messages (which don't happen here specifically)
					// and images
					CommWrite(&commOutMessage);
				}
			}
		}

		// since the SetAndWait function at the top is set to wakeup every second if no message is available, using
		// this counter as a way to figure out how many times we have woke up without messages from the client/controller.
		wakeCount++;

		// at this point, client has likely disconnected, the socket check has been set and we havent
		// received anything from the controller. We should assume the client has disconnected and listen
		// for a new incoming request to re-establish communication.
		if (wakeCount == NOTHING_FROM_CLIENT_AFTER_SOCKET_CHECK_TIMEOUT && socketCheckSent == 1) {
			printf("client disconnected\n");
			// get new socket
			tcpSocket = EstablishSocket();
			// modify SetAndWait array
			ModifySetAndWait(oldTcpSocket, tcpSocket);
			oldTcpSocket = tcpSocket;
			wakeCount = 0;
			socketCheckSent = 0;
		} else if (wakeCount == NOTHING_FROM_CLIENT_TIMEOUT) {
			// we havent heard from client in a while, send a socket check
			printf("checking socket\n");
			SocketCheck();
			// reset so we can start counting from 9
			wakeCount = 0;
			socketCheckSent = 1;
		}
	}

	printf("killing comm node\n");
	return 0;
}
