/*
 * @file tx2_nav_node.c
 * @author Patrick Henz
 * @date 12-1-2019 
 * @brief Navigation node for TX2.
 * @details Navigation node for the TX2 rover. This node handles all naviagtion related
 * 	    decision making and utilizes shared memory from tx2_cam_node.cpp, tx2_gps_node.c,
 * 	    and tx2_gyro_node.c.   
 */

#include <stdio.h>
#include "../include/Messages.h"
#include "../include/SharedMem.h"
#include "../include/LatLonTrig.h"
#include "../include/FilterGen.h"
#include "../include/Parameters.h"
#include "../include/protocol.h"
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#define DEBUG /**< This compiles the program for debug mode. There are certain macros and print
		   statements that are included when DEBUG is defined. Comment out for non-debug
		   commpilation. */

/**
 * @brief Value indicates that the CAN message in #Message holds no navigation value for motor control.
**/
#define NO_VALUE -1

/**
 * @brief Checks CAN message in #Message; returns true if equal to #NO_VALUE, else false.
**/
#define IS_NO_VALUE(m) (NO_VALUE == (m).canMsg.Message[0])

/**
 * @brief Returns true if turn is a left turn.
**/
#define IS_LEFT_TURN(v) ((v) < 0.0f)

/**
 * @brief Returns true if turn is a right turn.
**/
#define IS_RIGHT_TURN(v) ((v) > 0.0f)

/**
 * @brief Copies #Position p2 over to p1.
**/
#define COPY_POS(p1, p2) (p1.latitude = p2.latitude);\
		           (p1.longitude = p2.longitude);

/**
 * @brief Returns true if the #Position p is not the Gulf of Guinea.
**/
#define NOT_GULF_OF_GUINEA(p) (0.0f != p.latitude || 0.0f != p.longitude)

/**
 * @brief Flush values from protocol.h
**/
#define FLUSH FLUSH_BITS

/**
 * @brief Flush values from protocol.h
**/
#define NO_FLUSH 0

/**
 * @brief Used to set the CAN message in a #Message struct.
 * @detail f takes the macros #FLUSH or #NO_FLUSH, m is the #Message struct, and d is 
 * 	   a direction value as defined in protocol.h.
**/
#define DIRECTION_MESSAGE(f, m, d) ((m).canMsg.Message[0] = SET_CMD(f, PUSH, d))

/**
 * @brief Checks direction value in #Message.
 * @detail Returns true if the CAN message in #Message m is equal to d, the direction
 * 	   value as defined in protocol.h.
**/
#define DIRECTION_MESSAGE_EQUALS(m, d) ((d) == (m).canMsg.Message[0])

/**
 * @brief States used by the navigation node when navigating to destination positionx.
**/
typedef enum navigationStates {
	Stopped,
	MovingForward,
	TurningLeft,
	TurningRight
} NavigationStates;

/**
 * @brief Global used to keep track of current #NavigationStates.
**/
NavigationStates currentState;

/**
 * @brief Holds the left filter from the FilterGen.h library.
**/
FILTER_TYPE * leftFilter;

/**
 * @brief Holds the right filter from the FilterGen.h library.
**/
FILTER_TYPE * rightFilter;

/**
 * @brief Holds the center filter from the FilterGen.h library.
**/
FILTER_TYPE * centerFilter;

/**
 * @brief Holds the number of no-zero elements in the left filter created with FilterGen.h library.
**/
unsigned int leftFilterArea;

/**
 * @brief Holds the number of no-zero elements in the right filter created with FilterGen.h library.
**/
unsigned int rightFilterArea;

/**
 * @brief Holds the number of no-zero elements in the center filter created with FilterGen.h library.
**/
unsigned int centerFilterArea;

/**
 * @brief Holds a list of values from semantic segmentation dot product with left filter in #PreviousValues struct. 
**/
PreviousValues leftValues;

/**
 * @brief Holds a list of values from semantic segmentation dot product with right filter in #PreviousValues struct. 
**/
PreviousValues rightValues;

/**
 * @brief Holds a list of values from semantic segmentation dot product with center filter in #PreviousValues struct. 
**/
PreviousValues centerValues;

