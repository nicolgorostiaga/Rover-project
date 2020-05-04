/**
 * @file I2CGyro.h
 * @author Patrick Henz
 * @date 12-2-2019
 * @brief Header file for the I2CGyro library
 * @details Header file for the I2CGyro library. Function prototypes (non-internal functions!), macros, and typedefs
 * 	    can be found here.
**/

#ifndef I2CGYRO_H
#define I2CGYRO_H

#include <stdio.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

// register addresses and register value macros
#define CTRL_1_G 0x10
#define CTRL_6_X 0x20
#define ODR_238  0x80
#define USEC_238HZ  4202
#define SAMPLE_T 0.0042f

typedef struct i2c {
	char registerAddress;
	char Buffer[6];
        short int bytes;
} I2C; /**< Struct containing the address being read from/written to, the number of bytes, and the buffer where the input/output is stored*/

/**
 * @brief Opens up the I2C file associated with the Gyro module.
 * @details Opens up the I2c file associated with the Gyro module. It also attaches the slave address
 *	    of the attached device (LSM9DS1) to the file descriptor.
 * @return Returns the file descriptor for the opened file.
**/
int OpenI2C();

/**
 * @brief Closes the I2C file descriptor used to communicate with Gyro.
**/
void CloseI2C();

/**
 * @brief GetAngularVelocity() returns the angular velocity as measured by the gyroscope.
 * @details This function returns the angular velocity (in degrees/sec) as measured by the
 * 	    gyroscope. At the moment, this function simply returns the angular velocity of the
 * 	    Z (vertical) axis. This is used when the rover is in the process of turning to 
 * 	    figure out how the rover is responding to turn commands and making any corrections
 * 	    if needed.
**/
float GetAngularVelocity();

#endif
