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
	str		=	NULL;
}

CustomEventData::~CustomEventData()
{
	delete[] str;
	str = NULL;
}

void CustomEventData::setValue(double newNumber)
{
	delete []str; str = NULL;
	number	=	newNumber;
	type	=	LUA_TNUMBER;
}

void CustomEventData::setValue(char *newStr, size_t newLength)
{
	if( str != NULL )
		delete[] str;

	type	=	LUA_TSTRING;
	length	=	newLength;
	str		=	new char[length+1];
	strlcpy(str, newStr, length);
}

CustomEventData &CustomEventData::operator=(const CustomEventData &o)
{
	type	=	o.type;
	length	=	length;

	if( type == LUA_TNUMBER )
		number	=	o.number;
	else if (type == LUA_TSTRING)
	{	// Lets make a new copy of the data
		setValue(o.str, o.length);
	}

	return *this;
}