/**
 * @brief Current position of rover.
**/
Position currentPosition;

/**
 * @brief Destination position of current command.
**/
Position destinationPosition;

/**
 * @brief Previous position of rover at last turn.
**/
Position previousPosition;

/**
 * @brief At destination flag.
**/
int atDestination;

/**
 * @brief Holds the angle the rover would turn from 1 turn command. 
**/
double trueTurningAngle;

/**
 * @brief Global #Parameters struct. 
**/
Parameters parameters;

/**
 * @brief Width of image/semantic segmentation array.
**/
int imageWidth;

/**
 * @brief Height of image/semantic segmentation array.
**/
int imageHeight;

/**
 * @brief #SharedMem with tx2_gyro_node.c.
**/
SharedMem * sharedAngle;

/**
 * @brief #SharedMem with tx2_gps_node.c.
**/
SharedMem * sharedPosition;

/**
 * @brief Lookup table that stores rover turning angles for specific turn command counts.
 * @details Lookup table that stores rover turning angles for specific turn command counts.
 * 	    The indicies in this array contain the turning angle, and the index itself is the number
 * 	    of turn commands needed for the rover to turn that angle.
**/
double TurningLookupTable[11];

/**
 * @brief Number of elements in #TurningLookupTable.
**/
int TurningLookupTableCount = 0;

/**
 * @brief Flag to keep track of request semantic segmentation data.
**/
int segmentationRequestSent = 0;

/**
 * @brief Internal function used by nav node to perform semantic segmentation dot product.
 * @details Internal function used by nav node to perform semantic segmentation dot product.
 * 	    Currently, this function splits the seg data in half and only performs the dot 
 * 	    product with the lower half of the image. The idea is that anything above the 
 * 	    horizon isn't going to give us meaningful navigation information. This will
 * 	    likely, and probably should, account for the whole image at some point.
 * @param mask The semantic segmentation array.
 * @param filter The semantic segmentation filter produced with #FilterGen.h.
**/
FILTER_TYPE ApplyFilter(uint8_t * mask, FILTER_TYPE * filter)
{
	int index;
	// zero out accumulator
	FILTER_TYPE dotProduct = 0;
	// cut the image in half
	uint8_t * half_mask = &mask[(imageWidth*imageHeight)/2];

	// perform the dot product
	for (index = 0; index < ((imageWidth*imageHeight)/2); index++) {
		dotProduct += half_mask[index]*filter[index];
	}

	return dotProduct;
}

/**
 * @brief Returns the number of turn commands needed to turn a certain angle.
 * @details Returns the number of turn commands needed to turn a certain angle.
 * @param angle The desired turning angle.
 * @return The index of the array whose angle is closest to the angle parameter.
**/
int GetTurnCount(double angle)
{
	int i;
	double diffWithCur;
	double diffWithNxt;

	// get absolute value
	if (angle < 0.0f) {
		angle = -angle;
	}

	// loop through array, since there is no 0 turn, skip it
	for (i = 1; i < 10; i++) {
		// if the current index and next index are both greater than angle, continue
		if ((i + 1) < TurningLookupTableCount && TurningLookupTable[i] < angle && TurningLookupTable[i + 1] < angle) {
			continue;
		} else {
			// angle is inbetween index i and i + 1
			// get difference between angle and i
			diffWithCur = angle - TurningLookupTable[i];

			// figure out the difference between angle and i + 1
			// make sure it is positive
			if (angle > TurningLookupTable[i + 1]) {
				diffWithNxt = angle - TurningLookupTable[i + 1];
			} else {
				diffWithNxt = TurningLookupTable[i + 1] - angle;
			}

			// return the index with the smallest difference
			if (diffWithCur < diffWithNxt)	{
				return i;
			} else {
				return i + 1;
			}
		}
	}

	// we didn't find it, this is unlikely to occur
	return (i + 1);
}

