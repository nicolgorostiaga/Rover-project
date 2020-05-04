/**
 * @file FilterGen.c
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Function definition file for filter gen library.
 * @details Function definition file for filter gen library. Also provides definitions for averaging 
 *	    functionality.
**/

#include "../include/FilterGen.h"

void EnterNewValue(PreviousValues * previousValues, FILTER_TYPE value)
{
	// only increment count if we are considering list to not be "full"
	if (previousValues->count < previousValues->maxCount) {
		previousValues->count++;
	}

	// add value to the head of the list, increment the head
	// in a circular buffer fashion
	previousValues->values[previousValues->head] = value;
	previousValues->head = (previousValues->head + 1) % previousValues->maxCount;
}

void SetMaxCount(PreviousValues * previousValues, unsigned int maxCount)
{
	// set max count
	previousValues->maxCount = maxCount;
}

void ClearValues(PreviousValues * previousValues)
{
	// clear values
	previousValues->head = 0.0;
	previousValues->count = 0.0;
}

int EnoughDataPresent(PreviousValues * previousValues)
{
	// is enough data present according max count?
	return (previousValues->count == previousValues->maxCount)?(1):(0);
}

FILTER_TYPE GetMovingAverage(PreviousValues * previousValues)
{
	int i;
	FILTER_TYPE sum;

	sum = 0;

	// sum up all values stored in PreviousValues
	for (i = 0; i < previousValues->count; i++) {
		sum += previousValues->values[i];
	}

	// return average
	return (sum / previousValues->count);
}

int CreateLeftFilter(FILTER_TYPE ** array, int width, int height, int filterWidth, int filterHeight)
{
	int row, column;

	// determine the delta. This is the number of indices we move over to the left/right at each new row
	float deltaF = ((float) width/ (height - 1)) + 0.5f;
	int delta = (int)deltaF;

	int remainingEntries = 0;

	int middle = filterWidth / 2;

	int bufferSpace;

	int area = 0;

	// allocate memory for double pointer
	*array = malloc(sizeof(FILTER_TYPE)*filterWidth*filterHeight);	

	if (NULL == *array) {
		printf("Could not allocate meory\n");
		return -1;
	}

	// loop through and create left filter
	for (row = 0; row < filterHeight; row++) {
		// determine the number of non-zero elements at the current row
		remainingEntries = width - (row * delta);
		// zero spaces
		bufferSpace = middle - remainingEntries;
		for (column = 0; column < filterWidth; column++) {
			if (bufferSpace > 0) {
				// give buffer spaces zeros
				(*array)[(filterWidth * row) + column] = 0;
				bufferSpace--;
			} else if (remainingEntries > 0) {
				// performing a change of log operation. The base of the log is the number
				// of rows in the filter
				(*array)[(filterWidth * row) + column] = (log(row + 1) / log(filterHeight));
				remainingEntries--;
				area++;
			} else {
				// we are past buffer space and all non-zero entries have been created
				(*array)[(filterWidth * row) + column] = 0;
			}
		}
	}
	return area;
}

int CreateRightFilter(FILTER_TYPE ** array, int width, int height, int filterWidth, int filterHeight)
{
	int row, column;

	// determine the delta. This is the number of indices we move over to the left/right at each new row
	float deltaF = ((float) width/ (height - 1)) + 0.5f;
	int delta = (int)deltaF;

	int remainingEntries = 0;

	int middle = filterWidth / 2;

	int bufferSpace;

	int area = 0;

	// allocate memory for double pointer
	*array = malloc(sizeof(FILTER_TYPE)*filterWidth*filterHeight);	

	if (NULL == *array) {
		printf("Could not allocate meory\n");
		return -1;
	}

	// loop through and create right filter
	for (row = 0; row < filterHeight; row++) {
		// determine the number of non-zero elements at the current row
		remainingEntries = width - (row * delta);
		// zero spaces
		bufferSpace = middle - remainingEntries;
		for (column = filterWidth - 1; column >= 0; column--) {
			if (bufferSpace > 0) {
				// give buffer space zeros
				(*array)[(filterWidth * row) + column] = 0;
				bufferSpace--;
			} else if (remainingEntries > 0) {
				// performing a change of log operation. The base of the log is the number
				// of rows in the filter
				(*array)[(filterWidth * row) + column] = (log(row + 1) / log(filterHeight));;
				remainingEntries--;
				area++;
			} else {
				// past buffer area and non-zero regions have been created
				(*array)[(filterWidth * row) + column] = 0;
			}
		}
	}
	return area;
}

int CreateCenterFilter(FILTER_TYPE ** array, int flair, int width, int height, int filterWidth, int filterHeight)
{
	int row, column;

	// determine the delta. This is the number of indices we move over to the left/right at each new row
	float deltaF = ((float) (width - flair)/ (height - 1)) + 0.5f;
	int delta = (int)deltaF;
	int remainingEntries;
	int bufferSpace;
	int area = 0;

	// allocate memory for double pointer
	*array = malloc(sizeof(FILTER_TYPE)*filterWidth*filterHeight);	

	if (NULL == *array) {
		printf("Could not allocate meory\n");
		return -1;
	}

	// loop through and create center filter
	for (row = 0; row < filterHeight; row++) {
		// determine the number of non-zero elements at the current row
		remainingEntries = flair + (delta * row);
		// zero spaces
		bufferSpace = (filterWidth - remainingEntries) / 2;
		for (column = filterWidth - 1; column >= 0; column--) { 
			if (bufferSpace > 0) {
				// give buffer spaces zeros
				(*array)[(filterWidth * row) + column] = 0;
				bufferSpace--;
			} else if (remainingEntries > 0) {
				// performing a change of log operation. The base of the log is the number
				// of rows in the filter
				(*array)[(filterWidth * row) + column] = (log(row + 1) / log(filterHeight));
				remainingEntries--;
				area++;
			} else {
				// past buffer area and non-zero regions have been created
				(*array)[(filterWidth * row) + column] = 0;
			}
		}
	}
	return area;
}

int PrintFilter(FILTER_TYPE * array, int width, int height)
{
	int r, c;

	// loop through filter and print out values
	for (r = 0; r < height; r++) {
		for (c = 0; c < width; c++) {
			printf("%.2f", array[(r * width) + c]);
		}
		printf("\n");
	}

	return 0;
}
