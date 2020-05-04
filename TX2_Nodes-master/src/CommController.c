#include "../include/CommController.h"

#define BUFFER_SIZE 4096 

char imageBuffer[BUFFER_SIZE]; /**< image buffer, used for transmitting image */

int Port; /**< The port which the TCP socket is connected to. */

int TCPSocket; /**< TCP Socket used to communicate with outside 
		    world. */
int SetupSocket; /**< Socket used to listen for connection request. */

struct sockaddr_in address; /**< Address socket is bound to. */

int addressLength; /**< Stores address length. */

int opt; /**< Used when setting socket options. */

int brokenConnection; /**< Flag set if connection to client is broken */

/**
 * @brief Function handles a SIGPIPE signal.
 * @details Function handles a SIGPIPE signal, generated when checking the TCPSocket
 *	    to determine if connection is still open.
 * @pre None.
 * @post Sets #brokenConnection to 1, notifying the process that a new connection
 *	 needs to be established.
 */
void SIGPIPEHandler()
{
	brokenConnection = 1;
	printf("client has disconnected\n");
}

/**
 * @brief Function establishes connection with client.
 * @details In the scenario that the connection with the client has been lost,
 * 	    this private function is called by #SocketCheck() to reestablish
 * 	    the connection with a new socket.
 * @pre None
 * @post If #SocketCheck() determines that the socket connection was lost, this
 * 	 function will be called and in the event the client tries to reconnect,
 * 	 a new socket will be created to reestablish the conection.
 */
int EstablishSocket()
{
	// if a socket already exists, close it
	if (TCPSocket > 0)
	{
		close(TCPSocket);
	}

	// wait here until a new request comes in.
	while ((TCPSocket = accept(SetupSocket, (struct sockaddr *) &address, 
					  (socklen_t *) &addressLength)) < 0);
	printf("client connected.\n");
	brokenConnection = 0;

	// test, remove if necessary
	setsockopt(TCPSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

	return TCPSocket;
}

int InitializeComm(int port)
{
	Port = port;
	TCPSocket = 0;
	opt = 1;

	// setup for SIGPIPE signal, not really usedd
	struct sigaction sigPipe;
	sigPipe.sa_handler = SIGPIPEHandler;
	sigPipe.sa_flags = 0;

	sigaction(SIGPIPE, &sigPipe, NULL);

	addressLength = sizeof(address);

	//  create listenr socket
	//  SOCK_STREAM = TCP
	//  SOCK_DGRAM = UPD
	if ((SetupSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		printf("Failed to create INET Socket.\n");
		return -1;
	}

	//  normally, you cannot use the same port twice on a socket, this allows
	//  us to force the socket to listen/talk on port 8080. Might look into
	//  other port later on if this becomes an issue.
	if (setsockopt(SetupSocket, SOL_SOCKET, ADD_POR_REUSE, &opt, sizeof(opt)))
	{
		printf("Failed to set socket options.\n");
		return -1;
	}

	// set socket as no delay
	setsockopt(SetupSocket, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

	// setup port information, what addresses we want to accept
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(Port); //  host to network byte order

	// bind socket
	if (bind(SetupSocket, (struct sockaddr *) &address, sizeof(address)) < 0)
	{
		printf("Failed to bind socket.\n");
		return -1;
	}

	// set #SetupSocket to listen
	listen(SetupSocket, 1); //  second argument is que for number of connections

	// block until request has been made from controller/client
	EstablishSocket();

	return TCPSocket;
}

int CommRead(Message * message)
{
	return read(TCPSocket, message, sizeof(*message));
}

int CommWrite(Message * message)
{
	return write(TCPSocket, message, sizeof(*message));
}

int CommImageWrite(Message * message)
{
	int imageFile;
	int remainingBytes;
	struct stat statbuf;
	int bytesToSend = BUFFER_SIZE;
	int segment = 0;

	// open up the image file
	imageFile = open(message->camMsg.fileLocation, O_RDONLY);
	
	// we are extracting the size of the image, to let the receiver know
	// how many bytes to accept
	fstat(imageFile, &statbuf);
	message->camMsg.fileSize = remainingBytes = statbuf.st_size;

	//  send the first message to client, signaling file transfer
	write(TCPSocket, message, sizeof(Message));
	printf("notified controller of image\n");

	if (imageFile < 0 )
	{
		printf("error opening image file\n");
		return 0;
	}

	// loop through here until entire image has been sent. This loop reads
	// through the image in chunks of size BUFFER_SIZE.
	while (remainingBytes > 0)
	{
		bytesToSend = read(imageFile, imageBuffer, BUFFER_SIZE);
		write(TCPSocket, imageBuffer, bytesToSend);
		remainingBytes -= bytesToSend;
	} 
	
	// close the file
	close(imageFile);

	printf("file sent\n");
}

int SocketCheck()
{
	// marshall socket check message for client. Write it out to the TCP socket
	Message message = {.messageType = OKMessage, .okMsg.message = "Luna Bun"};
	write(TCPSocket, &message, sizeof(message));
	if (brokenConnection)
	{
		printf("client has disconnected\n");
		return -1;
	}
	return TCPSocket;
}

void CloseSocket()
{
	// close both TCP and setup/listening socket.
	close(TCPSocket);
	close(SetupSocket);
}
