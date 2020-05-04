/**
 * @file protocol.h
 * @author Joe Biwer
 * @date 11-15-2019
 * @brief Implementation of the motor unit protocol
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#define FLUSH_BITS                  0x1 	<< 7
#define CMD_BITS					0x1F 	<< 2
#define DIR_BITS					0x3

/********* Commands *********/
#define PUSH						0x0		/**< Push data to the end of the list/queue */
#define INSERT						0x1		/**< Insert data at the beginning of the list */

#define MOVE_RIGHT              0       /**< Directional: Turn right */
#define MOVE_LEFT               1       /**< Directional: Turn left */
#define MOVE_FORWARD            2       /**< Directional: Move forward */
#define MOVE_BACKWARD           3       /**< Directional: Move backwards */
#define MOVE_STOP               4       /**< Directional: Stop moving */

// API
#define IS_FLUSH(n)				(n & FLUSH_BITS)	/**< Is the command flush */
#define GET_CMDS(n)				((n & CMD_BITS) >> 2)	/**< Get the command bits */
#define GET_DIR(n)				(n & DIR_BITS)		/**< Get the directional bits */

/********* Construct Data ***/
#define SET_CMD(flush, cmd, dir)		\
	(((0x1 & flush) << 7) | 			\
	 ((cmd & 0x1F) << 2)  |				\
	 (dir & 0x3))								/**< Set the command word based on flush, command, and directional parameters */

#endif	/* PROTOCOL_H */

