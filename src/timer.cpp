/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "timer.h"

#define WINDOWS_TICKS_PER_SECOND		10000000
#define EPOCH_DIFFERENCE				11644473600LL

// Returns CPU frequency; cached (does not change during run-time)
TimeType getFrequency()
{
	static bool queried = false;
	static TimeType frequency;

	if( queried == false )
		QueryPerformanceFrequency(&frequency);

	return frequency;
}

// Returns the current high-precision time
TimeType getNow()
{
	TimeType now;
	//QueryPerformanceFrequency(&timer::frequency);
	QueryPerformanceCounter(&now);

	return now;
}

// Returns the number of seconds elapsed since t1 till t2.
double deltaTime(TimeType t2, TimeType t1)
{
	TimeType diff;

	diff.QuadPart = t2.QuadPart - t1.QuadPart;
	double fdiff = ((double)diff.QuadPart / getFrequency().QuadPart);
	return fdiff;
}
