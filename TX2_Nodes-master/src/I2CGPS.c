/**
 * @file I2CGPS.c
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Function definitions for I2CGPS library.
 * @details Function definitions for I2CGPS library. This file also contains internal file descriptors
 *	    and functions used only by the functions within this library. It should be noted that communication
 *	    with the XA1110 GNSS module is not quite like traditional I2C communication, which often contains
 *	    mixed write/read commands. This is completely centered around a buffer of ASCII characters. The
 *	    GNSS module simply holds onto this buffer and sends this data back to the TX2 when a read command 
 *	    is sent. Write commands are similar, the sender sends packets of ASCII characters over to the 
 *	    GNSS module, without the need to set registers or destination addresses.
**/

#include "../include/I2CGPS.h"

/**
 * @brief I2C struct contains relevent information when reading and writing to XA1110 over I2C bus.
**/
typedef struct i2c {
	unsigned int bytes;
	char gpsBuffer[255];
} I2C;

/**
 * @brief Temporary buffer used by various functions when parsing NMEA data
 * @details Temporary buffer used by various functions when parsing NMEA data. Another important piece of 
 *	    functionality that this provides is to retain information in the scenario that we only receive
 *	    half of a data packet that contains information that we wnat. That data is stored here until the 
 *	    next read returns the rest of the information.
**/
char nmeaBuffer[128];

/**
 * @brief The size of the data in #nmeaBuffer.
**/
int nmeaBufferSize;

/**
 * @brief Used to keep track of the index inside fo #nmeaBuffer in the scenario that we received a split up
 *	  message.
**/
int previousIndex;

/**
 * @brief Internal file descriptor used by the I2CGPS.c functions.
**/
int I2CFd;

/**
 * @brief Pointer for #I2C struct, used as an internal global for this library.
**/
I2C * i2cData;

/**
 * @brief No GPS fix displayed flag
**/
int messageDisplayed = 0;

/**
 * @brief Internal I2C write function 
 * @details Internal I2C write function 
 * @param i2c #I2C struct whose data will be written to the I2C bus.
 * @return I2C write status
**/
int i2cWrite(I2C * i2c)
{
	int status;
        do {
		// try and write to GNSS device
		status = write(I2CFd, i2c->gpsBuffer, i2c->bytes);
		// if we try and write while device isn't ready, wait and try again
		if (-1 == status) {
			usleep(10000);
		}
        } while (-1 == status);
	return status;
}

/**
 * @brief Internal I2C read function 
 * @details Internal I2C read function 
 * @param i2c #I2C struct that will contain data read from the bus.
 * @return I2C read status
**/
int i2cRead(I2C * i2c)
{
	int status;
	do {
		// try and read from the GNSS device
		status = read(I2CFd, i2c->gpsBuffer, sizeof(i2c->gpsBuffer));
		// if we try and read while device isn't ready, wait and try again
		if (-1 == status) {
			usleep(10000);
		}
	} while (-1 == status);
	// status contains the number of bytes read
	i2c->bytes = status;
	return status;
}

/**
 * @brief NMEA checksum calculator.
 * @detaisl NMEA checksum claculator. This function takes the GPS string input and calculates the checksum
 *	    that should be appeneded to the end of the string. This is used both when creating a checksum and
 * 	    when checking a checksum from a read in NMEA packet.
 * @param gpsString The string of ASCII characters whose checksum is being calculated.
 * @param index This is the address of an integer used to keep track of index. If we are calculating a checksum
 *	   	to append to a string, this index helps keep track of where to ultimately place the checksum in
 *		the array. If that functionality is not desired, passing in NULL is checked and the memory is
 *		dynamically allocated.
**/
char CalculateChecksum(char * gpsString, int * index)
{
	char checkSum = 0x00;
	int memAllocated = 0;

	// address wasn't passed in, allocate memory and set flag
	if (index == NULL) {
		index = malloc(sizeof(int));
		memAllocated = 1;
	}

	// initialize to start of array
        *index = 0;

	// traverse through string, calculate checksum
	// the NMEA checksum is simply the result of XORING the characters
	// in the string between the '$' and '*' characters. 
	// NOTE: The calculated checksum is in a 8-bit hex format. When appeneded however, it
	// is the ASCII representation that is appeneded.
	// As an example, if we XORd ASCII characters together and ended up with the value 0x7A, we would
	// append the characters '7''A' to the string.
	while (gpsString[*index] != '*') {
		if (gpsString[*index] != '$'){
			checkSum ^= gpsString[*index];
		}
		(*index)++;
	}

	// increment once more, this would be the starting point of the checksum
	(*index)++;

	// if memory was allocated, free it
	if (memAllocated) {
		free(index);
	}

	return checkSum;
}

