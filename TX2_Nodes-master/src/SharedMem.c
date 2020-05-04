/**
 * @file SharedMem.c
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Function definitions for SharedMem library.
 * @details Function definitions for SharedMem library.
**/

#include "../include/SharedMem.h"

/**
 * @brief Semantic segmentation shared memory file descriptor.
**/
int segFd;
/**
 * @brief Angle data shared memory file descriptor.
**/
int angFd;
/**
 * @brief GNSS position shared memory file descriptor.
**/
int posFd;

SharedMem * CreateSharedMemory(int size, SMType type)
{
	SharedMem * sharedMem;
	int memFd;

	// determine how to open the shared memory. In this case, the important thing
	// is the file location
	// modes (3rd param) in usr/include/aarch64-linux-gnu/sys/stat.h
	switch (type) {
		case SegmentationData:
			memFd = segFd = shm_open(SHARED_SEG_NAME, O_CREAT | O_RDWR, S_IRWXU);
			break;
		case AngleData:
			memFd = angFd = shm_open(SHARED_ANG_NAME, O_CREAT | O_RDWR, S_IRWXU);
			break;
		case PositionData:
			memFd = posFd = shm_open(SHARED_POS_NAME, O_CREAT | O_RDWR, S_IRWXU);
	}

	if (memFd <= 0)
	{
		printf("error opening shared memory\n");
		return NULL;
	}

	// truncate file, total shared memory size
	int stat = ftruncate(memFd, size + sizeof(SharedMem));

	if (stat == -1)
	{
		printf("error truncating file\n");
		return NULL;
	}

	// see man page for protections and flags
	sharedMem = mmap(NULL, size + sizeof(SharedMem), PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);

	if (sharedMem == NULL)
	{
		printf("error mapping memory\n");
		return NULL;
	}

	return sharedMem;
}

SharedMem * OpenSharedMemory(int size, SMType type)
{
	SharedMem * sharedMem;
	int memFd;

	// determine how to open shared memory
	// semantic seg is read only, others are read write to set flags.
	switch (type) {
		case SegmentationData:
			memFd = segFd = shm_open(SHARED_SEG_NAME, O_RDONLY, 0);
			break;
		case AngleData:
			memFd = angFd = shm_open(SHARED_ANG_NAME, O_RDWR, 0);
			break;
		case PositionData:
			memFd = posFd = shm_open(SHARED_POS_NAME, O_RDWR, 0);
			break;
	}

	if (memFd <= 0)
	{
		printf("error opening shared memory\n");
		return NULL;
	}

	// determine how to map to pointer. This is shared between AngleData and PositionData
	switch (type) {
		case SegmentationData:
			sharedMem = mmap(NULL, size + sizeof(SharedMem), PROT_READ, MAP_SHARED, memFd, 0);
			break;
		case AngleData:
		case PositionData:
			sharedMem = mmap(NULL, size + sizeof(SharedMem), PROT_READ | PROT_WRITE, MAP_SHARED, memFd, 0);
			break;
	}

	if (sharedMem == NULL)
	{
		printf("error mapping memory\n");
		return NULL;
	}

	return sharedMem;
}

void CloseSharedMemory()
{
	// close everything
	close(segFd);
	close(angFd);
	close(posFd);
}
