/**
 * @file Command.c
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Function definition file for Command library.
 * @details Function definitions file for Command library. This file also contains hidden global variables to 
 *	    be used by the following functions.
**/

#include "../include/Command.h"

/**
 * @brief Head pointer for the linked list queue of #CommandNode nodes.
**/
CommandNode * commandHead = NULL;

/**
 * @brief Keeps track of the command id that will be given to the next newest node
**/
unsigned long nextCommandId = 1;

/**
 * used for the likely scenario that the user sends a command to place new command
 * after the currently executing command, and the currently executing command finishes
 * before the new command arrives
**/
unsigned long lastCommandExecuted = 0;

/**
 * @brief Internal function used to allocate memory for a new command node.
 * @details Internal function used to allocate memory for a new command node. This call also
 *	    initializes the node via attributes provided by the #CmdMsg and #CommandNode pointers.
**/
CommandNode * CreateNewCommandNode(CmdMsg * cmdMsg,
			           CommandNode * nextCommand)
{
	CommandNode * temp;

	// allocate the memory
	temp = malloc(sizeof(CommandNode));

	// initialize the values
	temp->commandId = nextCommandId++;
	temp->commandType = cmdMsg->commandType;
	temp->position.latitude = cmdMsg->position.latitude;
	temp->position.longitude = cmdMsg->position.longitude;
	temp->nextCommand = nextCommand;
	
	return temp;
}

int InsertCommand(CmdMsg * cmdMsg, Message * message)
{
	CommandNode * temp;
	int insertionType = HEAD_INSERT;
	unsigned int currentPosition = 1;

	// if list is empty or the command we want to place the new command at was just popped
	if (NULL == commandHead || lastCommandExecuted == cmdMsg->previousCommandId) {
		commandHead = CreateNewCommandNode(cmdMsg, commandHead);
	} else {
		// we need to traverse the list and figure out where the new node needs to go
		temp = commandHead;
		while (NULL != temp->nextCommand && temp->commandId != cmdMsg->previousCommandId) {
			temp = temp->nextCommand;
		}
		
		// insert the command
		temp->nextCommand = CreateNewCommandNode(cmdMsg, temp->nextCommand);
		insertionType = NON_HEAD_INSERT;
	}

	// if this was a head insertion, we need to marshal the necessary parameters into the #Message
	// struct pointer that was passed in.
	if (HEAD_INSERT == insertionType) {
		message->messageType = PositionMessage;
		message->source = TX2Master;
		message->destination = TX2Nav;
		message->positionMsg.position.latitude = commandHead->position.latitude;
		message->positionMsg.position.longitude = commandHead->position.longitude;
	}

	return insertionType;
}

int GetNextCommand(Message * message)
{
	CommandNode * temp;

	// is list empty?
	if (NULL == commandHead) {
		printf("\n\nCOMMAND LIST EMPTY\n\n");
		message->destination = TX2Master;
		return 0;
	} else {
		// list is not empty, as this implements a queue, just pop a command off the top
		// incremnt head
		temp = commandHead;
		commandHead = commandHead->nextCommand;

		// set the previous command
		lastCommandExecuted = temp->commandId;

		// free memory from the previous command
		free(temp);

		message->source = TX2Master;

		// as the head of this queue is the command currently in execution, figure out if
		// there is another command available, if so marshall the #Message struct pointer
		// and return 1.
		if (NULL != commandHead) {
			if (PositionCommand == commandHead->commandType) {
				message->destination = TX2Nav;
				message->messageType = PositionMessage;

				message->positionMsg.position.latitude = commandHead->position.latitude;
				message->positionMsg.position.longitude = commandHead->position.longitude;
			} else if (CameraCommand == commandHead->commandType) {
				message->destination = TX2Cam;
				message->messageType = CamMessage;
			}
			
			return 1;
		}
	}
	return 0;
}

int DeleteCommand(CmdMsg * cmdMsg, Message * message)
{
	CommandNode * temp;
	CommandNode * nodeToDelete;
	temp = commandHead;

	// traverse down the command queue until we either find the #CommandNode we are deleting, or
	// end up finding nothing at all.
	while (NULL != temp->nextCommand && cmdMsg->commandId != temp->nextCommand->commandId) {
		temp = temp->nextCommand;
	}

	// we found what we are looking for
	if (cmdMsg->commandId == temp->nextCommand->commandId) {
		// if head has changed, notify nav of the new destination
		// this will need to be modified to account for camera functionality
		if (temp == commandHead) {
			message->destination = TX2Nav;
			message->source = TX2Master;
			message->messageType = PositionMessage;
			message->positionMsg.position.latitude = commandHead->nextCommand->position.latitude;
			message->positionMsg.position.longitude = commandHead->nextCommand->position.longitude;
		}

		// rearrrange pionters and free memory
		nodeToDelete = temp->nextCommand;
		temp->nextCommand = temp->nextCommand->nextCommand;
		free(nodeToDelete);
	}
}

void FlushCommands()
{
	CommandNode * temp;

	// go through entirl queue and delete
	while (NULL != commandHead) {
		temp = commandHead;
		commandHead = commandHead->nextCommand;
		free(temp);
	}

	// reset to initial values
	lastCommandExecuted = 0;
	nextCommandId = 1;
}

void PrintCommands()
{
	CommandNode * temp;
	temp = commandHead;

	// traverse through queue, print each #CommandNode that we come across
	while (temp != NULL) {
		printf("commandId = %ld\n", temp->commandId);
		printf("lat %f lon %f\n\n", temp->position.latitude, temp->position.longitude);
		temp = temp->nextCommand;
	}
}
