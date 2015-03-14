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

#include <stdio.h>

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
		system("pause");
		exit(1);
	}
}

Mutex::~Mutex()
{
	if( handle )
		CloseHandle(handle);
	handle = NULL;
}

int Mutex::lock(int timeoutSecs)
{
	char errBuff[1024];
	if( !handle )
	{
		slprintf(errBuff, sizeof(errBuff), "Cannot lock NULL Mutex\n");
		fprintf(stderr, errBuff);
		Logger::instance()->add(errBuff);
		return false;
	}

	DWORD dwWaitResult = WaitForSingleObject(handle, timeoutSecs);
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
