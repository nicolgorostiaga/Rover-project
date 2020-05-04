/**
 * @file Parameters.c
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Function definitions for Parameters library.
 * @details Function definitions for Parameters library.
**/

#include "../include/Parameters.h"

/**
 * @brief Internal function used to read through file until next value is reached.
 * @details Internal function used to read through file until next value is reached.
 *	    The values in these viles are colon delimited, and are comprised of human 
 *	    readable ascii values. As an example, here is a snippet from Parameters.txt.
 *	    <br><br><center>
 *          centerDotProductValueCount     :1
 *	    turningWeight                  :0.70
 *	    distanceFromPreviousThreshold  :5.25
 *	    <br><br></center>
 *	    This function simply reads up to the next value.
 * @param fd The file descriptor for Parameters.txt.
**/
void GetToNextValue(int fd) {
	char temp;

	read(fd, &temp, 1);

	// read until next value or EOF
	while (!EOF || temp != ':') {
		read(fd, &temp, 1);
        }
}

/**
 * @brief Internal function used to convert from ASCII to float
 * @details Internal function used to convert from ASCII to float.
 * @param fd File descriptor for Parameters.txt.
 * @return Returns the converted float value.
 * @pre Expects the Parameters.txt read pointer to have been incremented 
 *	to a value that is a floating point value in ASCII format.
**/
float GetFloat(int fd)
{
	float floatTemp;
	char tempChar;
	float divider = 1.0f;
	float sign;

	floatTemp = 0.0f;

	read(fd, &tempChar, 1);	

	// is the value negative
	if ('-' == tempChar) {
		sign = -1.0f;
	} else {
		sign = 1.0f;
	}

	// get whatever is too the left of the decimal point
	while (tempChar != '.') {
		floatTemp += (floatTemp*10) + (tempChar - '0');
		read(fd, &tempChar, 1);
	}

	read(fd, &tempChar, 1);
	
	// values are terminated via a new line character
	// read in digits after the decimal point until this is reached.
	while(tempChar != '\n') {
		divider *= 0.1f;
		floatTemp += divider*(tempChar - '0');
		read(fd, &tempChar, 1);
	}

	// return converted value
	return (sign*floatTemp);
}

/**
 * @brief Internal function used to convert from ASCII to int. 
 * @details Internal function used to convert from ASCII to int.
 * @param fd File descriptor for Parameters.txt.
 * @return Returns the converted int value.
 * @pre Expects the Parameters.txt read pointer to have been incremented 
 *	to a value that is an integer value in ASCII format.
**/
int GetInt(int fd)
{
	int tempInt;
	int sign;
	char tempChar;

	tempInt = 0;

	read(fd, &tempChar, 1);

	// is the value negative?
	if (tempChar == '-') {
		sign = -1;
		read(fd, &tempChar, 1);
	} else {
		sign = 1;
	}

	// loop through until end of line and convert to binary
	while(tempChar != '\n') {
		tempInt = (tempInt * 10) + (tempChar - '0');
		read(fd, &tempChar, 1);	
	}

	// return value
	return (sign*tempInt);
}

int GetParameters(char * fileName, Parameters * parameters)
{
	int fd;

	// open Parameters.txt
	fd = open(fileName, O_RDONLY);

	if (fd <= 0) {
		return -1;
	}

	// Note that the values are expected to be in this order, and
	// the order is not in any way validated. If something is out 
	// of place, this will cease to work properly.
	GetToNextValue(fd);
	parameters->distanceToGoThreshold = GetFloat(fd);
	GetToNextValue(fd);
	parameters->distanceFromStartThreshold = GetFloat(fd);	
	GetToNextValue(fd);
	parameters->angleToTurnThreshold = GetFloat(fd);
	GetToNextValue(fd);
	parameters->dotProductThreshold = GetFloat(fd);
	GetToNextValue(fd);
	parameters->sideDotProductValueCount = GetInt(fd);
	GetToNextValue(fd);
	parameters->centerDotProductValueCount = GetInt(fd);
	GetToNextValue(fd);
	parameters->turningWeight = GetFloat(fd);
	GetToNextValue(fd);
	parameters->distanceFromPreviousThreshold = GetFloat(fd);
	GetToNextValue(fd);
	parameters->turningAngle = GetFloat(fd);
	GetToNextValue(fd);
	parameters->multiTurnThreshold = GetFloat(fd);
	GetToNextValue(fd);
	parameters->usingGps = GetInt(fd);
	GetToNextValue(fd);
	parameters->manual = GetInt(fd);

	close(fd);

	return 0;
}

void PrintParameters(Parameters * parameters)
{
	printf("dotProductThreshold = %.4f\n", parameters->dotProductThreshold);
	printf("distanceToGoThreshold = %.4f\n", parameters->distanceToGoThreshold);
	printf("distanceFromStartThreshold = %.4f\n", parameters->distanceFromStartThreshold);
	printf("angleToTurnThreshold = %.4f\n", parameters->angleToTurnThreshold);
	printf("sideDotProductValueCount = %d\n", parameters->sideDotProductValueCount);
	printf("centerDotProductValueCount = %d\n", parameters->centerDotProductValueCount); 
	printf("turningWeight %.4f\n", parameters->turningWeight);
	printf("distanceFromPreviousThreshold %.4f\n", parameters->distanceFromPreviousThreshold);
	printf("turningAngle = %.6f\n", parameters->turningAngle);
	printf("multiTurnThres = %.6f\n", parameters->multiTurnThreshold);
	printf("usingGps = %s\n", (parameters->usingGps)?("True"):("False"));
	printf("manual = %s\n", (parameters->manual)?("True"):("False"));
}
