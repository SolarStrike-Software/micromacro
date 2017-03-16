/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "customevent.h"
#include "strl.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

using namespace MicroMacro;

CustomEventData::CustomEventData()
{
	type	=	LUA_TNIL;
	length	=	0;
	number	=	0;
}

CustomEventData::~CustomEventData()
{
	str = "";
}

void CustomEventData::setValue(double newNumber)
{
	str		=	"";
	number	=	newNumber;
	type	=	LUA_TNUMBER;
}

void CustomEventData::setValue(char *newStr, size_t newLength)
{
	type	=	LUA_TSTRING;
	length	=	newLength;
	str		=	std::string(newStr, newLength);
}

CustomEventData &CustomEventData::operator=(const CustomEventData &o)
{
	type	=	o.type;
	length	=	length;

	if( type == LUA_TNUMBER )
		number	=	o.number;
	else if (type == LUA_TSTRING)
	{	// Lets make a new copy of the data
		str		=	std::string(o.str, length);
	}

	return *this;
}