/**
 * @brief Internal function used to compare checksums.
 * @details This internal function is used to check that the checksum in a NMEA packet is correct.
 * @param gpsString The NMEA packet whose checksum is being checked.
 * @return Returns 0 if the checksum in the NMEA packet matches the checksum as calculated by the function
 *	   #CalculateChecksum.
**/
int CompareChecksum(char * gpsString)
{
	char providedChecksum = 0x00;
	char offset = 0x00;
	char checkSum;
	int i;

	// calculate the checksum
	checkSum = CalculateChecksum(gpsString, &i);

	// this turnary operation accounts for the offset between '9' and 'A' in the 
	// ASCII table. We are dealing with hexadecimal values in an ASCII format.
	offset = (gpsString[i] > '9')?('7'):('0');
	providedChecksum |= (gpsString[i] - offset) << 4;

	// increment to next character
	i++;

	// again, checking to see what our offset value is to account for hex values in an
	// ASCII format
	offset = (gpsString[i] > '9')?('7'):('0');
	providedChecksum |= (gpsString[i] - offset);

	// return result
	return (checkSum == providedChecksum)?(0):(-1);
}

/**
 * @brief Internal function used to determine at what index in the array an element in the NMEA packet resides.
 * @details Internal function used to determine index in array of NMEA element. Values in NMEA strings are
 *	    comma delimited, which this function uses to its advantage.
 * @param gpsString The NMEA string we are parsing.
 * @elementIndex The index in the NMEA packet (not array!) which the element resides
 * @return Returns the index into the gpsString that the element we are looking for resides.
**/
int GetIndexOfElement(char * gpsString, int elementIndex)
{
        int characterIndex = 0;
        int commaCount = 0;

	// traverse through string until we are at the specified element
        while (commaCount < elementIndex) {
		// increment only at comma
                if (',' == gpsString[characterIndex]) {
                        commaCount++;
                }
                characterIndex++;
        }

	// return result
        return characterIndex;
}

/**
 * @brief Internal function used to extract latitude and longitude from NMEA packet.
 * @details Internal function used to extract latitude and longitude from NMEA packet. For this project, I 
 *	    have been parsing GNGLL packets. An example is as follows.
 *	    <br>
 *	    <br>
 *	    <center>
 *
 *	    $GNGLL,3150.679234,N,11711.934544,E,032946.000,A,A*4C
 * 
 *	   </center>
 * @param gpsString The NMEA packet we are extracting the coordinate from.
 * @param latitude A pointer to a float variable, used as an output.
 * @param longitude A pointer to a float variable, used as an output.
 * @return Returns 0 if successful, -1 if error.
**/
int GetLatLong(char * gpsString, float * latitude, float * longitude)
{
	float tempFloat;

	// latitude is the first element in the NMEA packet
        int index = GetIndexOfElement(gpsString, 1);

	// check for various conditions
        if (gpsString[index] == 'A') {
		index = index + 2;
        } else if (gpsString[index] == 'V') {
		printf("Invalid GPS data\n");
                return -1;
        } else if (gpsString[index] == ',') {
		// this is the most likely condition to occur. In the scenario that the GNSS module does not
		// have a satellite fixe, the strings it returns will be empty and filled with commas. If we 
		// are at the first index, and we are on a comma, we know there is not fix.
		if (!messageDisplayed) {
			printf("No GPS Signal/Lock\n");
			messageDisplayed = 1;
		}
		return -1;
	}

	messageDisplayed = 0;

	// first two digits in the string are already in degree format. Simply convert to integers
        *latitude = ((gpsString[index] - '0') * 10) + (gpsString[index+1] - '0');
        index = index + 2;
	// from this point, the rest of the value in the string is in minutes format. To conver to degrees
	// we need to extract the entierty of the float value and divide by 60, since 60 minutes to a degree
        tempFloat = ((gpsString[index] - '0') * 10) + (gpsString[index+1] - '0');
        index = index + 3;
        tempFloat += ((gpsString[index++] -'0') * 0.1f);
        tempFloat += ((gpsString[index++] -'0') * 0.01f);
        tempFloat += ((gpsString[index++] -'0') * 0.001f);
        tempFloat += ((gpsString[index++] -'0') * 0.0001f);
        tempFloat += ((gpsString[index++] -'0') * 0.00001f);
        tempFloat += ((gpsString[index++] -'0') * 0.000001f);
        index++;
        
	// convert from mintues to degrees, append
        *latitude += tempFloat/60.0f;
 
	// figure out if we are in the southern hemisphere
        if (gpsString[index] == 'S') {
                *latitude = -(*latitude);
        }

	// increment to longitude
        index = index + 2;

	// this time, the first 3 digits are already in degree format
        *longitude = ((gpsString[index] - '0') * 100) + 
                     ((gpsString[index+1] - '0') * 10) + 
                      (gpsString[index+2] - '0');
        index = index + 3;
	// same as with latitude, we need to convert this from minutes to degrees. Extract the float value
        tempFloat = ((gpsString[index] - '0') * 10) + (gpsString[index+1] - '0');
        index = index + 3;
        tempFloat += ((gpsString[index++] -'0') * 0.1f);
        tempFloat += ((gpsString[index++] -'0') * 0.01f);
        tempFloat += ((gpsString[index++] -'0') * 0.001f);
        tempFloat += ((gpsString[index++] -'0') * 0.0001f);
        tempFloat += ((gpsString[index++] -'0') * 0.00001f);
        tempFloat += ((gpsString[index++] -'0') * 0.000001f);
        index++;
        
	// convert from minutes to degrees
        *longitude += tempFloat/60.0f;

	// determine if we are to the east or west
        if (gpsString[index] == 'W') {
                *longitude = -(*longitude);
        }

	return 0;
}

