/**
 * @file Messages.c
 * @author Patrick Henz
 * @date 12-11-2019
 * @brief Function defitions for Messages library.
 * @details Function defitions for Messages library.
**/

#include "../include/Messages.h"

/**
 * @brief Internal array for library to keep track of file descriptors
**/
int * fileDescriptors;

/**
 * @brief Largest file descriptor in the #fileDescriptors array.
**/
int maxFileDescriptor;

/**
 * @brief The number of file descriptors in #fileDescriptors.
**/
int fdCount;

/**
 * @brief Struct for defining timeouts.
**/
struct timespec timeout;

void SetupSetAndWait(int * fd, int count)
{
	int i;

	// retain address of fd for internal use
	fileDescriptors = fd;	
	fdCount = count;

	maxFileDescriptor = 0;

	// find the largest file descriptor in the set
	for (i = 0; i < count; i++) {
		if (maxFileDescriptor < fd[i]) {
			maxFileDescriptor = fd[i];
		}
	}
}

int ModifySetAndWait(int oldFd, int newFd)
{
	int i;
	int found = 0;

	// find the old file descriptor we are replacing
	for (i = 0; i < fdCount; i++) {
		// if we find it, replace it and break
		if (fileDescriptors[i] == oldFd) {
			fileDescriptors[i] = newFd;
			found = 1;
			break;
		}
	}

	// if we replaced a file descriptor, figure out if the
	// new fd is larger than our previous max. replace it if so
	if (found && newFd > maxFileDescriptor) {
		maxFileDescriptor = newFd;
	} else if (!found) {
		// error
		return -1;
	}

	return 0;
}

void CopyMessage(Message * destination, Message * source)
{
	memcpy(destination, source, sizeof(Message));
}

int SetAndWait(fd_set * rdfs, long seconds, long nanoSeconds)
{
	int i;
		
	// zero out read file descriptors set
	FD_ZERO(rdfs);

	// set the file descriptors
	for (i = 0; i < fdCount; i++) {
		FD_SET(fileDescriptors[i], rdfs);
	}

	// set timeout values
	timeout.tv_sec  = seconds;
	timeout.tv_nsec = nanoSeconds;

	// block here until we either timeout or a file descriptor becomes available to read.
	return pselect(maxFileDescriptor + 1, rdfs, NULL, NULL, &timeout, NULL);	
}
