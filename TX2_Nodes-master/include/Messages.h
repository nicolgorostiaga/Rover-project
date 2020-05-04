/**
 * @file Messages.h
 * @author Patrick Henz
 * @date 4-28-2019
 * @brief Header file for Messages library.
 * @details Header file for Messages library. #Messages.h provides a convenient
 * 	    message struct that contains a union of all possible messages that
 * 	    can be passed around in the system. This library also provides wrapper
 *	    functions for the pselect() function (see select man page for additional
 *	    information). Originally, a process would be paused and signals were used
 *	    to wake a process up to read in a new message. That is NOT a good way to 
 *	    multiple processes communicating with one another, especially if communication
 *	    is asynchronous. If there is an incoming signal while a process is in a
 *	    signal handler, the new signal is lost. I didn't find a convenient wawy around
 *          this. Also, syscalls like read() and write() don't like being interrupted.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h> 
#include <sys/socket.h>
#include <sys/uio.h> 
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Enums used to diffrentiate between TX2 nodes.
 * @details NodeName enums are used to differentiate between TX2 nodes. These values are hardcoded 
 *	    as they are used as the routing mechanism (array) by tx2_master.c. 
 *	    For a node to send a message to a specific node, it simply sets the 
 *	    destination attribute in #Message to the enum and tx2_master.c handles 
 *	    the routing. This enum is also used in the source attribute of 
 * 	    #Message, as a way to further interpret an incoming message.
**/
typedef enum nodeNames {
	TX2Comm    = 0,
	TX2Can     = 1,
	TX2Cam     = 2,
	TX2Nav     = 3,
	TX2Gps     = 4,
	TX2Gyro    = 5,
	Controller = 6,
	TX2Master  = 7
} NodeName; 

/**
 * @brief Message type enums.
 * @details Message type enums used in the #Message struct. These enums are used as a way to further
 *          differentiate incoming messages so a node can properly interpret the message.
**/
typedef enum messageTypes {
	CANMessage,			// message for or from CAN node
	CamMessage,			// used for taking images and sending back to client
	PositionMessage,		// message contains direction/positional information
	OKMessage,			// ping sent to controller to check socket
	ClientDisconnect,		// client disconnect message
	SharedMemory,			// shared memory, used by multiple nodes
	OperationMode,			// notifies receiver to check OpModeMsg struct in Message struct
	ParametersMessage,              // signal sent from controller to reload Parameters (Nav node)
	KillMessage,			// kill message sent from controller
	CalibrationCompleteMessage,
	CommandMessage,			// tells master to interpret Message as CmdMsg
	GyroMessage			// request for Gyro node to start collecting samples
} MessageTypes; 


/////////////////////////////
//  Message Union/Structs  //
/////////////////////////////

// The following structs are contained within a union in the Message struct. This provides a 
// convenient way of passing the same data structure around while being able to interpret that
// data structure in many different ways. The routing mechanism, TX2Master, doesn't need to do
// any interpretation, it's really only at the end points of commincation that interpretation
// happens.

/**
 * @brief Operation states used by the navigation node.
 * @details Operation states used by the navigation node. Also used by controller.c as a means to 
 *	    toggle behavior of the tx2_navigation_node.c. Contained in #OpModeMsg struct in #Message
 *	    union.
**/
typedef enum opMode
{
	Automatic,  // use semantic seg data to navigate
	Manual      // use manual user input
} OpMode;


/**
 * @brief Struct containging #OpMode enum.
 * @details This struct holds the #OpMode enum. This union member of #Message should be checked if 
 *	    #OperationMode is set as the #MessageType.
**/
typedef struct opModeMsg {
	OpMode opMode;
} OpModeMsg;

/**
 * @brief Struct that holds CAN messages.
 * @details Struct holds CAN messages, either too or from tx2_can_node.c.
**/
typedef struct canMsg {
	int SId;			// CAN SId
	int Bytes; 			// number of bytes in Message array
	unsigned char Message[8];	// data
	int writeCount;		        // this contains the number of times a CAN 
				   	// message is to be written. Used only for turning commands.
} CanMsg;

/**
 * @brief Struct that holds camera messages.
 * @details Struct holds camera messages. This is primarily used as a means for controller.c to 
 *	    notify tx2_cam_node.cpp to take an image, save it to disk, and send a message to 
 *	    CommController.c to read in the new image (via fileLocation) and transmit the image back
 *	    to controller.c. 
**/
typedef struct camMsg {
	int ready;
	int fileSize;
	char fileLocation[32];
} CamMsg;

typedef struct shMem {
	int width;
	int height;
} ShMem; /**< Shared memory struct for Cam-to-Nav node. Used to give Nav node dimensions of shared memory. */

/////////////////////////////////////////////////
//	GPS RELATED TYPEDEFS TODO	       //
/////////////////////////////////////////////////

/**
 * @brief Latitude and Longitude positional coordinate.
**/
typedef struct _Position {
	float latitude;
	float longitude;
} Position;

/**
 * @brief Struct containing #Position struct.
**/
typedef struct positionMsg {
	Position position;
} PositionMsg; 

/**
 *  @brief Struct used to keep track of #Position information.
 *  @details This struct is primarily used to keep track of #Position coordinates; latitude 
 *  	     and longitude. As the GNSS module (XA1110) is capable of returning additional 
 *  	     information, this struct contains some additional members in the scenario that
 *  	     this information is desired.
**/
typedef struct gpsMsg {
	Position position;
	float heading;
	float velocity;
	unsigned int time;
} GpsMsg; 

/////////////////////////////////////////////////
//	Command List Typedefs TODO	       //
/////////////////////////////////////////////////

