/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "mutex.h"
#include "strl.h"
#include "error.h"
#include "debugmessages.h"
#include "logger.h"

#include <exception>
#include <stdexcept>
#include <stdio.h>

class bad_mutex_exception : public std::exception
{
	virtual const char *what() const throw()
	{
		return "Bad MUTEX (NULL)";
	}
} bad_mutex;

Mutex::Mutex()
{
	handle = CreateMutex(NULL, FALSE, NULL);
	if( !handle )
	{
		int err = GetLastError();
		const char *errString = getErrorString(err);

		char errMsg[1024];
		slprintf(errMsg, sizeof(errMsg), "Failed to create a mutex. Error %d: %s\n",
			err, errString);
		fprintf(stderr, errMsg);
		Logger::instance()->add(errMsg);
		try {
			throw bad_mutex;
		} catch(std::exception &e) {
			printf("%s\n", e.what());
			system("pause");
			exit(1);
		}
	}
}

Mutex::~Mutex()
{
	if( handle )
		CloseHandle(handle);
	handle = NULL;
}

int Mutex::lock(int timeoutMsecs)
{
	char errBuff[1024];
	if( !handle )
	{
		slprintf(errBuff, sizeof(errBuff), "Cannot lock NULL Mutex\n");
		fprintf(stderr, errBuff);
		Logger::instance()->add(errBuff);
		try {
			throw bad_mutex;
		} catch(std::exception &e) {
			printf("%s\n", e.what());
			system("pause");
			exit(1);
		}
		return false;
	}

	DWORD dwWaitResult = WaitForSingleObject(handle, timeoutMsecs);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
			return true;
		break;

		case WAIT_ABANDONED:
		{
			slprintf(errBuff, sizeof(errBuff), "Waiting for mutex has timed out.\n");
			fprintf(stderr, errBuff);
			Logger::instance()->add(errBuff);
			return false;
		}
		break;

		default:
		{
			slprintf(errBuff, sizeof(errBuff), "Unknown result from WaitForSingleObject: %d\n", dwWaitResult);
			fprintf(stderr, errBuff);
			Logger::instance()->add(errBuff);
			return false;
		}
		break;
	}

	return true;
}

int Mutex::unlock()
{
	char errBuff[1024];
	if( !handle )
	{
		slprintf(errBuff, sizeof(errBuff), "Cannot unlock NULL Mutex\n");
		fprintf(stderr, errBuff);
		Logger::instance()->add(errBuff);
		return false;
	}

	if( !ReleaseMutex(handle) )
	{ // Uh oh... That's not good.
		slprintf(errBuff, sizeof(errBuff), "Unable to ReleaseMutex()\n");
		fprintf(stderr, errBuff);
		Logger::instance()->add(errBuff);
		return false;
	}

	return true;
}
