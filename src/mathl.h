/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef MATHL_H
#define MATHL_H

	#include <algorithm>

	template <typename T>
	T clamp(T value, T _min, T _max)
	{
		value = std::min(std::max(value, _min), _max);

		return value;
	}

#endif
