/**
 * @file FilterGen.h
 * @author Patrick Henz
 * @date 12-2-2019
 * @brief Header file for the FilterGen library.
 * @details This is the header file for the FilterGen library. This contains the function declarations
 *	    macros, and typedefs used by the FilterGen. These filters are used by #tx2_nav_node.c to 
 *	    to generate filters that help to interpret the incoming data from the semantic segmentation
 *	    netowrk used in #tx2_cam_node.cpp. The filters are triangular in nature and are meant to be 
 *	    used in a dot product operation with the semantic segmentation data to condense the data down
 *	    into single values.
**/

#ifndef FILTERGEN_H
#define FILTERGEN_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define FILTER_TYPE float
#define VALUES_TO_AVG 10

typedef struct previousValues {
	unsigned int count;
	unsigned int head;
	unsigned int maxCount;
	FILTER_TYPE values[VALUES_TO_AVG];
} PreviousValues; /**< Used to store previous values from the dot product operation, as to perform a moving average */

/**
 * @brief Clears the values from the #PreviousValues struct.
 * @details Clears the values from the #PreviousValues struct. The actual values in memory are not cleared, rather count
 *	    is set to 0.
 * @param previousValues A pointer to the #PreviousValues struct being cleared. 
 * @post The count attribute in the #PreviousValues struct will have been set to 0, effectively clearing out previously 
 *	 stored values.
**/
void ClearValues(PreviousValues * previousValues);

/**
 * @brief Initializes the maxCount field of a #PreviousValues struct.
 * @details This function sets the maxCount field of the #PreviousValues struct
 *	    that is passed in as a parameter. In order for a #PreviousValues struct
 *	    to be used, this function must be called first.
 * @param previousValues A pointer to the #PreviousValues struct whose maxCount value is to be set.
 * @param maxCount The value that will be assigned to maxCount.
 * @post The maxCount field in the #PreviousValues struct will have been set to maxCount. This value
 *	 should be less than the macro #VALUES_TO_AVG, which defines the size of the array in the
 *	 #PreviousValues struct.
**/
void SetMaxCount(PreviousValues * previousValues, unsigned int maxCount);

/**
 * @brief EnterNewValue enters a new value into the #PreviousValues struct.
 * @details EnterNewValue enters a new value into the #PreviousValues struct. It utilizes the 
 *	    maxCount field to index into the values array in #PreviousValues as if it were a ring buffer.
 * @param previousValues The #PreviousValues struct which the new value is being added to.
 * @param value The value being added to the #PreviousValues struct.
 * @pre Assumes that #SetMaxCount() has been called and maxCount has been properly set.
 * @post The new value will be addded to the #PreviousValues struct in ring buffer fashion, as defined by maxCount.
**/
void EnterNewValue(PreviousValues * previousValues, FILTER_TYPE value);

/**
 * @brief Checks to make sure enough values are present to perform moving average.
 * @details This function checks to make sure there are enough values in the #PreviousValues
 *	    struct to perform a moving average. This simply checks if maxCount == count and
 *	    returns the result. This allows for a definable averaging count. 
 * @param previousValues The #PreviousValues struct whose count is being checked.
 * @return Returns 1 if maxCount == count, 0 if the expression is false.
**/
int EnoughDataPresent(PreviousValues * previousValues);

/**
 * @brief Performs the average on the values in the #PreviousValues struct.
 * @details Performs the average on the values in the #PreviousValues struct. This function
 * 	    assumes that #EnoughDataPresent() was called prior and returned the value 1.
 * @param previousValues The #PreviousValues struct whose average is being calculated.
 * @return Returns the result of the average.
 * @pre GetMovingAverage() assumes that #EnoughDataPresent() has been called and returned the
 *	value 1. This function will work if #EnoughDataPresent() is not called first, but will
 *	only average over the current value count. Does not check if count == 0.
**/
FILTER_TYPE GetMovingAverage(PreviousValues * previousValues);

