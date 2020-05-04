/**
 * @file Parameters.h
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Header file for Parameters library.
 * @details Header file for Parameters library. At the moment, this library is used only by the
 * 	    tx2_nav_node.c. This allows navigation related information to be set in the text file
 * 	    Parameters.txt in a human readable format without the need to recompile the source code.
 * 	    The controller application, controller.c, can even send a parameter message to the rover
 * 	    prompting the navigation node to read in the Parameters.txt file again to load in values,
 * 	    meaning these values can be modified during run time, without rebuilding the rovers 
 * 	    sowftware or restarting the rover at all.
**/

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define PARAMETERS_FILE "../Parameters.txt"

/**
 * @brief This struct contains all parameters used by the tx2_nav_node.c during the navigation
 * 	  process.
 * @details This struct contains all parameters used by the tx2_nav_node.c during the navigation
 * 	    process. If the navigation node is prompted to do so by controller.c, these values are
 * 	    repopulated during runtime.
**/
typedef struct parameters {
	// threshold used for semantic segmentation dot product. If the dot product result is above
	// or equal to this value, the rover will likely try to turn
        float dotProductThreshold; 	
	// the distance between the rover and a destination position whic defines whether or not
	// the rover has reached said destination position
        float distanceToGoThreshold;		 
	// unused
        float distanceFromStartThreshold;
	// the distance between the current position and the previously recorded position. The
	// previously recorded position is typically recorded after the rover makes a turn. This
	// distance threshold determines how far the rover should move forward before trying to 
	// calculate a new GNSS turn angle. Once this threshold is met, the rover calculates how
	// much it needs to turn to stay on target.
	float distanceFromPreviousThreshold;
	// unused
        float angleToTurnThreshold;
	// used to weight the dot product result in the scenario that multiple turns are needed.
	float turningWeight;
	// unused
	float turningAngle;
	// unused
	float multiTurnThreshold;
	// defines how many dot product values are retained from the left and right semantic
	// segmentation filters. These counts are kept for averaging purposes.
        int   sideDotProductValueCount;
	// defines how many dot product values are retained from the center semantic segmentation
	// filter. Used for averaging.
	int   centerDotProductValueCount;
	// This acts as a flag, which if set, prompts the rover to use gps data when navigating. 
	// This is almost always set as navigation whithout gps is mostly pointless, with the
	// rover driving arround aimlessly. Give value of 1 to use gps, 0 to not use gps.
	int   usingGps;
	// flag that puts the rover in manual mode at startup. 1 for manual, 0 for automatic
	int   manual;
} Parameters;

/**
 * @brief Function call reads in new parameter values.
 * @details Calling this function reads in the file defined by fileName and parses it for
 * 	    #Parameters values. These values are then loaded into the parameters sturct which
 * 	    is pointed to by the incoming #Parameters address.
 * @param fileName Path to Parameters.txt file.
 * @param parameters Pointer to #Parameters struct whose values will be read in from the 
 * 	  Parameters.txt file.
 * @pre Expects the Parameters.txt file to be in a very specific order. Check Parameters.txt to see
 * 	the format. This function sees ':' as a delimeter, with anyting immediately following the
 * 	colon ccharacter as the value to be read in. This allows for Parameters.txt to be in a 
 * 	human readable format.
 * @post The #Parameters pointer will contain the values parsed from Parameters.txt.
**/
int GetParameters(char * fileName, Parameters * parameters);

/**
 * @breif Prints out data members of #Parameters pointer.
 * @details Prints out data members of #Parameters pointer. Used primarily for testing purposes.
**/
void PrintParameters(Parameters * parameters);

#endif
