/**
 * @file SharedMem.h
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Header file for SharedMem library.
 * @details Header file for SharedMem library. This library provides a means to create and
 *	    share memory between coordinating processes. Often times, a node needs immediate
 *	    information from another node, and marshalling data into a #Message struct and 
 *	    writing to a pipe is not feasible. This is espically true as the communication
 *	    between nodes is asynchronous and could potentially happen at any time. To account
 *          for this, the SharedMem library provides access to shared memory, allowing TX2 nodes
 *	    to communicate and provide pertinent information more rapidly.
**/

#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Messages.h"

// file names for shared memory
#define SHARED_SEG_NAME "shared_nav_memory"
#define SHARED_ANG_NAME "shared_angle_memory"
#define SHARED_POS_NAME "shared_pos_memory"

/**
 * @brief Macro used to set an angle in shared memory.
 * @details This macro sets an angle in shared memory, and sets
 *	    a bit signifying to the receiving node that new data 
 *	    is available.
**/
#define SET_SHARED_ANGLE(mem, val) *((float *)((mem) + 1)) = (val);\
				   mem->dataAvailableFlag = 1

/**\
 * @brief Macro used by receiving node to read in new angle.
 * @details Macro used by receiving node to read in new angle in 
 *	    in shared memory. dataAvailableFlag is reset after read.
 *	    A while loop is used to block the reading process until the
 *	    data is avialable. This isn't the best way to implement and
 *	    could easily lead to the reading process entering deadlock.
 *	    Though, as this is only every used during a turn, so long as the
 *	    gyro doesn't malfunction the reading process should not get stuck.
**/
#define GET_SHARED_ANGLE(mem, val) while (0 == mem->dataAvailableFlag);\
				   (val) = *((float *)((mem) + 1));\
				   mem->dataAvailableFlag = 0

/**
 * @brief Macro used to set a shared #Position in memory.
 * @details Macro used to set a shared #Position in memory. 
**/
#define SET_SHARED_POSITION(mem, val) while (mem->currentlyBeingAccessed);\
				      ((Position *)((mem) + 1))->latitude = val.latitude;\
				      ((Position *)((mem) + 1))->longitude = val.longitude;\
				      mem->dataAvailableFlag = 1.0

/**
 * @brief Macro used to read a #Position from shared memory.
 * @details Macro used to read a #Position from shared memory.
**/
#define GET_SHARED_POSITION(mem, val) do {\
					mem->currentlyBeingAccessed = 1;\
       				      	if (mem->dataAvailableFlag == 1) {\
				      		val.latitude = ((Position *)(mem + 1))->latitude;\
				      		val.longitude = ((Position *)(mem + 1))->longitude;\
						mem->dataAvailableFlag = 0;\
					}\
				        mem->currentlyBeingAccessed = 0;\
				      } while (0)

/**
 * @brief Enum used to differentiate between different types of shared memory.
**/
typedef enum _SMType {
	SegmentationData,
	AngleData,
	PositionData
} SMType;

/**
 * @brief Struct used to access shared memory.
 * @details Struct used to access shared memory. It should be noted that this struct is
 *	    really only used as the header for the memory that is allocated. This header
 *	    contains bitfields used by a process to determine the state of the shared memory.
 *	    As an example take note of the macros above used to set and get values from the
 *	    shared memory. As this struct is meant to be flexible and could hold any data type,
 *	    the macros above index into the data by performing pointer arithmetic on the 
 *	    struct, and then interpret the data accordingly through casting.
**/
typedef struct _SharedMem {
	unsigned int dataAvailableFlag : 1;		// the data in shared memory is new, has not been read
	unsigned int currentlyBeingAccessed : 1;        // the data in shared memory is currently being set
	unsigned int otherData : 30;			// other potential flags, unused at the moment
} SharedMem;

/**
 * @brief Function creates shared memory of a specified size and type.
 * @details This function creates shared memory of a specified size and type. The return value is a pointer
 *	    to the shared memory which is of size (size + sizeof(SharedMem)). When calling this function, simply
 *	    provide the size of the data area, the header is automatically accounted for.
 * @param size The size of the data area. This function allocates size + sizeof(SharedMem) bytes for the actual
 *	       shared memory.
 * @param type The type of shared memory being allocated, as defined by #SMType. Different memory types have 
 *	       different read/write permissions, this value is used to differentiate. Please see SharedMem.c to
 *	       see how this enum effects the shared memory location. At the moment there can only be one call
 *	       to this function from any one of the running nodes as type is also used as an identifier for a 
 *	       shared memory location. As an example, after tx2_cam_node.cpp creates shared memory of #SMType
 *	       #SegmentationData, no other process can call this function with that type.
 * @return Returns the address to the shared memory space. 
 * @post Once this function has been called, #OpenSharedMemory can be called by the 
**/
SharedMem * CreateSharedMemory(int size, SMType type);

/**
 * @brief Function opens shared memory of a specified size and type.
 * @details This function opens the shared memory created from the #CreateSharedMemory call, specified via
 *	    the parameter type. The size of the shared memory location also needs to be known by the process
 *	    which is opening this location. For #AngleData and #PositionData, this size of the shared memory 
 *	    is hard coded. For semantic segmentation however, the size is entirley dependent on the resolution
 *	    of the camera. In this case, a special #Message struct #ShMem is passed in a message from the 
 *	    tx2_cam_node.cpp to tx2_nav_node.c, which contains the dimensions of the image, allowing the size
 *	    to be determined at run time in the event that a different camera is used. 
 * @param size The size of the data area. This must be the same as the size used when calling
 * 	       #CreateSharedMemory, else you run the risk of a crashing node. Segfault.
 * @param type The type of shared memory. As mentioned in #CreateSharedMemory, the #SMType not only determines
 *	       the read/write access to the memory location, but defines the shared memory location itself. 
 *	       After a node has called #CreateSharedMemory with a specified type, any process (node) that calls
 *	       this function with the specified type is accessing the memory that was created after the call to
 *	       #CreateSharedMemory.
 * @return Returns a #SharedMem pointer to the memory region created afte the call to #CreateSharedMemory. The
 *	   size of this region is (size + sizeof(SharedMem)).
**/
SharedMem * OpenSharedMemory(int size, SMType type);

/**
 * @brief This function closes the file descriptors used when creating/opening shared memory.
 * @details As shared memory regions are initially opened as files, they should be closed after a node
 *	    is done using the shared memory region. This function call closes all shared memory file
 *	    descriptors that a node either created or opened.
 * @post All file descriptors for shared memory that a process holds have been closed.
**/
void CloseSharedMemory();

#endif