/**
 * @brief Sends request to tx2_cam_node.cpp for more semantic segmentation data.
 * @param masterWrite The fd for the masterWrite pipe.
**/
void RequestSemSegData(int masterWrite)
{
	Message message;

	segmentationRequestSent = 1;

	// notify cam module that we are ready for data
	memset(&message, 0, sizeof(message));
	message.messageType = SharedMemory;
	message.source = TX2Nav;
	message.destination = TX2Cam;
	write(masterWrite, &message, sizeof(message));
}

/**
 * @brief Sends request to tx2_gyro_node.cpp for turning data.
 * @param masterWrite The fd for the masterWrite pipe.
**/
void RequestGyroData(int masterWrite)
{
	Message message;

	// notify gyro module we are ready for data
	memset(&message, 0, sizeof(message));
	message.messageType = GyroMessage;
	message.source =TX2Nav;
	message.destination = TX2Gyro;
	write(masterWrite, &message, sizeof(message));
}

/**
 * @brief Function that determines how the rover is to manuever its environment.
 * @details Function that determines how the rover is to manuever its environment.
 * 	    At the moment, this function is triggered only when new semantic
 * 	    segmentation data is available and ready to use. Several shared memories
 * 	    are used by this function to give the navigation node the most up-to-date 
 * 	    data possible. In this function, the navigation node makes really basic
 * 	    navigation decisions; continue moving straight, turn left, turn right, or do 
 * 	    nothing. It does this by bringing in semantic segmentation data and gps data.
 * 	    <br>
 * 	    <br>
 * 	    The semantic segmentation data is multiplied with a left, right, and center filter
 * 	    using a dot product, and the summed value is divided by the number of non-zero elements
 * 	    in the filters to produce something like an average. The semantic segmentation data is 
 * 	    in an array that has the same number of elements as the pixels in the original image. 
 * 	    Each element is an integer value reperesenting the class it belongs to. Here is the 
 * 	    class breakdown.
 * 	    <br>
 * 	    <br>
 * 	    <center>
 * 	    segNet -- class 00  label 'void'
 * 	    segNet -- class 01  label 'dynamic'
 * 	    segNet -- class 02  label 'ground'
 * 	    segNet -- class 03  label 'road'
 * 	    segNet -- class 04  label 'sidewalk'
 * 	    segNet -- class 05  label 'parking'
 * 	    segNet -- class 06  label 'building'
 * 	    segNet -- class 07  label 'wall'
 * 	    segNet -- class 08  label 'fence'
 * 	    segNet -- class 09  label 'guard rail'
 * 	    segNet -- class 10  label 'bridge tunnel'
 * 	    segNet -- class 11  label 'pole'
 * 	    segNet -- class 12  label 'traffic light'
 * 	    segNet -- class 13  label 'traffic sign'
 * 	    segNet -- class 14  label 'vegetation'
 * 	    segNet -- class 15  label 'terrain'
 * 	    segNet -- class 16  label 'sky'
 * 	    segNet -- class 17  label 'person'
 * 	    segNet -- class 18  label 'car'
 * 	    segNet -- class 19  label 'truck'
 * 	    segNet -- class 20  label 'cycle'
 * 	    </center>
 * 	    <br>
 * 	    <br>
 * 	    The idea is that the 'average' produced from the dot product gives us a general idea 
 * 	    of what is in front of us, or to our left or right. We consider smaller values like 'ground',
 * 	    'road', and 'sidewalk' things that are safe to drive on, and things like buildings and people
 * 	    as objects to avoid. As the dot product produces a single value, we can use the single value of
 * 	    the left, center, and right averages to make a decision, rather than a more complex operation.
 * 	    Essentially, the lower the value, the 'safer' we consider it to be.
**/
void MoveRover(uint8_t * mask, int masterWrite)
{
	double distanceToGo;
	double distanceFromPrevious;
	double turn;
	double absTurn;
	double adjustedWeight;
	double angleTurned;
	int canIMove;
	unsigned int multiTurnAttempts;
	int i;

	int directionCount;

	// we only send messages, if at all, to the CAN node
	// prep for this
	Message message;
	memset(&message, 0, sizeof(Message));
	message.messageType = CANMessage;
	message.destination = TX2Can;
	message.source = TX2Nav;
	message.canMsg.SId = 0x123;
	message.canMsg.Bytes = 1;

	// initialize with no value
	DIRECTION_MESSAGE(NO_FLUSH, message, NO_VALUE);

	directionCount = 0;
	turn = 0;

	FILTER_TYPE centerAverage, leftAverage, rightAverage;

	//  apply dot product and enter new value into values arrays	
	EnterNewValue(&centerValues, (ApplyFilter(mask, centerFilter) / centerFilterArea));
	EnterNewValue(&leftValues, (ApplyFilter(mask, leftFilter) / leftFilterArea));
	EnterNewValue(&rightValues, (ApplyFilter(mask, rightFilter) / rightFilterArea));

	// are we using gps?
	if (parameters.usingGps) {
		// get the most up to date position form the gps node
		GET_SHARED_POSITION(sharedPosition, currentPosition);

		// if we need to initialize previousDestination, do that now
		if (!NOT_GULF_OF_GUINEA(previousPosition)) {
			COPY_POS(previousPosition, currentPosition);
		}

		// if we have enough info to calculate distances, do it
		if (NOT_GULF_OF_GUINEA(currentPosition) && NOT_GULF_OF_GUINEA(previousPosition)) {
			// distance to destination
			distanceToGo = Distance(currentPosition, destinationPosition);
			// distance from last turn
			distanceFromPrevious = Distance(currentPosition, previousPosition);
			// are we at the destination?
			atDestination = (distanceToGo < parameters.distanceToGoThreshold)?(1):(0);
		}
	} else {
		// destination doesn't apply
		atDestination = 0;
	}

	// use usingGps from #Parameters struct to determine how we decide if we can move or not
	canIMove = (!parameters.usingGps)?(EnoughDataPresent(&leftValues)):
				       (EnoughDataPresent(&leftValues) &&
				        !atDestination &&
					NOT_GULF_OF_GUINEA(currentPosition) &&
				        NOT_GULF_OF_GUINEA(destinationPosition));	

	// we have valid coordinates and there is some distance between us and the destination point
	if (canIMove)
	{
		// calculate the dot product moving averages for left, center, and right #PreviousValues
		centerAverage = GetMovingAverage(&centerValues);
		leftAverage = GetMovingAverage(&leftValues);
		rightAverage = GetMovingAverage(&rightValues);

		// if we are using GPS and have traveled far enough, calculate a new turning angle to point
		// us in the right direction to reach destination
		if (parameters.usingGps && distanceFromPrevious > parameters.distanceFromPreviousThreshold){
			printf("DISTANCE FROM PREVIOUS %.4f\n", distanceFromPrevious);
			// calculate new turning angle
			turn = DegreeTurnAndDirection(currentPosition, 
						      previousPosition, 
						      destinationPosition);
			printf("\n\n\nGNSS TURN ANGLE = %.4f\n\n\n", turn);
		}

		// get the absolute value
		absTurn = (turn < 0.0f)?(-turn):(turn);

		directionCount = 0;

		// if we need to turn multiple times for GPS/GNSS turn, weight the left, center, and right
		// dot product averages as needed
		if (absTurn > trueTurningAngle) {
			// rough estimate of the number of turns needed
			directionCount = (unsigned int)(absTurn / trueTurningAngle);

			if (IS_LEFT_TURN(turn)){
				// if we need to turn left, weight the left side to pull it down
				leftAverage *= pow(parameters.turningWeight, directionCount);
			} else if (IS_RIGHT_TURN(turn)){
				// if right, weight the ride side down
				rightAverage *= pow(parameters.turningWeight, directionCount);
			}
		} else {
			// else, weight the center to pull it down lower, left and right higher
			// more likely to keep moving straight
			centerAverage *= parameters.turningWeight;
			rightAverage *= (1 + (1 - parameters.turningWeight));
			leftAverage *= (1 + (1 - parameters.turningWeight));
			directionCount = 1;
		}

		// this switch statement determines how the rover is going to move. Essentially, the lower a value is
		// for a corresponding direction, the more likely we are to try and move in that direction. The states
		// give the rover an affinity to a certain direction. The idea is that if the rover decides to move right,
		// lets say becuase there is a path to the right, we want it to keep moving right until it sees the path and
		// can continue moving forward. Without this the rover has a tendency of getting stuck in a loop of turning
		// left, then right, then left.... so on and so forth. It's not an optimal solution, but it does the trick,
		// especially when driving around on a walkway
		switch(currentState) {
			case Stopped:
				currentState = MovingForward;
				break;
			case MovingForward:
				// if the center value is smallest, just keep moving forward
				if (centerAverage < parameters.dotProductThreshold &&
				    centerAverage < leftAverage && 
				    centerAverage < rightAverage) {
					DIRECTION_MESSAGE(NO_FLUSH, message, MOVE_FORWARD);
					directionCount = 1;
				} else if (leftAverage < rightAverage) {
					// we want to turn left
					currentState = TurningLeft;
					DIRECTION_MESSAGE(FLUSH, message, MOVE_LEFT);
				} else if (rightAverage < leftAverage) {
					// we want to turn right
					currentState = TurningRight;
					DIRECTION_MESSAGE(FLUSH, message, MOVE_RIGHT);
				} else {
					// should be pretty unlikely
					// we essentially do nothing at this point
					directionCount = 0;
				}
				break;
			case TurningLeft:
				if (centerAverage < parameters.dotProductThreshold) {
					// we were turning left, and are now able to move forward
					currentState = MovingForward;
					DIRECTION_MESSAGE(NO_FLUSH, message, MOVE_FORWARD);
					directionCount = 1;
				} else {
					// else, affinity to turning left, keep turning
					DIRECTION_MESSAGE(NO_FLUSH, message, MOVE_LEFT);
					// due to object avoidance
					directionCount = 1;
				}
				break;
			case TurningRight:
				if (centerAverage < parameters.dotProductThreshold) {
					// we were turning right, we are now able to move forward
					currentState = MovingForward;
					DIRECTION_MESSAGE(NO_FLUSH, message, MOVE_FORWARD);
					directionCount = 1;
				} else {
					// else affinity to turning right, keep turning
					DIRECTION_MESSAGE(NO_FLUSH,message, MOVE_RIGHT);
					// due to object avoidance
					directionCount = 1;
				}
				break;
		}

		// if we have turned, we need to clear old values and save currentPosition as previousPosition for
		// turning to work properly
		if (DIRECTION_MESSAGE_EQUALS(message, MOVE_LEFT) || DIRECTION_MESSAGE_EQUALS(message, MOVE_RIGHT)) {
			ClearValues(&centerValues);
			ClearValues(&leftValues);
			ClearValues(&rightValues);
			COPY_POS(previousPosition, currentPosition);
		}

		// just a single move command, write it to master
		if (!DIRECTION_MESSAGE_EQUALS(message, NO_VALUE) && directionCount == 1) {
			message.canMsg.writeCount = 1;
			write(masterWrite, &message, sizeof(message));
		} else if (DIRECTION_MESSAGE_EQUALS(message, MOVE_LEFT) || DIRECTION_MESSAGE_EQUALS(message, MOVE_RIGHT)) {
			multiTurnAttempts = 0;
			// keep trying to get to the angle we need to get to
			do {
				// request the gyro node wake up and start recording
				RequestGyroData(masterWrite);
				// this should already be set first time through, but for 
				// multiturn will potentially need to be set
				if (turn > trueTurningAngle || turn < -trueTurningAngle) {
					if (turn < 0) {
						DIRECTION_MESSAGE(NO_FLUSH, message, MOVE_LEFT);
					} else {
						DIRECTION_MESSAGE(NO_FLUSH, message, MOVE_RIGHT);
					}
				}

				// this is just to clear the data available flag
				sharedAngle->dataAvailableFlag = 0;

				// print what we want to turn
				printf("turning %f\n", turn);

				// if multi turn fails for whatever reason, we will switch
				// to single turn mode
				if (multiTurnAttempts++ < 3) {
					// get turn count
					directionCount = GetTurnCount(turn);
				} else {
					directionCount = 1;
				}
				
				// send turn command to CAN node
				printf("directionCount %d\n", directionCount);
				message.canMsg.writeCount = directionCount;
				write(masterWrite, &message, sizeof(message));

				// get the shared memory angle reading from gyro node
				GET_SHARED_ANGLE(sharedAngle, angleTurned);
				printf("after multiturn I turned %f\n", angleTurned);

				// add angle turned to turn. The gyro turning convention is opposite of
				// calculated angles, so addition makes turn smaller in magnitude.
				// We typically say, if we want to turn left, turn is negative. If we want
				// to turn right, turn is positive.
				//
				// The gyro data returns a positive angle for left turns and a negative
				// angle for right turns.
				turn += angleTurned;

			// continue until turn is small enough
			} while (turn > trueTurningAngle || turn < -trueTurningAngle);
			printf("\n\nMULTI TURN COMPLETE\n\n");
		} else {
			COPY_POS(previousPosition, currentPosition);
		}
	}
	
	if (!atDestination) {
		// only request new seg data if we are current navigating
		RequestSemSegData(masterWrite);
	} else if (atDestination){
		// send request to master for additional commands, if any at all
		message.source = TX2Nav;
		message.destination = TX2Master;
		message.messageType = CommandMessage;

		printf("\n\n!!!AT DESTINATION!!!\n");
		printf("REQUESTING NEW COMMAND\n\n");

		write(masterWrite, &message, sizeof(message));
	} 
}

