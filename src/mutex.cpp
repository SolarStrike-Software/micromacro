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

using MicroMacro::Mutex;

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
		slprintf(errMsg, sizeof(errMsg), "Failed to create a mutex. Error %d: %s",
			err, errString);
		fprintf(stderr, errMsg);
		Logger::instance()->add("%s", errMsg);
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
	{
		ReleaseMutex(handle);
		CloseHandle(handle);
	}
	handle = NULL;
}

int Mutex::lock(int timeoutMsecs, std::string origin)
{
	char errBuff[1024];
	if( !handle )
	{
		printf("Current origin: %s\n", origin.c_str());
		slprintf(errBuff, sizeof(errBuff), "Cannot lock NULL Mutex");
		fprintf(stderr, errBuff);
		Logger::instance()->add("%s", errBuff);
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
			prevOrigin	=	origin;
			return true;
		break;

		case WAIT_ABANDONED:
		{
			printf("Previous (known) call: %s\n", prevOrigin.c_str());
			slprintf(errBuff, sizeof(errBuff), "Waiting for mutex has timed out.");
			fprintf(stderr, errBuff);
			Logger::instance()->add("%s", errBuff);

			try {
				throw bad_mutex;
			} catch(std::exception &e) {
				printf("%s\n", e.what());
				exit(1);
			}

			return false;
		}
		break;

		case WAIT_FAILED:
		{
			printf("Previous (known) call: %s\n", prevOrigin.c_str());
			int errCode = GetLastError();
			slprintf(errBuff, sizeof(errBuff), "Waiting for mutex has failed. Err code: %d", errCode);
			fprintf(stderr, errBuff);
			Logger::instance()->add("%s", errBuff);

			try {
				throw bad_mutex;
			} catch(std::exception &e) {
				printf("%s\n", e.what());
				exit(1);
			}

			return false;
		}

		default:
		{
			printf("Previous (known) call: %s\n", prevOrigin.c_str());
			slprintf(errBuff, sizeof(errBuff), "Unknown result from WaitForSingleObject: %d", dwWaitResult);
			fprintf(stderr, "%s\n", errBuff);
			Logger::instance()->add("%s", errBuff);
			return false;
		}
		break;
	}

	return true;
}

int Mutex::unlock(std::string origin)
{
	char errBuff[1024];
	if( !handle )
	{
		printf("Current origin: %s\n", origin.c_str());
		slprintf(errBuff, sizeof(errBuff), "Cannot unlock NULL Mutex");
		fprintf(stderr, errBuff);
		Logger::instance()->add("%s", errBuff);

		try {
			throw bad_mutex;
		} catch(std::exception &e) {
			printf("%s\n", e.what());
			system("pause");
			exit(1);
		}
		return false;
	}

	if( !ReleaseMutex(handle) )
	{ // Uh oh... That's not good.
		printf("Current origin: %s\n", origin.c_str());
		slprintf(errBuff, sizeof(errBuff), "Unable to ReleaseMutex()");

		try {
			throw bad_mutex;
		} catch(std::exception &e) {
			printf("%s\n", e.what());
			system("pause");
			exit(1);
		}

		fprintf(stderr, "%s\n", errBuff);
		Logger::instance()->add("%s", errBuff);
		return false;
	}

	return true;
}
