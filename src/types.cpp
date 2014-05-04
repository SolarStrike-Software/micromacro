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

std::string getChunkString(MemoryChunk *pChunk, unsigned int offset, unsigned int length, int &err)
{
	err = 0;
	if( (offset+length) > pChunk->size )
	{
		err = -1;
		return "";
	}

	return std::string(pChunk->data+offset, length);
}