/**
 * @brief Creates the left filter for the semantic segmentation data.
 * @details CreateLeftFilter creates the left filter for the semantic segmentation data. The filter is
 *	    treated as a two-dimensional array, though it CANNOT be accessed as one since the memory was
 *	    dynamically allocated. Check tx2_nav_node.c to see how these memory locations are accessed. The
 *	    arrays are triangular and comprised of zero and non-zero values of type #FILTER_TYPE. The non-zero elements are
 *	    logarithmic in nature, with a base defined from filterHeight. The closer to the bottom of the
 *	    matrix, the closer the value is to 1, the closer to row 0 the closer the value to 0. The following is an
 *	    example of a filter, please note that the non-zero elements 1s to aid in readability.
 *	    <center> 
 * 	    00000000000000000000000000000000000000000000000000000<br>
 * 	    00111111111111111111111111000000000000000000000000000<br>
 * 	    00001111111111111111111111000000000000000000000000000<br>
 * 	    00000011111111111111111111000000000000000000000000000<br>
 * 	    00000000111111111111111111000000000000000000000000000<br>
 * 	    00000000001111111111111111000000000000000000000000000<br>
 * 	    00000000000011111111111111000000000000000000000000000<br>
 * 	    00000000000000111111111111000000000000000000000000000<br>
 * 	    00000000000000001111111111000000000000000000000000000<br>
 * 	    00000000000000000011111111000000000000000000000000000<br>
 * 	    00000000000000000000111111000000000000000000000000000<br>
 * 	    00000000000000000000001111000000000000000000000000000<br>
 * 	    00000000000000000000000011000000000000000000000000000<br>
 * 	    00000000000000000000000000000000000000000000000000000<br>
 * 	    00000000000000000000000000000000000000000000000000000<br>
 *	    </center>
 * 
 * @param array A pointer to a pointer of type #FILTER_TYPE. This function assumes the memory for array
 *	        has not yet been allocated and performs the allocation as well, hence the double pointer.
 * @param width Defines the width of the non-zero elements in row 0 of the filter.
 * @param height Defines the height of the non-zero elements in the filter.
 * @param filterWidth Defines the width of the filter.
 * @param filterHeight Defines the height of the filter.
 * @return Returns the number of non-zero elements in the array. This is used to "average" the result
 *	   of the dot product.
 * @pre The double pointer to array has not been initialized.
 * @post The memory needed for the filter will have been dynamically allocated and the filter will have
 *	 created inside the memory given to array.
**/
int CreateLeftFilter(FILTER_TYPE ** array, int width, int height, int filterWidth, int filterHeight);

/**
 * @brief Creates the right filter for the semantic segmentation data.
 * @details CreateRightFilter creates the right filter for the semantic segmentation data. The filter is
 *	    treated as a two-dimensional array, though it CANNOT be accessed as one since the memory was
 *	    dynamically allocated. Check tx2_nav_node.c to see how these memory locations are accessed. The
 *	    arrays are triangulari and comprised of zero and non-zero values of type #FILTER_TYPE. The non-zero elements are
 *	    logarithmic in nature, with a base defined from filterHeight. The closer to the bottom of the
 *	    matrix, the closer the value is to 1, the closer to row 0 the closer the value to 0. The following is an
 *	    example of a filter, please note that the non-zero elements 1s to aid in readability.
 *	    <center> 
 * 	    00000000000000000000000000000000000000000000000000000<br>
 *          00000000000000000000000000011111111111111111111111100<br>
 *          00000000000000000000000000011111111111111111111110000<br>
 *          00000000000000000000000000011111111111111111111000000<br>
 *          00000000000000000000000000011111111111111111100000000<br>
 *          00000000000000000000000000011111111111111110000000000<br>
 *          00000000000000000000000000011111111111111000000000000<br>
 *          00000000000000000000000000011111111111100000000000000<br>
 *          00000000000000000000000000011111111110000000000000000<br>
 *          00000000000000000000000000011111111000000000000000000<br>
 *          00000000000000000000000000011111100000000000000000000<br>
 *          00000000000000000000000000011110000000000000000000000<br>
 *          00000000000000000000000000011000000000000000000000000<br>
 *          00000000000000000000000000000000000000000000000000000<br>
 *          00000000000000000000000000000000000000000000000000000<br>
 *	    </center>
 * 
 * @param array A pointer to a pointer of type #FILTER_TYPE. This function assumes the memory for array
 *	        has not yet been allocated and performs the allocation as well, hence the double pointer.
 * @param width Defines the width of the non-zero elements in row 0 of the filter.
 * @param height Defines the height of the non-zero elements in the filter.
 * @param filterWidth Defines the width of the filter.
 * @param filterHeight Defines the height of the filter.
 * @return Returns the number of non-zero elements in the array. This is used to "average" the result
 *	   of the dot product.
 * @pre The double pointer to array has not been initialized.
 * @post The memory needed for the filter will have been dynamically allocated and the filter will have
 *	 created inside the memory given to array.
**/
int CreateRightFilter(FILTER_TYPE ** array, int width, int height, int filterWidth, int filterHeight);

