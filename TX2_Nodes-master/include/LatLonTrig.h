/**
 * @file LatLonTrig.h
 * @author Patrick Henz
 * @date 10/9/2019
 * @brief Header file with declaration for latitude and longitude directional controls
 * @details Contains function declarations for latitude and longitude directional control
**/

#ifndef LATLONTRIG_H
#define LATLONTRIG_H

#include <math.h>
#include "Messages.h"
#include <stdio.h>

/**
 * @brief Returns the difference in latitude between p1 and p2
**/
#define LAT_DIFF(p1, p2) (p1.latitude - p2.latitude)
/**
 * @brief Returns the difference in longitude between p1 and p2
**/
#define LON_DIFF(p1, p2) (p1.longitude - p2.longitude)

/**
 * @brief Macro provides a wrapper for the pow function, specifially to square the passed in value.
**/
#define SQUARE(n) pow((n), 2)

#define RADIUS_OF_EARTH 6371000.0f 

/**
 * @brief Subtracts prev from pos.
 * @details Subtracts prev from pos. This is used to center the previous position at the origin when
 * 	    determining angle to turn.
**/
#define PREV_TO_ORIG(pos, prev) (pos.latitude -= prev.latitude);\
				(pos.longitude -= prev.longitude)

#define PI (3.141592f)

/**
 * @brief Converts an angle in degrees to radians.
**/
#define TO_RAD(d) ((d) * (PI / 180.0f))

/**
 * @brief Calculates the distance between position1 and position2 in meters.
 * @details Calculates the distance between position1 and position2. At the moment, this function
 * 	    uses the Haversine equation to calculate the closest distance between two points on
 * 	    a sphere. The closer one is to the Earths poles, the smaller the distance between 
 * 	    longitude deggrees. Because of this, lat/lon can not be treated as Cartesian coordinates,
 * 	    thus, the use of the Haversine function.
 * @param position1 #Position struct for 1st position.
 * @param position2 #Position struct for 2nd position.
 * @return Returns the distance between the two positions in meters.
**/
float Distance(Position position1, Position position2);

/**
 * @brief Returns the angle and direction the rover needs to turn to face the destination point.
 * @details DegreeTurnAndDirection returns the degree value the rover needs to turn, as well as the
 * 	    direction in which the turn needs to be made; negative returned if the rover needs to
 * 	    turn left, positive value if the rover needs to turn right. This function assumes the
 * 	    rover has been traveling in a straight line between currentPosition and previousPosition,
 * 	    having made no turns. The law of Cosines is used to determine the angle.
 * @param currentPosition The most recent recorded #Position of the rover.
 * @param previousPosition The last #Position the rover was at after a turn or previousPosition 
 * 		           initialization.
 * @param destinationPosition The destination #Position the rover is navigating to.
 * @return Returns the angle in degrees that the rover needs to turn. If the turn is a left turn, the
 *         the return value is negative. If the value is positive, the turn is a right turn.
**/
float DegreeTurnAndDirection(Position currentPosition,
			      Position previousPosition,
			      Position destinationPosition);

// function used to print position, used primarily for testing
void PrintPosition(Position * position);

#endif
