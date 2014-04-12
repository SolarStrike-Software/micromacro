#ifndef TIMER_H
#define TIMER_H

	#include "wininclude.h"

	typedef LARGE_INTEGER		TimeType;
	TimeType getFrequency();
	TimeType getNow();
	double deltaTime(TimeType, TimeType);

#endif
