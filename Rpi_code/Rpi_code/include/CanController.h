/**
 * @file CanController.h
 * @author Patrick Henz
 * @date 4-17-2019
 * @brief Header file for CanController.c. This implements all CAN functionality.
 * @details After calling #InitializeCan(), all other functions are ready to use.
 *
 * 	    --------------------
 *          ---USEFUL HEADERS---
 *          --------------------
 *
 *	    /usr/include/linux/can.h
 *	    	- canfd_frame (struct), also can_frame, fd for flexible data rate
 *	    	- sockaddr_can (struct) sockaddr for CAN sockets
 *	    	- can_filter (struct)  //  NEEDS TO BE IMPLEMENTED ON TX2
 *		
 *	    /usr/include/aarch64-linux-gnu/bits/socket.h
 *	    	- msghdr (struct) message structure
 *
 *	    /usr/include/aarch64-linux-gnu/bits/uio.h
 *	    	- iovec (struct) io vector
 *		
 *	    /usr/include/net/if.h
 *	    	- ifreq (struct) interface request
 */

#ifndef CAN_CONTROLLER
#define CAN_CONTROLLER

#define DEBUG

#include "Messages.h"
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h> 
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/uio.h> 
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Macor used to initialize CAN module. Used by CanController.c
**/
#define finit_module(fd, param_values, flags) syscall(__NR_finit_module, fd, param_values, flags)
/**
 * @brief Macor used to remove CAN module. Used by CanController.c
**/
#define remov_module(name, flags) syscall(__NR_delete_module, name, flags)

#define PREV_MSG_SID 0x002

/**
 * @brief Function initializes a CAN socket.
 * @details Function initializes a CAN socket that can be read/written from/to.
 * @return Returns the file descriptor for the CAN socket.
 * @pre Assumes that proper CAN modules have been loaded into OS kernel.
 * @post CAN functionality is initialized and messages can be sent
 *	     and received on the CAN bus.
 */
int InitializeCan();

/**
 * @breif Function reads message that was sent over CAN bus.
 * @details Function reads message that was sent over CAN bus.
 * @param message This function must be provided with a #Message
 *	  struct address. The SId, Message, and Number of Bytes of
 *	  the received message will be written into this struct.
 * @return The number of bytes read.
 * @pre	A #Message struct has been ceated its address has been passed to this function.
 * @post The incoming message has been written to the #Message struct.
 */
int CanRead(Message * message);

/**
 * @breif Function writes a message over CAN bus.
 * @details Function writes a message over CAN bus.
 * @param message This function must be provided with a #Message
 *	  struct address. The SId, Message, and Number of Bytes of
 *	  the message being sent must be provided prior to call.
 * @return The number of bytes written.
 * @pre	A #Message struct has been ceated and passed to this
 *      function.
 * @post The data in the #Message struct will have been written
 *	 to the CAN bus.
**/
int CanWrite(Message * message);

/**
 * @brief Function closes CAN socket.
 * @details Function closes CAN socket.
 * @pre Expects #InitializeCan() to have been called first with an open CAN socket FD in memory.
 * @post The CAN socket will have been closed.
**/
void CloseCan();

#endif