/**
 * @brief Function used to populate #TurningLookupTable.
 * @details Function used to populate #TurningLookupTable. This function
 * 	    only populates the array up to the point where we are over 
 * 	    180 degree turns, as anything larger than that would enver 
 * 	    be used. The number of elements in #TurningLookupTable is
 * 	    retained in #TurningLookupTableCount.
 * *param masterWrite FD for masterWrite pipe.
**/
void CalibrateTurning(int masterWrite)
{
	double singleRight;
	double multiRight;
	double singleLeft;
	double multiLeft;
	int turncount;
	int i;

	Message message;

	// print message
	printf("\n\n\nCALIBRATING\n\n\n");

	// prepare CAN #Message 
	memset(&message, 0, sizeof(message));

	message.messageType = CANMessage;
	message.destination = TX2Can;
	message.source = TX2Nav;
	message.canMsg.SId = 0x123;
	message.canMsg.Bytes = 1;

	// set direction as left, since left turns are 
	// read as being positive
	DIRECTION_MESSAGE(NO_FLUSH, message, MOVE_LEFT);

	// loop through array
	for (i = 1; i < 11; i++ ){
		// wake gyro up
		RequestGyroData(masterWrite);
		// this is just to clear the data available flag
		sharedAngle->dataAvailableFlag = 0;

		// set turn count
		message.canMsg.writeCount = i;

		// execute turn
		write(masterWrite, &message, sizeof(message));	

		printf("\n\nSENDING TURN COMMAND\n\n");
		// get single right turn angle
		GET_SHARED_ANGLE(sharedAngle, singleLeft);	

		printf("%d turn = %f\n", i, singleLeft);

		// in the scenario the rover doens't have good footing
		// and turns less than it did in the previous turn
		if (singleLeft < TurningLookupTable[i-1]) {
			i--;
		} else {
			TurningLookupTable[i] = singleLeft;
			TurningLookupTableCount++;
		}

		// stop once we are at, or above, 180 degrees
		if (singleLeft >= 180.0f) {
			break;
		}

	}

	// print out the turning lookup table
	for (i = 1; i < 11; i++) {
		printf("%f  ", TurningLookupTable[i]);
	}


	// get the smallest angle we can turn
	trueTurningAngle = TurningLookupTable[1];
	message.messageType = CalibrationCompleteMessage;
	message.source = TX2Nav;
	message.destination = TX2Gps;

	// notify gps node that calibration is complete
	write(masterWrite, &message, sizeof(message));

	printf("calibration complete\n");
}

