/**
 * @file LatLonTrig.c
 * @author Patrick Henz
 * @date 10/9/2019
 * @brief Function definitions for latitude and longitude directional controls
 * @details Contains function definitions for latitude and longitude directional control
**/

/**
 * TO COMPILE THIS YOU MUST LINK WITH MATH LIBRARY
 * gcc [flags] <files> -lm
 * WITHOUT -lm YOU WILL GET UNDEFINED REFERENCE ERROR
**/

#include "../include/LatLonTrig.h"

// using haversine formula
float Distance(Position position1, Position position2)
{
        float latitudeDelta = TO_RAD(position1.latitude - position2.latitude);
        float longitudeDelta = TO_RAD(position1.longitude - position2.longitude);
        float a, c;

        a = SQUARE(sin(latitudeDelta / 2.0f)) +
                (cos(TO_RAD(position1.latitude)) *
                 cos(TO_RAD(position2.latitude)) *
                 SQUARE(sin(longitudeDelta / 2.0f)));

        c = 2 * atan2(sqrt(a), sqrt(1-a));

        return RADIUS_OF_EARTH * c;
}

/**
 * @brief Interla function used to calculate angle based on three distances.
**/
float DegreeTurn(float a, float b, float c)
{
	float sign;
	float cosValue; 
	float divSlope;

	// law of cosines
	cosValue = -(SQUARE(c) - SQUARE(a) - SQUARE(b))/(2.0f*a*b);

	return acos(cosValue);
}

float DegreeTurnAndDirection(Position currentPosition, 
		 Position previousPosition, 
		 Position destinationPosition)
{
	float dTraveled, dPreviousToDestination, dCurrentToDestination;
	float degreesToTurn;
	float slope, pointOnLine;
	float sign;

	// move previousPositino to origin, adjust other positions as needed
	PREV_TO_ORIG(currentPosition, previousPosition);
	PREV_TO_ORIG(destinationPosition, previousPosition);
	PREV_TO_ORIG(previousPosition, previousPosition);

	// calculate the various distances for the law of cosines
	dTraveled = Distance(currentPosition, previousPosition);
	dPreviousToDestination = Distance(previousPosition, destinationPosition);
	dCurrentToDestination = Distance(currentPosition, destinationPosition);

	// we are forming a line with the current position through the axis (now previousPosition) to
	// help determine which way we need the rover to turn
	slope = currentPosition.latitude/currentPosition.longitude;	

	// where the destinationPositions longitude would be on the line
	pointOnLine = slope*destinationPosition.longitude;

	// the actual value from this operation is meaningless, only doing this to get
        // the sign figured out correctly...
        sign = (destinationPosition.latitude - pointOnLine) * currentPosition.latitude;

	// determine if we are turning left or right
        if (slope < 0) {
                sign = (sign < 0)?(-1.0f):(1.0f);
        } else {
                sign = (sign < 0)?(1.0f):(-1.0f);
        }

	// finish claculations, convert from radians to degrees and add correct sign
	return sign*(PI - DegreeTurn(dTraveled, dCurrentToDestination, dPreviousToDestination))*(180.0f/PI);
}

void PrintPosition(Position * position)
{
	printf("Latitude = %.6f, Longitude = %.6f\n", position->latitude, position->longitude);
}
