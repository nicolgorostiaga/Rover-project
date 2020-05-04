/**
 * @file tx2_can_node.c
 * @author Patrick Henz
 * @date 4-4-2019
 * @brief CAN node for TX2.
 * @details CAN node for the TX2. To interface with the TX2s CAN controller, socket CAN is used. 
 *          Refer to https://www.kernel.org/doc/Documentation/networking/can.txt for additional
 *          information in regards to socket CAN. This module intiializes the CAN controller and
 *          performs all communication too and from any devices that are attached to the CAN bus.
**/

#define DEBUG /**< Definition compiles the CAN node in debug mode. */

#include "../include/CanController.h"
#include "../include/Messages.h"

#include <stdio.h>

int main(int argc, char ** argv)
{
#ifdef DEBUG
	printf("starting CAN node\n");
#endif
	fd_set rdfs;
	int status;
	int canSocket;
	int masterRead;
	int masterWrite;
	int nbytes;
	int readFds[2];
	int i; // for test
	int killMessageReceived;

	Message message;
	Message previousMessage;

	// make sure master node has given use the correct
	// number of pipes
	if (argc != 3) {
		printf("Error starting Can Node\n");
		return -1;
	}

	// set read and write pipes
	masterRead = atoi(argv[1]);
	masterWrite = atoi(argv[2]);

	// initialize CAN, capture fd so we can use it for
	// SetAndWait
	canSocket = InitializeCan();

	readFds[0] = canSocket;
	readFds[1] = masterRead;

	// initialize SetAndWait
	SetupSetAndWait(readFds, 2);

	killMessageReceived = 0;

	while(!killMessageReceived) {
		// wait for an fd to be read to read
		if (SetAndWait(&rdfs, 1, 0) < 0) {
			printf("SET AND WAIT ERROR CAN\n");
		}

		// check fds
		for (i = 0; i < 2; i++) {
			if (!FD_ISSET(readFds[i], &rdfs)) {
				continue;
			}

			if (readFds[i] == canSocket) {
				CanRead(&message);

				///////////////////////////////////
				//	TODO: CAN READ		//
				///////////////////////////////////
				
				// this functionality exists and works correctly, there
				// just isn't anything to read from at the moment.
			} else if (readFds[i] == masterRead) {
				read(masterRead, &message, sizeof(message));
				
				// kill message
				if (message.messageType == KillMessage) {
					killMessageReceived = 1;
					CloseCan();
					close(masterRead);
					close(masterWrite);
				} else {
					// as mentioned in Messages.h, writeCount is used to write
					// a message to the CAN bus a certain number of times.
					// This is primarily used for multi-turn commands
					for (i = 0; i < message.canMsg.writeCount; i++) {
						CanWrite(&message);
					}
				}
			}
		}
	}

	printf("killing can node\n");
	return 0;
}
