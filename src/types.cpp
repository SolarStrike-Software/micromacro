/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "types.h"

BatchJob &BatchJob::operator=(const BatchJob &o)
{
	this->count = o.count;
	this->type = o.type;
	return *this;
}
