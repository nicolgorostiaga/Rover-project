/**
 * @file logWriter.c
 * @author Patrick Henz
 * @date 12-12-2019
 * @brief logWriter process.
 * @details The logWriter process is created by controller.c to essentially
 * 	    print incoming data from the TX2 to the screen. It also handles
 * 	    incoming image data and saves to disk.
**/

#include <stdio.h> 
#include <sys/socket.h> 
#include <sys/stat.h>
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <string.h> 
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "include/Messages.h"

int sock;

#define BUFFER_SIZE 64

char imageBuffer[BUFFER_SIZE];

/**
 * @brief Function used to read data from TCP socket.
 * @details This function reads incoming data from the TX2.
 * 	    It prints certain messages to screen, and if there
 * 	    is an incoming image it saves it to disk.
**/
void ReadFromSocket()
{
	int i;
	Message messageIn;
	// read the message
	read(sock, &messageIn, sizeof(messageIn));

	// if CAN message, print to screen
	if (messageIn.messageType == CANMessage)
	{
		printf("\rCAN Message - ");
		printf("SId %X - ", messageIn.canMsg.SId);
		for (i = 0; i < messageIn.canMsg.Bytes; i++)
			printf("%X", messageIn.canMsg.Message[i]);
		printf("\n\n\r");
	} 
	else if (messageIn.messageType == CamMessage)
	{
		// incoming message indicates an image is coming, prep for it
		printf("\n\rReceiving image..\n");
		char fileName[32];
                char command[15];
		int bytesToRead = BUFFER_SIZE;
		int bytesRemaining = messageIn.camMsg.fileSize;

		// indexing to 3 to account for "../" in relative file path
		memcpy(fileName, &messageIn.camMsg.fileLocation[3], 18);

		// create the image
		int imageFile = open(fileName, O_RDWR | O_CREAT);		

		if (imageFile <= 0) 
		{
			printf("error creating image\n");
		}

		printf("\n\rwriting file %s\n", fileName);

		// read in the image, save to disk
		while (bytesRemaining > 0)
		{
			bytesToRead = read(sock, imageBuffer, BUFFER_SIZE);
			write(imageFile, imageBuffer, bytesToRead);
			bytesRemaining -= bytesToRead;
		}

		// changge file permissions
		fchmod(imageFile, 444);
		close(imageFile);
		printf("\n\rFile received.\n\r");
	}
	else if (messageIn.messageType == OKMessage)
	{
		// result of socket check, tell comm node we are A-OK.
		write(sock, &messageIn, sizeof(messageIn));
	}
}

int main(int argc, char ** argv)
{
	fd_set rdfs;
	int readFds[1];
	sock = atoi(argv[1]);
	readFds[0] = sock;

	// initialize SetAndWait
	SetupSetAndWait(readFds, 1);

	while(1)
	{
		// wait for incoming message
		if (SetAndWait(&rdfs, 1, 0) < 0) {
			printf("error\n");
		}

		// if there is a message, read it
		if (FD_ISSET(sock, &rdfs)) {
			ReadFromSocket();
		}
	}
}
