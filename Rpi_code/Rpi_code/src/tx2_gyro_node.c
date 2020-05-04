/**
 * @file tx2_gyro_node.c_
 * @author Patrick Henz
 * @date 12-1-2019
 * @brief Gyro node for TX2.
 * @details Gyro node for the TX2. Uses the LSMDS1 module for gyro functionality. This node only
 * 	    reads in values from the gyroscope and calculates angular velocity (degs/sec) if it
 * 	    has received a request to do so. Otherwise this node sits idle, waiting for a request
 * 	    to perfom some action. This node also makes use of shared memory to communicate with
 * 	    the navigation node (tx2_nav_node.c) in situations where sending a #Message over a pipe
 * 	    wouldn't work correctly.
 */

#include <stdio.h>
#include "../include/Messages.h"
#include "../include/I2CGyro.h"
#include "../include/SharedMem.h"
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#define SEC_COUNT(s) ((s) * 238)

#define DEBUG /**< This compiles the program for debug mode. There are certain macros and print
		   statements that are included when DEBUG is defined. Comment out for non-debug
		   commpilation. */

int main(int argc, char ** argv)
{
	fd_set rdfs;
	int status;
	int masterRead;
	int masterWrite;
	int nbytes;
	int readFds[2];
	int i;
	int I2CGyroFd;
	int killMessageReceived;
	int positionCount = 0;
	int sampleCount;
	int noSampleCount;
	int lowCount;
	int sampling;
	float zVelocity;
	float angleTurned;
	SharedMem * sharedAngle;

	Message message;
	memset(&message, 0, sizeof(message));

	// make sure master has given us the correct
	// number of pipes
	if (argc != 3) {
		printf("Error starting Nav Node\n");
		return -1;
	}

	// open I2C fd for gyro
	I2CGyroFd = OpenI2C();

	masterRead = atoi(argv[1]);
	masterWrite = atoi(argv[2]);

	readFds[0] = masterRead;

	// initialize SetAndWait
	SetupSetAndWait(readFds,1);

	// create shared memory
	sharedAngle = CreateSharedMemory(sizeof(float), AngleData);

	if (sharedAngle == NULL) {
		printf("error creating shared memory for angle\n");
		return -1;
	}

	sharedAngle->dataAvailableFlag = 0;

	// setup shared mem message for TX2Nav
	message.messageType = SharedMemory;
	message.source = TX2Gyro;
	message.destination = TX2Nav;

	sleep(5);

	// signal nav node that shared mem is ready
	write(masterWrite, &message, sizeof(message));

	killMessageReceived = 0;
	sampling = 0;

	//  main while loop
	while(!killMessageReceived) {
		// wait until new message available
		if (SetAndWait(&rdfs, 1, 0) < 0) {
			printf("SET AND WATI ERROR GYRO\n");
		}

		// only 1 FD to read from
		if (!FD_ISSET(masterRead, &rdfs)) {
			continue;
		}

		// read new message from master
		read(masterRead, &message, sizeof(message));

		// kill message
		if (message.messageType == KillMessage) {
			killMessageReceived = 1;
			CloseI2C();
			close(masterRead);
			close(masterWrite);
			break;
		} else if (message.messageType == GyroMessage && message.source == TX2Nav) {
			// we have a request for gyro data
			noSampleCount = 0;

			// sit in while loop and collect turning data
			while (sharedAngle->dataAvailableFlag == 0 && noSampleCount < SEC_COUNT(2)) {
				// get angular velocity sample
				zVelocity = GetAngularVelocity();

				// trigger to start sampling
				if (!sampling && (zVelocity > 20 || zVelocity < -20)) {
					angleTurned = 0;
					sampleCount = 1;
					sampling = 1;
					lowCount = 0;
					angleTurned += (zVelocity * SAMPLE_T);
				} else if (sampling && (zVelocity > 10 || zVelocity < -10)) {
					// keep sampling while turning is happening
					sampleCount++;
					angleTurned += (zVelocity * SAMPLE_T);
				} else if (sampling && lowCount++ < 25) {
					// used to account for noise during turn, keep
					// sampling until we are mostly sure that turn 
					// has completed
					sampleCount++;
				} else if (sampling && sampleCount >= 75) {
					// we have good turning data, set shared memory
					SET_SHARED_ANGLE(sharedAngle, angleTurned);
					sampling = 0;
				} else {
					// reset
					sampling = 0;
					noSampleCount++;
				}
				// sleep so we are at a ~238 HZ sampling frequency
				usleep(USEC_238HZ);
			}
		}
	}

	printf("killing gyro node\n");

	CloseI2C();

	return 0;
}
