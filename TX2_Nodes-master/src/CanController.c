/**
 * @file CanController.c
 * @author Patrick Henz
 * @date 12-2-2019
 * @brief File contains function definitions and global variable definitions for CanController library.
 * @details CanController.c contains function definitions and global variable definitions for the 
 *          CanController library. For a detailed higher level description of these functions, please see
 *	    CanController.h.
 *          
**/

#include "../include/CanController.h"

int CanSocket; /**< File descriptor for open CAN socket */

/**
 * Socket Address private member
 */
struct sockaddr_can addr; //  /usr/include/linux/can.h

/**
 * Interface Request private member, assigned to #addr member
 */
struct ifreq ifr; //  /usr/include/net/if.h

/**
 * @brief An internal function used to restart CAN controller.
 * @details RestartCan() is an internally used function by the CanController library. When this
 *	    function is called the can modules are removed before being re-loaded into kernel. Take note that
 *	    CAN baud rate is also determined here.
 * @return Returns 0 if no error, returns -1 or value from other function call if error.
**/
int RestartCan()
{
	int status;
	int error = 0;

	// remove mttcan module
	status = system("modprobe -r mttcan");
	if (status < 0)
	{
		printf("error unloading mttcan module\n");
		error = -1;
	}

	// load mttcan module
	status = system("modprobe mttcan");
	if (status < 0)
	{
		printf("error loading mttcan module\n");
		error = -1;
	}

	// couldn't find a better way to do this. Calling system is the same as running this in
	// the command line. It isn't ideal but works. This also sets the CAN baud rate.
	status = system("ip link set can0 type can tq 250 prop-seg 5 phase-seg1 6 phase-seg2 4 sjw 1 restart-ms 100");
	if (status < 0)
	{
		printf("error during ip link\n");
		error = -1;
	}

	// setup can0 for use
	status = system("ip link set up can0");
	if (status < 0)
	{
		printf("error during ip link setup\n");
		error = -1;
	}

	return error;
}

/**
 * @brief Internal function used to dynamically load kernel modules during runtime. 
 * @details This internal function is used by the CanController library to dynamically load
 *	    kernel modules during CAN initialization. I initially wanted to find a better way to
 *	    do this without having to rely on system(), but as it was functional I left as is.
 * @return Returns 0 if no error. Returns -1, or value set by other function call, if error is present.
**/
int LoadModules()
{
	int status;
	int error = 0;
	
	// open can module
	int canFd = open("/lib/modules/4.4.38-tegra/kernel/net/can/can.ko", O_RDONLY);

	// use system call macro defined in CanController.h to load CAN module
	status = finit_module(canFd, "", 0);
	if (status < 0)
	{
		printf("error loading can module\n");
		printf("attemping can restart\n");
		status = RestartCan();
		if (status == 0)
		{
			printf("restart successful\n");
			return 0;
		}
		else
		{
			printf("restart unsuccessful\n");
		}
	}

	// load can raw module
	int canRawFd = open("/lib/modules/4.4.38-tegra/kernel/net/can/can-raw.ko", O_RDONLY);
	status = finit_module(canRawFd, "", 0);
	if (status < 0)
	{
		printf("error loading can-raw module\n");
		error = -1;
	}

	// load mttcan module
	status = system("modprobe mttcan");
	if (status < 0)
	{
		printf("error loading mttcan module\n");
	}

	// setup CAN characteristics. This is the same call that is made in #RestartCan(). Note that
	// this is also where CAN baud rate information is set. TQ and the size of each segment.
	status = system("ip link set can0 type can tq 250 prop-seg 5 phase-seg1 6 phase-seg2 4 sjw 1");
	if (status < 0)
	{
		printf("error during ip link\n");
		error = -1;
	}

	// finish setting up CAN
	status = system("ip link set up can0");
	if (status < 0)
	{
		printf("error during ip link setup\n");
		error = -1;
	}

	return error;
}

// Initialize socket can, return the file descriptor if succesful, esle return -1.
int InitializeCan()
{
	printf("Initializing CAN controller\n");

	// dynamically load the kernel modules
	LoadModules();

	//  opens socket
	if ((CanSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
	{
		printf("error openning socket.\n");
		return -1;
	}

	//  clear everything, setup socket CAN attributes
	memset(&addr, 0, sizeof(addr));
	memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));

	strncpy(ifr.ifr_name, "can0", 4);
	ifr.ifr_name[4] = '\0';
	ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	// setup error mask, not really used	
	can_err_mask_t err_mask = CAN_ERR_MASK;
	setsockopt(CanSocket, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask, sizeof(err_mask));

	
	// when socket created, exists only in namespace. This assigns socket to addr.
	if (bind(CanSocket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		printf("bind error.\n");
		return -1;
	} 

	return CanSocket;
}

int CanRead(Message * message)
{
	struct can_frame frame; //  /usr/include/linux/can.h
	struct iovec iov; //  /usr/include/aarch64-linux-gnu/bits/uio.h
	struct msghdr receivedMessage; //  /usr/include/aarch64-linux-gnu/bits/socket.h

	// craete control message
	char ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3*sizeof(struct timespec) + sizeof(__u32))];

	memset(&frame, 0, sizeof(frame));
	//  setup for msg, when recvmsg is called.
	//  these structs will contain the CAN message after
	//  it has been read in from the bus. 
	iov.iov_base = &frame;
	receivedMessage.msg_name = &addr;
	receivedMessage.msg_iov = &iov;
	receivedMessage.msg_iovlen = 1;
	receivedMessage.msg_control = &ctrlmsg;

	iov.iov_len = sizeof(frame);
	receivedMessage.msg_namelen = sizeof(addr);
	receivedMessage.msg_controllen = sizeof(ctrlmsg);
	receivedMessage.msg_flags = 0;

	// read in the CAN data
	recvmsg(CanSocket, &receivedMessage, 0);

	// since CAN data is someone burried within the above structs, we need to 
	// extract this data and put it in our #Message struct
	// get the SID
	message->canMsg.SId = frame.can_id;

	// copy the data over
	memcpy(message->canMsg.Message, frame.data, frame.can_dlc);

	// set the number of bytes in the message we read in
	message -> canMsg.Bytes = frame.can_dlc;

	// check if there was an error, print out error if necessary
	if (frame.can_id & CAN_ERR_FLAG)
	{
		printf("CAN error\n");
	}

	// set #MessateType as #CANMessage
	message -> messageType = CANMessage;

	return message->canMsg.Bytes;	
}

int CanWrite(Message * message)
{
	int status;
	struct can_frame frame; //  /usr/include/linux/can.h

	memset(&frame, 0, sizeof(frame));

	// setup the struct with the CAN data contained in the #Message struct
	frame.can_id = message -> canMsg.SId;
	frame.can_dlc = message -> canMsg.Bytes;
	// copy over the payload
	memcpy(&frame.data, &(message -> canMsg.Message), message -> canMsg.Bytes);

	// write the data to the CAN bus
	status = write(CanSocket, &frame, CAN_MTU);

	// check status
	if (status < 0)
	{
		printf("CAN socket error, restarting\n");
	}
	return status;
}

void CloseCan()
{
	close(CanSocket);
}