int main(int argc, char ** argv)
{
#ifdef DEBUG
	printf("staring navigation node\n");
#endif
	fd_set rdfs;
	OpMode opMode;
	int status;
	int masterRead;
	int masterWrite;
	SharedMem * sharedMem;
	int nbytes;
	int readFds[1];
	uint8_t * mask;
	int i;
	int x, y;
	int direction;
	int killMessageReceived;
	double tempDistance;
	int receivedSegMem = 0;
	int receivedAngMem = 0;
	int receivedPosMem = 0;
	double tempAngle; //for testing

	Message message;

	// make sure we have enough pipes
	if (argc != 3) {
		printf("Error starting Nav Node\n");
		return -1;
	}

	masterRead = atoi(argv[1]);
	masterWrite = atoi(argv[2]);

	readFds[0] = masterRead;

	// initialize SetAndWait
	SetupSetAndWait(readFds,1);

	// as the nav node utilizes shared memory that is initialized by 
	// other process, wait here until everything is available
	do {
		// wait until new data is ready in fd
		if (SetAndWait(&rdfs, 1, 0) < 0) {
			printf("SET AND WAIT ERROR NAV 1\n");
		}

		// check each fd for data
		for (i = 0; i < 1; i++) {
			if (!FD_ISSET(readFds[i], &rdfs)) {
				continue;
			}

			// read message, should just be from master
			read(readFds[i], &message, sizeof(message));
			
			if (message.messageType == SharedMemory && message.source == TX2Cam) {
				// camera node shared memory ready
				imageWidth = message.shMem.width;
				imageHeight = message.shMem.height;
				receivedSegMem = 1;
				printf("\n\nSEG MEM REC\n\n");
			}
			if (message.messageType == SharedMemory && message.source == TX2Gyro) {
				// gyro node shared memory ready
				receivedAngMem = 1;
				printf("\n\nANG MEM REC\n\n");
			}
			if (message.messageType == SharedMemory && message.source == TX2Gps) {
				// gps shared memory ready
				receivedPosMem = 1;
				printf("\n\nPOS MEM REC\n\n");
			}
		}
	} while(!receivedSegMem || !receivedAngMem || !receivedPosMem);

	// open shared memory for semantic segmentation
	sharedMem = OpenSharedMemory((imageWidth * imageHeight), SegmentationData);

	if (NULL == sharedMem) {
		printf("SEGMENTATION SHARED MEMORY ERROR IN NAV NODE\n");
		pause();
	}

	// open shared memory for angle data
	sharedAngle = OpenSharedMemory(sizeof(float), AngleData);

	if (NULL == sharedAngle) {
		printf("ANGLE SHARED MEMORY ERROR IN NAV NODE\n");
		pause();
	}

	// open shared memory for position data
	sharedPosition = OpenSharedMemory(sizeof(Position), PositionData);

	if (NULL == sharedPosition) {
		printf("POSITION SHARED MEMORY ERROR IN NAV NODE\n");
		pause();
	}

	printf("\n\nSHARED MEM INITIALIZATION COMPLETE\n\n");

	// get the starting memory address of the semantic segmentation data 
        mask = (uint8_t *)(sharedMem + 1);

	// open the parameters file
	status = GetParameters(PARAMETERS_FILE, &parameters);

	if (-1 == status) {
		printf("ERROR OPENING PARAMETERS FILE\n");
		return -1;
	}

	// print values to terminal
	PrintParameters(&parameters);

	// set the value counts we store for moving averages
	SetMaxCount(&centerValues, parameters.centerDotProductValueCount);
	SetMaxCount(&leftValues, parameters.sideDotProductValueCount);
	SetMaxCount(&rightValues, parameters.sideDotProductValueCount);

	// initialize by clearing
	ClearValues(&centerValues);
	ClearValues(&leftValues);
	ClearValues(&rightValues);

	// set initial positions to gulf of guinea
	memset(&currentPosition, 0, sizeof(Position));
	memset(&previousPosition, 0, sizeof(Position));
	memset(&destinationPosition, 0, sizeof(Position));

	// create the filters we use on the semantic segmentation data
	leftFilterArea = CreateLeftFilter(&leftFilter, (imageWidth/2), (imageHeight/2), imageWidth, (imageHeight/2));
	rightFilterArea = CreateRightFilter(&rightFilter, (imageWidth/2), (imageHeight/2), imageWidth, (imageHeight/2));
	centerFilterArea = CreateCenterFilter(&centerFilter, (int)((double)imageWidth*(0.75f))/2, (int)((double)imageWidth*(0.75f)), (imageHeight/2), imageWidth, (imageHeight/2));	

	currentState = Stopped;

	// kill flag
	killMessageReceived = 0;

	atDestination = 0;

	// put in manual or automatic
	opMode = (parameters.manual)?(Manual):(Automatic);

	// calibrate turning
	//CalibrateTurning(masterWrite);

	// request data if in automatic mode
	if (Automatic == opMode) {
		printf("\n\n\nSTARTING IN AUTOMATIC MODE\n\n\n");
		RequestSemSegData(masterWrite);
	} else {
		printf("\n\n\nSTARTING IN MANUAL MODE\n\n\n");
	}

	//  main while loop
	while(!killMessageReceived)
	{
		// wait for message from master
		if (SetAndWait(&rdfs, 1, 0) < 0) {
			printf("SET AND WAIT ERROR 2 status = %d\n", status);
		}

		// check to see if new data available from master
		for (i = 0; i < 1; i++) {
			if (!FD_ISSET(readFds[i], &rdfs)) {
				continue;
			}

			// read message
			read(readFds[i], &message, sizeof(message));

			// new semantic segmentation data is available
			if (message.source == TX2Cam && opMode == Automatic && message.messageType == SharedMemory) {
				//printf("\n\nNEW SEG DAT\n\n");
				segmentationRequestSent = 0;
				MoveRover(mask, masterWrite);
			} else if (message.messageType == OperationMode) {
				// switch operating modes
				switch(opMode) {
					case Automatic:
						// switching from automatic to manual
						if (message.opModeMsg.opMode == Manual) {
							printf("switching to manual\n");
							opMode = Manual;
						}
						break;
					case Manual:
						// switching from manual to automatic
						if (message.opModeMsg.opMode == Automatic) {
							printf("switching to automatic\n");
							opMode = Automatic;
							COPY_POS(previousPosition, currentPosition);
							RequestSemSegData(masterWrite);
						}
						break;
				}
			} else if (TX2Comm == message.source && message.messageType == CANMessage) {
				// all manual controls must first pass through the navigation node to
				// keep them from interferring with automatic navigation
				if (opMode == Manual) {
					// in manual mode, ok to pass message off to CAN node
					message.source = TX2Nav;
					message.destination = TX2Can;
					message.canMsg.writeCount = 1;
					write(masterWrite, &message, sizeof(message));
				} else {
					// we are in automatic mode, don't do anything with message
					printf("attempting manual control when rover is in automatic mode\n");
				}
			} else if (message.messageType == PositionMessage && (message.source == TX2Comm || message.source == TX2Master)) {
				printf("setting destination position\n");
				COPY_POS(destinationPosition, message.positionMsg.position);
				//memset(&currentPosition, 0, sizeof(Position));
				//COPY_POS(previousPosition, currentPosition);
				atDestination = 0;
				if (!segmentationRequestSent) {
					RequestSemSegData(masterWrite);
				}
			} else if (message.messageType == ParametersMessage) {
				printf("READING NEW PARAMETERS\n");
				// we received command to re populate parameters struct
				status = GetParameters(PARAMETERS_FILE, &parameters);
				if (-1 == status) {
					printf("ERROR OPENING PARAMETERS FILE\n");
					return -1;
				}
				// print to screen for verification of new values
				PrintParameters(&parameters);
				// set new values for previous values structs
				SetMaxCount(&centerValues, parameters.centerDotProductValueCount);
				SetMaxCount(&leftValues, parameters.sideDotProductValueCount);
				SetMaxCount(&rightValues, parameters.sideDotProductValueCount);
				ClearValues(&centerValues);
				ClearValues(&leftValues);
				ClearValues(&rightValues);
			}
			else if (message.messageType == KillMessage) {
				// we received a kill message
				killMessageReceived = 1;
				CloseSharedMemory();
				close(masterRead);
				close(masterWrite);
			}
		}
	}

	printf("killing nav node\n");
	return 0;
}