/**
 * @brief Creates the center filter for the semantic segmentation data.
 * @details CreateCenterFilter creates the center filter for the semantic segmentation data. The filter is
 *	    treated as a two-dimensional array, though it CANNOT be accessed as one since the memory was
 *	    dynamically allocated. Check tx2_nav_node.c to see how these memory locations are accessed. The
 *	    arrays are triangulari and comprised of zero and non-zero values of type #FILTER_TYPE. The non-zero elements are
 *	    logarithmic in nature, with a base defined from filterHeight. The closer to the bottom of the
 *	    matrix, the closer the value is to 1, the closer to row 0 the closer the value to 0. The following is an
 *	    example of a filter, please note that the non-zero elements 1s to aid in readability.
 *	    <center> 
 *          00000000000000000000000000000000000000000000000000000<br>
 *          00000000000000000000000111111100000000000000000000000<br>
 *          00000000000000000000000111111110000000000000000000000<br>
 *          00000000000000000000001111111110000000000000000000000<br>
 *          00000000000000000000001111111111000000000000000000000<br>
 *          00000000000000000000011111111111000000000000000000000<br>
 *          00000000000000000000011111111111100000000000000000000<br>
 *          00000000000000000000111111111111100000000000000000000<br>
 *          00000000000000000000111111111111110000000000000000000<br>
 *          00000000000000000001111111111111110000000000000000000<br>
 *          00000000000000000001111111111111111000000000000000000<br>
 *          00000000000000000011111111111111111000000000000000000<br>
 *          00000000000000000011111111111111111100000000000000000<br>
 *          00000000000000000111111111111111111100000000000000000<br>
 *          00000000000000000111111111111111111110000000000000000<br>
 *	    </center>
 * @param array A pointer to a pointer of type #FILTER_TYPE. This function assumes the memory for array
 *	        has not yet been allocated and performs the allocation as well, hence the double pointer.
 * @param width Defines the width of the non-zero elements in row 0 of the filter.
 * @param height Defines the height of the non-zero elements in the filter.
 * @param filterWidth Defines the width of the filter.
 * @param filterHeight Defines the height of the filter.
 * @return Returns the number of non-zero elements in the array. This is used to "average" the result
 *	   of the dot product.
 * @pre The double pointer to array has not been initialized.
 * @post The memory needed for the filter will have been dynamically allocated and the filter will have
 *	 created inside the memory given to array.
**/
int CreateCenterFilter(FILTER_TYPE ** array, int flair, int width, int height, int filterWidth, int filterHeight);

// Test function that prints filters to terminal
int PrintFilter(FILTER_TYPE * arry, int width, int height);

#endif
