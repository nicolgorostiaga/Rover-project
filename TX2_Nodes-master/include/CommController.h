/**
 * @file CommController.h
 * @author Patrick Henz
 * @date 4-17-2019
 * @brief Header file for CommController library.
 * @details Header file for CommController. This implements TCP
 * 	    socket functionality. Used by the TX2 communication node to
 * 	    communicate with the outside world.
**/

#ifndef COMM_CONTROLLER
#define COMM_CONTROLLER

#define DEBUG

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include "Messages.h"

#define PORT 5000 /**< Port number used for communication */
#define ADD_POR_REUSE (SO_REUSEADDR | SO_REUSEPORT) /**< Macro for port/address 
							 reuse */

/**
 * @brief Initializes and returns a TCP socket after successfully establishing a connection.
 * @details This function fully initializes a socket and accepts a connection with a device
 * 	    on the network.
 * @param port The desired port for the socket.
 * @return Returns the TCP socket that can be read/written from/to.
 * @pre None.
 * @post The return socket is available to be written to via #CommRead() and #CommWrite().
 */
int InitializeComm(int port);

/**
 * @brief Function checks to see if socket connection still exists.
 * @details This function checks to see if the client device is still connected.
 * @return Returns -1 in the scenario the connection was lost.
 * @pre Assumes #InitializeComm() has been called.
 * @post Returns the current TCP fd if the connection still exists, returns -1 otherwise.
 */
int SocketCheck();

/**
 * @brief Function creates a new socket that can be read/written from/to.
 * @details This function creates a new socket that can be read/written from/to. Should
 * 	    only be used in the scenario that a connection was lost.
 * @return Returns the new TCP socket fd.
 * @pre Assumes #InitializeComm() has been called.
 * @post Returned socket can be read/written from/to via #CommRead() and #CommWrite();
 */
int EstablishSocket();

/**
 * @brief Function for reading from TCP socket.
 * @details Function for reading from TCP socket.
 * @param message #Message struct that the incoming message will be written to.
 * @return The number of bytes read.
 * @pre The address of an initialized #Message struct will have been passed in.
 * @post The #Message struct will contain the incoming message.
 */
int CommRead(Message * message);


/**
 * @brief Function for writing to TCP socket.
 * @details Function for writing to TCP socket.
 * @param message #Message struct that will be written to the TCP socket.
 * @return The number of bytes written.
 * @pre The address of an initialized #Message struct will have been passed in, must contain
 * 	the message that is being written to the TCP socket.
 * @post The data in the #Message struct will have been written to the TCP socket.
 */
int CommWrite(Message * message);

/**
 * @brief Function for writing image to TCP socket.
 * @details Function for writing image to TCP socket.
 * @param message #Message struct that will be written to the TCP socket.
 * @return The number of bytes written.
 * @pre The address of an initialized #Message struct will have been passed in, must contain
 * 	the message that is being written to the TCP socket.
 * @post The data in the #Message struct will have been written to the TCP socket.
 */
int CommImageWrite(Message * message);

/**
 * @brief Function checks if the client node is still connected.
 * @details Function checks if the client node is still connected. In the event that the client
 * 	    has disconnected, the host must call accept again to ensure a new connection is 
 * 	    created to reestablish communication. This function handles all of this. Should be
 * 	    called regularly. It is non blocking.
 * @pre None.
 * @post If the client has disconnected and tries to reconnect, the host will accept the new
 *       request and communication will have been reestablished.
 */
int SocketCheck();

/**
 * @brief Function closes TCP socket and listening socket.
 * @details Calling this function closes the TCP socket and listening socket. Only called
 * 	    when tx2_comm_node.c finishes execution.
**/
void CloseSocket();

#endif
