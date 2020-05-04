#define WITH_AVERAGING

/**
 * @file tx2_gps_node.c
 * @author Patrick Henz
 * @date 12-1-2019
 * @brief GPS Node for TX2.
 * @details This TX2 software node initializes the GNSS module (XA1110) and polls it
 *          for detail when prompted to do so. To request data, a message must be sent to
 *          this node (use example at line 165) to wake it up and write a new position to 
 *          shared memory. Shared memory macros #GET_SHARED_POSITION and #SET_SHARED_POSITION
 *          are used to set these locations in memory accordingly.
 */

// these macros are for command packets for the GNSS module. The majority are not in use.
// please consult with the following manual for additonal information on the following command
// packets
//
// AirPrime_XM_XA_Series_Software_User_Guide_r3
//
#define CMD_MODE "$PGCMD,380,7*"
#define UPDATE_RATE_10HZ "$PGCMD,233,3*"
#define UPDATE_RATE_05HZ "$PGCMD,233,2*"
#define GPS_ONLY  "$PGCMD,229,1,0,0,0,1*"
#define GPS_GALI  "$PGCMD,229,1,0,0,1,1*"
#define DISABLE_229 "$PGCMD,229,1,0,0,0,0*"
#define FITNESS_MODE "$PMTK886,1*"
#define NORMAL_MODE  "$PMTK886,0*"
#define AIC_MODE     "$PMTK286,1*"
#define RTCM_MODE    "$PMTK301,1*"
#define FULL_COLD_START "$PMTK104*"
#define GNSS_SBAS_EN    "$PMTK313,1*"
#define DGPS_SBAS    "$PMTK301,2*"
#define MIN_SAT      "$PMTK306,25*"
#define CLEAR_EPO    "$PMTK127*"
#define SIX_PREC     "$PMTK265,3*"

#define SEARCH_GPS_GLONASS "$PMTK353,1,1,0,0,0*"

                 //         0 1 2 3 4 5 6 7 8 9 A B C D E F G 0 1 2
#define MIN_PRINT "$PMTK314,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*"

/**
 * @brief Converts from miliseconds to nanoseconds.
**/
#define MS_TO_NS(x) ((x) * 1000000) // convert ms to ns
		 
#include <stdio.h>
#include "../include/Messages.h"
#include "../include/I2CGPS.h"
#include "../include/SharedMem.h"
#include <unistd.h>
#include <signal.h>
#include <stdint.h>


#define GPS_AVERAGE_COUNT 5 

#define DEBUG /**< This compiles the program for debug mode. There are certain macros and print
		   statements that are included when DEBUG is defined. Comment out for non-debug
		   commpilation. */

/**
 * @brief Internal struct used to keep track of a moving #Position average.
**/
typedef struct _Positions {
	Position position[GPS_AVERAGE_COUNT];
	int head;
	int positionCount;
} Positions;

/**
 * @brief Function adds a new position to the #Positions struct.
 * @details Function adds a new position to the #Positions struct. The #Position array  witin
 * 	    #Positions is used as a ring buffer, each new #Position overwrites the oldest value
 * 	    in the array.
 * @param positions A pointer to the #Positions struct being modified.
 * @param p1 The new #Position being added to the array in #Positions.
**/
void AddPosition(Positions * positions, Position * p1)
{
	// add position at current location of head
	positions->position[positions->head].latitude = p1->latitude;
	positions->position[positions->head].longitude = p1->longitude;

	// increment head, ring buffer fashion
	positions->head = (positions->head + 1) % GPS_AVERAGE_COUNT;

	// only increment count until we have all locations filled
	if (positions->positionCount < GPS_AVERAGE_COUNT) {
		positions->positionCount++;
	}
}

/**
 * @brief Function calculates the moving average for the positions in #Positions struct.
 * @details Function calculates the moving average for the positions in #Positions struct.
 * 	    It should be noted that this function assumes that the positions array is full
 * 	    of current values. If there arent #GPS_AVERAGE_COUNT values in the array, the 
 * 	    behavior of this function is unpredictable.
**/
void GetPositionAverage(Positions * positions, Position * average)
{
	int i;

	// zero out average
	memset(average, 0, sizeof(Position));

	// accumulate
	for (i = 0; i < GPS_AVERAGE_COUNT; i++) {
		average->latitude += positions->position[i].latitude;
		average->longitude += positions->position[i].longitude;
	}

	// calculate average
	average->latitude /= GPS_AVERAGE_COUNT;
	average->longitude /= GPS_AVERAGE_COUNT;
}

