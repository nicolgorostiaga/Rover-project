/**
 * @file I2CGyro.c
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Function definitions for I2CGyro library.
 * @details Function definitions for I2CGyro library.
**/

#include "../include/I2CGyro.h"

/**
 * @brief Internal I2C file descriptor.
**/
int i2cFd;

/**
 * @brief Internal #I2C struct.
**/
I2C i2cStruct;

/**
 * @brief Internal function used to set internal gyroscope registers.
 * @details Internal function used to set internal gyroscope registers. As writing to the gyroscope requires
 *	    mixed write/read I2C commands, normal read/write system calls cannot be used. These smbus calls 
 *	    must be used for this very reason. Again, READ AND WRITE WILL NOT WORK.
 * @param registerAddress The internal gyro address whose value is being set.
 * @param value The value being placed in registerAddress.
**/
void SetRegister(int registerAddress, char value)
{
        i2c_smbus_write_byte_data(i2cFd, registerAddress, value);
}

/**
 * @brief Internal function used to read from a gyroscope register.
 * @details Internal function used to read from a gyroscope register. Again, read and write do not work and
 *	    smbus functions must be used. Especially since this call in particular is a mixed write/read call.
 * @param i2c The #I2C struct whose values are being writtne to the I2C bus.
**/
int i2cRead(I2C * i2c)
{
        return i2c_smbus_read_i2c_block_data(i2cFd, i2c->registerAddress, i2c->bytes, i2c->Buffer);
}

int OpenI2C()
{
	I2C in = {.bytes = 1, .registerAddress = 0x0F };
	char filename[20];
	memset(filename, 0, sizeof(filename));

	// open I2C device
        sprintf(filename, "%s", "/dev/i2c-0");

	i2cFd = open(filename, O_RDWR);

	if (i2cFd <=  0) {
	        printf("error opening i2c for gyro\n");
	        return -1;
	}

	// bind with slave address
	if (ioctl(i2cFd, I2C_SLAVE, 0x6B) < 0) {
	        printf("error setting i2c address\n");
	        return -1;
	}

	// initial setup for gyroscope
	SetRegister(CTRL_6_X, ODR_238);
	SetRegister(CTRL_1_G, ODR_238); // 100 Hz instead of 800
	SetRegister(0x04, 0x80);
	SetRegister(0x1E, 0x20);

	i2cRead(&in);

	memset(&i2cStruct, 0, sizeof(i2cStruct));

	// since we are really only concerned about the Z axis, setup struct 
	// so we are only reading the 2 bytes associated with the Z axis.
	i2cStruct.bytes = 2;
	i2cStruct.registerAddress = 0x1C;

	return i2cFd;	
}

float GetAngularVelocity()
{
	short int z;
	float zG;

	// read in the Z axis registers
	i2cRead(&i2cStruct);

	// extract values
	z = i2cStruct.Buffer[0];
	z |= i2cStruct.Buffer[1] << 8;

	// convert binary value to degrees/sec
	zG = z * ((2.5f * 245.0f) / 65535.0f);

	return zG;
}

void CloseI2C()
{
	close(i2cFd);
}
