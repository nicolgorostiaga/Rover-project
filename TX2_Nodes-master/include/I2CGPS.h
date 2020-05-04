/**
 * @file I2CGPS.h
 * @author Patrick Henz
 * @date 12-2-2019
 * @brief Header file for the I2CGPS library.
 * @details Header file for the I2CGPS library. All function prototypes and macros are defined in this header file.
**/

#ifndef I2CGPS_H
#define I2CGPS_H

#include <string.h>
#include <stdio.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include "Messages.h"

/**
 * @brief Copies location p2 over to position p1.
**/
#define COPY_POS(p1, p2) ((p1).latitude = (p2).latitude);\
			 ((p1).longitude = (p2).longitude)

/**
 * @brief Checks to see if two positions are equal to one another.
**/
#define POS_EQUAL(p1, p2) ((p1).latitude == (p2).latitude &&\
	       	       	   (p1).longitude == (p2).longitude)

/**
 * @brief Adds the latitude and longitude of p2 to p1. Used for accumulation during averaging.
**/
#define SUM_POS(p1, p2) ((p1).latitude += (p2).latitude);\
			((p1).longitude += (p2).longitude)

/**
 * @brief Opens file for I2C bus that GNSS module is attached to.
 * @details Opens the file for the I2C bus the GNSS module is attached to and attaches 
 *	    slave address to the I2C file descriptor.
 * @return Returns the file descriptor for the I2C bus.
 * @post The I2C bus the GNSS module is attached to is ready for reading/writing.
**/
int I2CGPSOpen();

/**
 * @brief Reads from GNSS module.
 * @details I2CGPSRead() reads from the GNSS module. It is also parses the NMEA data returned
 *	    and extracts the latitude and longitude. The #Message struct that is passed in
 *	    is assigned the extracted values. This function also checks the checksum received
 *	    NMEA data to make sure the returned message is correct.
 * @param message The #Message struct used to assign the latitude and longitude coordinates.
 * @return As the NMEA data is stored in a buffer in the GNSS module, we may not receive an
 *	   entire packet, we may only get half. If either this situation occurs, or there is no
 *	   fix on GNSS satellites, the return value is 0. A 0 is also returned in the scenario
 *	   that the checksum received from the GNSS module does not match what this function 
 *	   calculated. Else, if valid latitude and longitude coordinates are extracted, the return value is 1.
 * @post The #Message struct that is passed in as a parameter will have the most recent GNSS
 *	 latitude and longitude coordinates. 1 will be returned if the GNSS data is valid, 0
 *	 if there is either no fix or half of a packet was received.
**/
int I2CGPSRead(Message * message);

/**
 * @brief Writes to the GNSS module.
 * @details Writes, typically commmand packets, to the GNSS module. The expected format
 *	    of these strings is an NMEA pakcet from $ to *, without the checksum. The
 *	    checksum is calculated and appended on the fly. Check the macros defined in
 *	    I2CGPS.c, as these follow that format. Otherwise check out XA1110 documentation
 *	    for additional information.
 * @param command An ASCII string comprised of the NMEA packet that is being sent to the GNSS module.
 * @pre The command string is in the proper NMEA format, WITHOUT the appended checksum. [$...*].
 * @post The command will have been sent over to the GNSS module.
**/
int I2CGPSWrite(char * command);

/**
 * @brief Closes the I2C file.
 * @details Closes the I2C file via the I2C file descriptor.
**/
void CloseI2C();

#endif
