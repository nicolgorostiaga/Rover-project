/**
 * @file controller.c
 * @author Patrick Henz
 * @contributors Nicol Anokhin and Jeniffer Ordonez
 * @date 5-04-2020
 * @brief Program serves as a basic remote control for TX2 rover.
 * @details This program serves as a basic remote control for the TX2 rover through the use of tcp sockets. 
 *	    <br>
 *	    <br>
 *	    ---key strokes-- (case-insensitive)
 *	    W - forward
 *	    A - turn left
 *	    D - turn right
 *          S - backwards
 *          <br>
 *          <br>
**/

#include <stdio.h> 
#include <unistd.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h> 
#include <arpa/inet.h>
#include <string.h> 
#include <fcntl.h>
#include <errno.h>
#include "include/Messages.h"

// port used to connect to TX2
#define PORT 5000 

// Macros for different IP addresses. Please note that the TX2s IP address could change
// at any time as IP addresses are dynamic, not static. If the controller stops connecting
// to the TX2, it is likely that the TX2s IP address has changed, use ifconfig to figure out
// what it is. One of these macros will need to be modified.
// Please also make sure that the machine controller is running on and the TX2 are on the
// same network. This will not work if they are on different networks.
#define ETHERNET_WIFI "10.42.0.33"

// wasd macro
#define WASD_PRESS(c) (c == 'w' || c == 'W' ||\
		       c == 'a' || c == 'A' ||\
		       c == 's' || c == 'S' ||\
		       c == 'd' || c == 'D')

// position macro
#define DIR_PRESS(c) (c == '1' || c == '2' ||\
		      c == '3' || c == '4')

// macro to copy s to d
#define COPY_POS(d, s) d.latitude = s.latitude;\
		       d.longitude = s.longitude

// p was pressed
#define PARAMETERS(c) ('p' == (c) || 'P' == (c))

// k was pressed
#define KILL(c) ('k' == (c) || 'K' == (c))

// positions 1-4 are all between ISELF, ECC, and the Education Building
Position p1 = { .latitude = 45.550721f, .longitude = -94.151741f };
Position p2 = { .latitude = 45.551082f, .longitude = -94.151746f };
Position p3 = { .latitude = 45.551488f, .longitude = -94.151698f };
Position p4 = { .latitude = 45.551071f, .longitude = -94.151232f };

// positions 5-16 are all down in Husky Stadium. they are meant to be
// used in groups, [p5-p8], [p09-p12], [p13-16] though there is
// no reason they couldn't be intermignled
Position p5 = { .latitude = 45.547445f, .longitude = -94.150944f };
Position p6 = { .latitude = 45.547524f, .longitude = -94.150423f };
Position p7 = { .latitude = 45.547829f, .longitude = -94.150434f };
Position p8 = { .latitude = 45.547738f, .longitude = -94.150965f };

Position p09 = { .latitude = 45.547558f, .longitude = -94.150741f };
Position p10 = { .latitude = 45.547445f, .longitude = -94.150865f };
Position p11 = { .latitude = 45.547370f, .longitude = -94.150724f };
Position p12 = { .latitude = 45.547465f, .longitude = -94.150550f };

Position p13 = { .latitude = 45.547329f, .longitude = -94.151008f };
Position p14 = { .latitude = 45.547359f, .longitude = -94.150305f };
Position p15 = { .latitude = 45.548103f, .longitude = -94.150353f };
Position p16 = { .latitude = 45.548088f, .longitude = -94.151421f };

