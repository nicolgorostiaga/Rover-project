/**
 * @author Patrick Henz
 * @file Command.h
 * @date 11/20/2019
 * @brief Definitions for the Command library
 * @details This header file contains definitions for the Command library. This library 
 * 	    is essentially a linked list that will be controlled by the master node. The
 * 	    elements of this linked list form a queue of commands, with the element at the 
 * 	    head of the list being the command that is currently in execution.
**/

#ifndef COMMAND_H
#define COMMAND_H

#include "Messages.h"

/**
 * @brief Value returned by #InsertCommand() if new node is at head of queue.
 * @details Value returned by #InsertCommand() if new node is at head of queue. As the head of the
 * 	    queue contains the command that is currently in execution, this lets master know that
 * 	    the updated information needs to be sent off to tx2_nav_node.c to update its
 * 	    destination. Or if a camera command to take a picture immediatley.
**/
#define HEAD_INSERT 1
/**
 * @brief Value returned by #InsertCommand() if new node is not at head of queue.
 * @details Value returned by #InsertCommand() if new node is not at head of queue. If this is the
 * 	    case, tx2_master.c doens't need to send any information new information to a node.
**/
#define NON_HEAD_INSERT 0

/**
 * @brief Typedef allows for declaration within #_CommandNode struct for linked list functionality.
**/
typedef struct _CommandNode CommandNode;

/**
 * @brief Struct for node in command linked list maintained by tx2_master.c.
 * @detail This structc contains the information needed for tx2_master.c to maintain a 
 * 	   linked list queue of commands. Each command has its own unique id, the type of command
 * 	   which is defined in Messages.h #CommandType, the #Position if it is a positional command,
 * 	   and a pointer to the next commandin the queue.
**/
struct _CommandNode {
	unsigned long commandId;
	CommandType commandType;
	Position position;
	CommandNode * nextCommand;
}; 

/**
 * @brief Function inserts a new command into the command queue.
 * @details InsertCommand() inserts a new command into the command queue. The #cmdMsg parameter
 *	    defines the command itself and #message is used in the scenario that the command was
 *	    inserted at the head of the list and a message needs to be generated.
 * @param cmdMsg This is a pointer to a #CmdMsg struct that defines the command that is being
 *	  	 inserted into the command queue.
 * @param message This is a pointer to a #Message struct, used as an output. If the new command
 *		  is inserted at the head of the linked list, message will be filled with any
 *	          relevant information needed to execute the command.
 * @post A new command node will have been inserted into the command queue. In the scenario that
 *	 the new node is at the head of the list, #message will be populated with all required info.
 * @return This function returns the value of one of the two defined macros; #HEAD_INSERT or #NON_HEAD_INSERT. 
 *	   This is used to notify the caller, in this case #tx2_master.c, if the message parameter needs to
 *	   be sent off to another TX2 node. 
**/
int InsertCommand(CmdMsg * cmdMsg, Message * message);

/**
 * @brief Function gets the next command in the command queue.
 * @details GetNextCommand() gets the next command in the queue. As the node at the head of the queue
 *	    is the command currently in execution, calling this function will pop the previous command
 *	    from the queue and free the memory allocated for the previous node. The next node in the
 *	    queue is promoted to the head of the list.
 * @param message This a pointer to a #Message struct. If a new command is present in the list, the
 *	          relevant information will be populated in message.
 * @return Returns a 1 in the scenario that a command was available and the #message parameter has been
 *	   populated with the relevant information. A 0 is returned in the case that no command was in the list.
**/
int GetNextCommand(Message * message);

/**
 * @brief Deletes a specific command node in the queue.
 * @details DeleteCommand() deletes a specific node from the command queue. 
 * @param cmdMsg This is a pointer to a #CmdMsg struct, whose commandId is used to determine which node to delete.
 * @param message In the event that the first node in the list was deleted, the #Message struct needs to be populated
 *	          with the next commands information. TODO: this functionality has not been fully implemented. The 
 *		  mechanism to correctly locate/free the required command node and to re-arranged the queue as needed is in
 *		  place. However, the function needs to be configured to return a value to let the caller know if any
 *		  further actions are required. TODO
**/
int DeleteCommand(CmdMsg * cmdMsg, Message * message);

/**
 * @brief Flushes the command queue.
 * @details FlushCommands() flushes the command queue. It removes each command node from the queue and frees the memory used
 *	    for each node created.
**/
void FlushCommands();

// function used for prototyping. No real purpose apart from testing.
void PrintCommands();

#endif
