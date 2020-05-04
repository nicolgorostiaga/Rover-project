/**
 * @file tx2_master.c
 * @author Patrick Henz
 * @date 12-1-2019 
 * @brief Master node for TX2.
 * @details Master node for TX2. This node is responsible for creating all child processes (other
 * 	    TX2 nodes) as well as the pipes these nodes use to commuicate with one another. Apart
 * 	    from creating the other nodes, the TX2 master node aslo acts as a routing mechanism for
 * 	    the other  nodes to pass messages around between one another. If a node wishes to send
 * 	    a #Message to another node, it must go through master; no pipes exist between child nodes.
 * 	    The only exception for interporcess communication is shared memory, but master has nothing
 * 	    to do with the creation or maintentance of shared memory.
 * 	    <br>
 * 	    <br>
 * 	    The master node is also responsible for maintaining a command queue via the Command.h library.
 * 	    This gives master the ability to send commands to child nodes to have them execute a specific
 * 	    type of functionality. When a child node finishes executing the command, it sends a request to
 * 	    master to pop the command queue and send out another command to the appropriate node.
 */

#define DEBUG /**< Used to compile the master node in debug mode. */

#include "../include/Messages.h"
#include "../include/Command.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

// when pipe is called, index 0 is a read, index 1 is a write
#define READ 0
#define WRITE 1

/**
 * @brief Defines the number of child nodes
**/
#define CHILD_COUNT 6

/**
 * @brief Calls to execute child nodes.
**/
char * ExecuteCommands[CHILD_COUNT] = {
	"./tx2_can_node",
	"./tx2_comm_node",
	"./tx2_cam_node",
	"./tx2_nav_node",
	"./tx2_gps_node",
	"./tx2_gyro_node"
};

/**
 * @brief Child node names.
**/
char * ChildNames[CHILD_COUNT] = {
	"tx2_can_node",
	"tx2_comm_node",
	"tx2_cam_node",
	"tx2_nav_node",
	"tx2_gps_node",
	"tx2_gyro_node"
};

/**
 * @brief Child node enums from Messages.h.
**/
int ChildIdentifiers[CHILD_COUNT] = {
	TX2Can,
	TX2Comm,
	TX2Cam,
	TX2Nav,
	TX2Gps,
	TX2Gyro
};

/**
 * @brief Macro used to determine if node is child node or parent.
**/
#define IS_CHILD_NODE(p) (0 == (p))

/**
 * @brief Internal function used to create child nodes and pipes.
 * @details Internal function used by TX2 master to create child nodes
 * 	    and the pipes needed to communicate with those nodes.
 * @param readPipes A pointer to the pipes that master reads from.
 * @param writePipes A pointer to the pipes that master writes to.
 * @return Returns 0 if success, -1 if error.
**/
int InitializeTX2Nodes(int * readPipes, int * writePipes)
{
	int tempReadPipe[2];
	int tempWritePipe[2];
	int tempChildPid;

	// used to pass pipe fds to child processes
	char param1[16];
	char param2[16];

	int i; // loop variable
	
	for (i = 0; i < CHILD_COUNT; i++) {
		// wipe old data
		memset(tempReadPipe, 0, sizeof(tempReadPipe));
		memset(tempWritePipe, 0, sizeof(tempWritePipe));
		memset(param1, 0, sizeof(param1));
		memset(param2, 0, sizeof(param2));

		// create pipes
		if(pipe(tempReadPipe) | pipe(tempWritePipe)) {
			printf("Error creating %s Pipes\n", ChildNames[i]);
			return -1;
		}
		
		// create child node process
		tempChildPid = fork();

		// is this the child? If so, take care of pipes and call #ExecuteCommands 
		if(IS_CHILD_NODE(tempChildPid)) {
			close(tempReadPipe[READ]);
			close(tempWritePipe[WRITE]);
			sprintf(param1, "%d", tempWritePipe[READ]); // CAN read
			sprintf(param2, "%d", tempReadPipe[WRITE]); // CAN write
			execl(ExecuteCommands[i], ChildNames[i], param1, param2, (char *)NULL);
		} else {
			// this is master node, close appropriate pipes
			close(tempReadPipe[WRITE]);
			close(tempWritePipe[READ]);

			// retain read and write pipes
			readPipes[ChildIdentifiers[i]] = tempReadPipe[READ];
			writePipes[ChildIdentifiers[i]] = tempWritePipe[WRITE];
		}
	}

	return 0;
}

int main(int argc, char ** argv)
{
#ifdef DEBUG
	printf("starting master node\n");
#endif	
	fd_set rdfs;
	struct timeval timeout;

	// pipes used during execution
	int readPipes[CHILD_COUNT];
	int writePipes[CHILD_COUNT];

	int status;
	int killMessageReceived;
	int i;

	Message message;
	unsigned int messageOkToSend;
	
	// create the child nodes and the pipes needed to 
	// communicate with them
	if (InitializeTX2Nodes(readPipes, writePipes) != 0) {
		printf("ERROR CREATING CHILD NODES");
		return -1;
	} else {
		printf("\n\nCHILD NODE/PIPE CREATION SUCCESS\n\n");
	}

	// initialize set and wait
	SetupSetAndWait(readPipes, CHILD_COUNT);

	killMessageReceived = 0;	

	while(!killMessageReceived) {
		// wait until fd is available or timeout
		if (SetAndWait(&rdfs, 1, 0) < 0) {
			printf("\n\nERROR\n\n)");
		}		

		// check each fd to see if message is available
		for (i = 0; i < CHILD_COUNT; i++) {
			if (!FD_ISSET(readPipes[i], &rdfs)) {
				continue;
			}

			// read the message
			read(readPipes[i], &message, sizeof(message));

			// manual needs to go through nav
			if (TX2Comm == message.source && TX2Can == message.destination) {
				message.destination = TX2Nav;
			} else if (message.messageType == KillMessage) {
				// kill received
				killMessageReceived = 1;
				break;
			} else if (CommandMessage == message.messageType && 
				   message.destination == TX2Master &&
				   message.source == TX2Nav) {
				// nav is done with current command, pop the queue
				printf("\n\nPOPING COMMAND QUEUE\n\n");
				messageOkToSend = GetNextCommand(&message);
				// if we don't need to send another command, continue so there is no write
				if (!messageOkToSend) {
					continue;
				}	
			} else if (CommandMessage == message.messageType && message.source == TX2Comm) {
				// this is where we either Create, Update, or Delete a command. Incoming
				// messages from the comm node are interpreted here
				switch (message.cmdMsg.commandOperation) {
					case Create:
						InsertCommand(&message.cmdMsg, &message);
						break;
					case Update:
						// this will be implemented as an update and delete
						break;
					case Delete:
						DeleteCommand(&message.cmdMsg, &message);
						break;
					default:
						printf("unkown command message operation");
						break;
				}
				PrintCommands();
				// if we don't need to write the new command to the nav node, continue
				if (message.destination != TX2Nav) {
					continue;
				}
			}

			// send message to destination
			write(writePipes[message.destination], &message, sizeof(message));
		}
	}


	printf("killing child processes\n");

	// loop through each child process and send kill message
	for (i = 0; i < CHILD_COUNT; i++) {
		memset(&message, 0, sizeof(message));
		message.messageType = KillMessage;
		write(writePipes[i], &message, sizeof(message));
		printf("closing %d\n", i);
		close(readPipes[i]);
		close(writePipes[i]);
	}	

	printf("master signing off...\n");

	return 0;
}