/**
 * @brief Internal function used to append checksum.
 * @details Internal function used to append checksums to a NMEA string. Typically used when sending
 *	    commands over to GNSS module.
 * @param gpsCommand The NMEA string the checksum is being appened to.
 * @param i2c The internal #I2C struct whose buffer the gpsString and checksum are being written to.
 i @post the incoming gpsCommand string will have a checksum appended and stored in the #I2C pointer buffer.
**/ 
int AppendChecksum(char * gpsCommand, I2C * i2c)
{
	int index;
	sprintf(i2c->gpsBuffer, "%s%X\r\n", gpsCommand, CalculateChecksum(gpsCommand, &index));
	// accounts for checksum, carriage return, and line feed
	i2c->bytes = index + 4;
}

int I2CGPSOpen() 
{
	char filename[32];
	int deviceAddress = 0x10;

	memset(filename, 0, sizeof(filename));

	// linux device path
	sprintf(filename, "%s", "/dev/i2c-1");

	I2CFd = open(filename, O_RDWR);

	if (I2CFd <= 0) {
		printf("Error opening I2C device\n");
		return -1;
	}

	// attach I2C device slave address
	if (ioctl(I2CFd, I2C_SLAVE, deviceAddress) < 0) {
		printf("Error setting I2C address\n");
		return -1;
	}

	// allocate memory for #I2C data struct
	i2cData = malloc(sizeof(I2C));

	memset(i2cData, 0, sizeof(I2C));

	previousIndex = 0;

	return I2CFd;
}

/**
 * @brief Internal function used to parse returned NMEA data to find the lat/lon data we desire.
 * @details Internal function used to parse returned NMEA data to find the lat/lon data we desire.
 * @param nmeaData The NMEA string we are parsing. If we find the packet is a GNGLL packet, return 1, else 0.
 * @return If packet is GNGLL, return 1, else 0
**/ 
int IsGNGLL(char * nmeaData)
{
	int index;
	int equal;
	char * GNGLL = "$GNGLL";
	// just look through first 6 characters of each string
	for (index = 0; index < 6; index++) {
		equal = (nmeaData[index] == GNGLL[index]);
		// if not equal, we are done here
		if (!equal) break;
	}

	return equal;
}

/**
 * @brief Internal function used to parse NMEA packets.
 * @details Internal function used to pares NMEA packets. As a read from the XA1110 module typically
 *	    returns a plethora of NMEA packets, we need to extract individual packets one at a time and
 *	    determine if they contain any information that we want.
 * @param input The NMEA string we are parsing
 * @return Returns 0 if we found a GNGLL packet, else returns -1. 
**/
int ParseNMEA(char * input)
{
	int index = 0;
	int endOfPacket = 0;

	// loop through whole string if necessary
	while (index < 255) {
		nmeaBuffer[previousIndex] = input[index];
		previousIndex++;
		// mark end of packet
		if (input[index] == '*') {
			endOfPacket = 1;
		}

		// we have reached end of the input string
		if (input[index++] == '\n') {
			// do we have the data we need
			if (endOfPacket && IsGNGLL(nmeaBuffer)) {
				nmeaBufferSize = previousIndex;	
				previousIndex = 0;
				return 0;
			 }	
			previousIndex = 0;
			endOfPacket = 0;
		}
	}

	// data at the cutoff is important
	if (index == 255 && IsGNGLL(nmeaBuffer) && !endOfPacket) {
		//previousIndex = previousIndex - 1; // account for CR and LF 
	} else {
		// discard it entirley, should ever hit this.... use an assert
		previousIndex = 0;
	}
	return -1;		
}

int I2CGPSRead(Message * message)
{
	int parseStatus;
	i2cRead(i2cData);

	// read in NMEA data from XA1110 and parse
	parseStatus = ParseNMEA(i2cData->gpsBuffer);

	// if we found what we want, return 1
	if (0 == parseStatus &&  // we have GNGLL data
	    0 == CompareChecksum(nmeaBuffer) && // Checksum mathes
	    0 == GetLatLong(nmeaBuffer, &message->gpsMsg.position.latitude, // lat lon correct
		    			&message->gpsMsg.position.longitude)) {
		return 1;
	} else {
		return 0;	// data, for whatever reason, isn't avialable
	}
}

int I2CGPSWrite(char * command)
{
	// append checkusm
	AppendChecksum(command, i2cData);
	// write to the XA1110
	i2cWrite(i2cData);
	return 0;
}

void CloseI2C()
{
	close(I2CFd);
	free(i2cData);
}