int main(int argc, char const *argv[]) 
{ 
	int status, index, bytes, i;
	struct sockaddr_in address; 
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char param1[16];
	int opt = 1;
	int child1;
	unsigned long previousCommandId;

	OpMode opMode = Manual;
	
 /* Variable to store user content */
    char letter,keyPress;

    /* File pointer to hold reference to our file */
    FILE * fPtr;
    /* infinite loop */
    int quit = 0;
    int ReadFile = 0;
    int repeat = 1;
    

	printf("Starting Controller\n");

	Message message;

	memset(&message, 0, sizeof(message));

	// create TCP socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	memset(&serv_addr, '0', sizeof(serv_addr)); 

	// setup port
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, ETHERNET_WIFI, &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 

	// use no-dealy
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
	
	// finally, connect to TX2
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	} 
	
	// use a child process to print information
	// fork and run logWriter process
	if((child1 = fork()) == 0)
	{
		sprintf(param1, "%d", sock);
		execl("./logWriter", "logWriter", param1, (char*) NULL);
	}

	sleep(1);

	// this allows us to press a key on the keyboard without needing
	// to press enter
	system("/bin/stty raw");

	printf("starting \n");
	do
	{
		// block for user input
		//status = read(1, &keyPress, sizeof(keyPress));

		printf("\r");
        //read the char in a file
	  if(repeat == 1)
        {
	    fPtr = fopen("/home/rover/Desktop/TX2_Nodes/passchar.txt", "r");
           /* fopen() return NULL if last operation was unsuccessful */
    	    if(fPtr == NULL)
            {
      	      /* File not created hence exit */
              printf("Unable to create file.\n");
              quit = 1;
            }
        
              fscanf(fPtr, "%c", &letter);
	          fclose(fPtr);
	          remove("passchar.txt");
              printf("%c\n",letter);
              keyPress= letter;
              repeat = 0;
	}
        
		if (WASD_PRESS(keyPress))
		{
			// manual message, prepare to send to CAN
			message.messageType = CANMessage;
			message.destination = TX2Nav;
			message.canMsg.SId = 0x123;
			message.canMsg.Bytes = 1;

			// figure out which key was pressed, give it the
			// appropriate CAN message for motor controller
			if (keyPress == 'w'){
				message.canMsg.Message[0] = 2;
                 repeat = 0; 
        	     letter = 0;
        	     keyPress = 0;}
			else if (keyPress == 'a'){
				message.canMsg.Message[0] = 1;
                repeat = 0; 
        	    letter = 0;
        	    keyPress = 0;}
			else if (keyPress == 'd'){
				message.canMsg.Message[0] = 0;
                repeat = 0; 
        	    letter = 0;
        	    keyPress = 0;}
			else if (keyPress == 's'){
				message.canMsg.Message[0] = 3;
                repeat = 0; 
        	    letter = 0;
        	    keyPress = 0;}
			else{
				message.canMsg.Message[0] = 4;
                repeat = 0; 
        	    letter = 0;
        	    keyPress = 0;}
			// send message off to TX2
			status = write(sock ,&message, sizeof(message));
		}
		else if (keyPress == 'c' || keyPress == 'C')
		{
			// camera button was pressed
			// create simple camera message
			message.destination = TX2Cam;
			message.messageType = CamMessage;

			write(sock, &message, sizeof(message));
            repeat = 0; 
        	letter = 0;
        	keyPress = 0;
		}
		else if (keyPress == 'm' || keyPress == 'M')
		{
			// we are toggling our operation mode
			message.destination = TX2Nav;
			message.messageType = OperationMode;

			if (opMode == Manual) {
				// was manual, now automatic
				opMode = Automatic;
				message.opModeMsg.opMode = Automatic;
			} else {
				// was automatic, now manual
				opMode = Manual;
				message.opModeMsg.opMode = Manual;
			}
			write(sock, &message, sizeof(message));
		}
	    else if(keyPress == 'x')
		{
			// tell tx2 we are disconnecting
			message.messageType = ClientDisconnect;
			write(sock, &message, sizeof(message));
			quit = 1;
		}	
           
	   if (repeat == 0){

	     fPtr = fopen("/home/rover/Desktop/TX2_Nodes/passchar.txt", "r");
           /* fopen() return NULL if last operation was unsuccessful */
    	    if(fPtr != NULL)
            {
      	      repeat = 1;
            }
	     }

	} while (!quit);

	system("/bin/stty cooked");	

	// kill logWriter
	kill(child1, SIGKILL);

	// shutdown socket
	shutdown(sock, SHUT_RDWR);

	close(sock);

	return 0; 
} 