int main(int argc, char ** argv)
{
#ifdef DEBUG
	printf("staring navigation node\n");
#endif

	fd_set rdfs;
	int status;
	int masterRead;
	int masterWrite;
	int readFds[2];
	int i;
	int I2CGPSFd;
	int killMessageReceived;
	Position previousPosition = { .latitude = 0.0f, .longitude = 0.0f};
	Position positionAverage;
	int positionsTaken;
	int navigationCalibrationComplete;
	SharedMem * sharedPosition;
	int newAverage;

	Message message;

	Positions positions = { .head = 0, .positionCount = 0 };

	// make sure master has given use correct number of pipes
	if (argc != 3) {
		printf("Error starting Nav Node\n");
		return -1;
	}

	masterRead = atoi(argv[1]);
	masterWrite = atoi(argv[2]);

	// open i2c fd for XA1110
	I2CGPSFd = I2CGPSOpen();

	if (0 >= I2CGPSFd) {
		printf("I2C FAILURE\n");
	}

	readFds[0] = masterRead;
	readFds[1] = I2CGPSFd;

	// setup SetAndWait
	SetupSetAndWait(readFds,2);

	killMessageReceived = 0;

	// create shared memory location to share with  tx2_nav_node.c
	sharedPosition = CreateSharedMemory(sizeof(Position), PositionData);

	if (sharedPosition == NULL) {
		printf("error creating shared memory for position\n");
		return -1;
	}

	// prep shared memory messasge for TX2Nav
	message.messageType = SharedMemory;
	message.source = TX2Gps;
	message.destination = TX2Nav;

	sleep(5);

	// notify nav node that shared memory is available
	write(masterWrite, &message, sizeof(message));

	// clear data valid flag
	sharedPosition->dataAvailableFlag = 0;

	////////////////////////////////////////////////////////////////
	//			XA1110 Initialization                 //
	//------------------------------------------------------------//
	// Please leave the commented out code in the sceanrio that   //
	// it is still needed. Some commands to the XA1110 require    //
	// the device to do a full cold start, which ends up wiping   //
	// all current user settings. The GNSS module also losses its //
	// fix on satellites and takes 30~60 seconds to acquire a fix //
	// again. As the GNSS module is currrently configured as      //
	// needed, this code is commented out to speed up the start   //
	// process. There is likely a better implementation for this  //
	// but for the time being is left as is in the sceanrio that  //
	// these commands will be needed at somepoint, which is       //
	// highly likely.					      //
	////////////////////////////////////////////////////////////////
	
	// initialize XA1110
        //I2CGPSWrite(CMD_MODE);
	//I2CGPSWrite(UPDATE_RATE_10HZ);
	//I2CGPSWrite(FULL_COLD_START);
	//sleep(30); // sleep while the GNSS module warms up after restart
	//I2CGPSWrite(CLEAR_EPO);
        I2CGPSWrite(SEARCH_GPS_GLONASS); // search GPS and Glonass
        I2CGPSWrite(MIN_PRINT);		 // print only positional information
	I2CGPSWrite(MIN_SAT);	         // min satellite count
	I2CGPSWrite(GNSS_SBAS_EN);	 // enable sbas
	I2CGPSWrite(DGPS_SBAS);		 // set to use sbas
	I2CGPSWrite(AIC_MODE);		 // active interference correction
	I2CGPSWrite(FITNESS_MODE);	 // fitness mode, supposed to work better at slow speeds
	
	printf("GPS unit initialized\n");

	navigationCalibrationComplete = 0;

	//  main while loop
	while(!killMessageReceived) {
		// wait for fds to become available, or timeout to get GNSS data
		if (SetAndWait(&rdfs, 0, MS_TO_NS(50)) < 0) {
			printf("SET AND WAIT ERROR GPS\n");
		}

		// check fds
		for (i = 0; i < 2; i++) {
			if (!FD_ISSET(readFds[i], &rdfs)) {
				continue;
			}

			// I don't know that this would every happen... I'm guessing the device
			// needs to be polled to retrieve data...
			if (readFds[i] == I2CGPSFd) {
				I2CGPSRead(&message);
			} else if (readFds[i] == masterRead) {
				// read the message from master
				read(readFds[i], &message, sizeof(message));

				// kill message
				if (message.messageType == KillMessage) {
					killMessageReceived = 1;
					CloseI2C();
					close(masterRead);
					close(masterWrite);
					break;
				} else if (CalibrationCompleteMessage == message.messageType && TX2Nav == message.source) {
					// wait for nav node to finish calibration before sending messages
					printf("GPS Received complete from Nav\n\n");
					navigationCalibrationComplete = 1;
				} 
				
			}
		}

		newAverage = 0;

		// ping XA1110. If data new position data has been acquired, add it
		// to the Positions struct
		if (I2CGPSRead(&message)) {
			AddPosition(&positions, &message.gpsMsg.position);
			newAverage = 1;
		}

		// if we have enough GNSS positions and a new value has been acquired
		if (positions.positionCount == GPS_AVERAGE_COUNT && newAverage) {
			// calculate the averge
			GetPositionAverage(&positions, &positionAverage);
			// set shared memory
			SET_SHARED_POSITION(sharedPosition, positionAverage);
		}
	}

	printf("killing gps node\n");
	return 0;
}