/**
 * @brief Enums used to define the operations to be performed on the #CommandNode linked list
 * @details The CommandOperation enum is used to distinguish between the different opeartions
 * 	    that can be performed on the #CommandNode linked list maintained by tx2_master.c.
 * 	    Gives the ability to create, delete, update stored commands, or flush the list 
 * 	    entirley. Though the majority of this functionality exists, it hasn't been fully 
 * 	    implemented. Please check Command.h and Command.c for additional info.
**/
typedef enum _CommandOperation {
	Create,
        Delete,
        Update,
	Flush
} CommandOperation; 

/**
 * @brief Enums used to define the type of command in #CommandNode.
 * @details These enums are to be used to define the type of command to be performed. Position 
 * 	    command tells master to send #CommandNode info to tx2_nav_node.c, CameraCommand would
 * 	    be for tx2_cam_node.cpp. Cam node commands have not been fully implemented.
**/
typedef enum _CommandType {
	PositionCommand,
	CameraCommand
} CommandType; 

/**
 * @brief Struct contains all the required info for performing opeartions on the #CommandNode 
 * 	  command queue.
 * @details This struct contains information to be sent from controller.c to the rover. All info
 * 	    needed to create, delete, and/or update a #CommandNode element in the command queue
 * 	    should be in this struct. Also should provide ability to flush queue entirely.
**/
typedef struct _CmdMsg {
	unsigned long commandId;
	CommandType commandType;
	CommandOperation commandOperation;
	unsigned long previousCommandId;
	Position position;
} CmdMsg; 

/**
 * @brief Struct used by tx2_comm_node.c when checking socket connection with controller.c.
 * @details This struct is used by tx2_comm_node.c and CommController.c when checking socket 
 * 	    connection with controller.c. The message itself is arbitray, as the #MessateType is
 * 	    primarily used by logWriter.c to return to the rover that the connection still holds.
**/
typedef struct okMsg {
	char message[32];
} OkMsg; 

/**
 * @brief The struct used by all nodes to communicate with one another.
 * @details The Message struct is the main struct used by nodes communicating with one another. 
 * 	    Message is even used by controller.c and logWriter.c when communication with the rover
 * 	    takes place.
 * 	    <br>
 * 	    <br>
 * 	    The struct contains three attributes that are common amongst all interpretations;
 * 	    #MessageTypes messageType, #NodeName source, and #NodeName destination. tx2_master.c
 * 	    uses destination to route messages to the correct TX2 node, and source is often used
 * 	    as a way to further interpret a message when it reaches its destination. 
 * 	    <br>
 * 	    <br>
 * 	    The rest of the message struct consists of a union of structs, all of which are defined
 * 	    above in this file. As pipes are used by all nodes to communicate with one another, this
 * 	    provides an easy and convenient way for nodes to pass equal sized messages arround 
 * 	    without needing to worry about the contents of the message itself. The receiving node
 * 	    simply uses the 3 non-changing attributes to determine how to interpret the incoming
 * 	    message. This also makes it easy to add additional functionality at a later date
 * 	    without the need to perform a drastic overhaul of other c files.
**/
typedef struct message {
	MessageTypes messageType;
	NodeName source;
	NodeName destination;
	union {
		PositionMsg positionMsg;
		CanMsg canMsg;
		CamMsg camMsg;
		ShMem shMem;
		OkMsg okMsg;
		OpModeMsg opModeMsg;
		GpsMsg gpsMsg;
		CmdMsg cmdMsg;
	};
} Message; 

/**
 * @brief SetupSetAndWait() helps support pselect() functionality.
 * @details This function helps support pselect() functionality. It stores a copy
 * 	    of the array which was passed as a parameter, determines which file
 *	    descriptor is the largest, and sets gets the process ready to call pselect().
 * @param fd A pointer to the array of file descriptors that will be read from.
 * @param count The number of file descriptors in fd.
 * @pre Assumes fd has been populated correctly and count matches the number of values in fd.
 * @post Setup is complete, the wrapper function #SetAndWait() can now be called.
**/
void SetupSetAndWait(int * fd, int count);

/**
 * @brief Overwrites old file descriptor with new file descriptor for #SetupAndWait().
 * @details Overwrites old file descriptor with new file descriptor for #SetupAndWait(). 
 *	    in the scenario that a new file descriptor is needed to replace an old one,
 *          i.e. the tx2_comm_node needing to establish a new socket with controller, this
 *	    function overwrites the old file descriptor sets the max file descriptor if the
 *	    new one is larger. 
 *	    <br>
 *	    <center><b>NOTE: This does overwrite the array passed in whe #SetupSetAndWait() is
 *	    first called. Just be aware of this</b></center>
 * @param oldFd The old file descriptor to be replaced.
 * @param newFd The new file descriptor.
 * @return Returns -1 if the old file descriptor wasn't found. Returns 0 if success.
 * @pre Expects #SetupSetAndWait() to have been called prior to ModifySetAndWait(). 
 * @post The old file descriptor will have been replaced with the new file descriptor.
**/
int ModifySetAndWait(int oldFd, int newFd);

/**
 * @brief Wrapper function which calls pselect().
 * @details SetAndWait() performs some basic setup before calling pselect(). This function
 *	    returns under 2 conditions; a file descriptor is available to be read, or a timeout.
 * @param rdfs The address of the fd_set read file descriptors variable.
 * @param seconds pselect() timeout in seconds.
 * @param nanoSeconds pselect() timeout in nano seconds.
 * @return The return value is the same value returned from pselect(). If a timeout occurs,
 *	   no file descriptors are available and 0 is returned. If file descriptors are 
 *	   available, the number of file descriptors is returned. -1 is returned on error.
 * @pre Assumes #SetupSetAndWait() was called prior to calling SetAndWait().
**/
int SetAndWait(fd_set * rdfs, long seconds, long nanoSeconds);

#endif
