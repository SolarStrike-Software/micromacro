/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "eventdata.h"
#include "strl.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

using namespace MicroMacro;

EventData::EventData()
{
	type	=	ED_NIL;
	length	=	0;
	iNumber	=	0;
	pSocket	=	NULL;
}

EventData::~EventData()
{
	str = "";
}

void EventData::setValue()
{
	str		=	"";
	type	=	ED_NIL;
}

void EventData::setValue(int newNumber)
{
	str		=	"";
	iNumber	=	newNumber;
	type	=	ED_INTEGER;
}

void EventData::setValue(unsigned long long newNumber)
{
	str			=	"";
	i64Number	=	newNumber;
	type		=	ED_64INTEGER;
}

void EventData::setValue(double newNumber)
{
	str		=	"";
	fNumber	=	newNumber;
	type	=	ED_NUMBER;
}

void EventData::setValue(std::string newStr, size_t newLength)
{
	type	=	ED_STRING;
	if( newLength != 0 )
		length	=	newLength;
	else
		length	=	newStr.length();
	str		=	std::string(newStr, newLength);
}

void EventData::setValue(char *newStr, size_t newLength)
{
	type	=	ED_STRING;
	length	=	newLength;
	str		=	std::string(newStr, newLength);
}

void EventData::setValue(MicroMacro::Socket *npSocket)
{
	type	=	ED_SOCKET;
	pSocket	=	npSocket;
}

void EventData::setValue(struct sockaddr_in *newSockAddr)
{
	type	=	ED_SOCKADDR;
	memcpy(&remoteAddr, newSockAddr, sizeof(struct sockaddr_in));
}

EventData &EventData::operator=(const EventData &o)
{
	type	=	o.type;
	length	=	length;

	// Copy only necessary data
	switch( type )
	{
		case ED_STRING:
			str	=	std::string(o.str, length);
		break;
		case ED_INTEGER:
			iNumber	=	o.iNumber;
		break;
		case ED_64INTEGER:
			i64Number	=	o.i64Number;
		break;
		case ED_NUMBER:
			fNumber	=	o.fNumber;
		break;
		case ED_SOCKET:
			pSocket	=	o.pSocket;
		break;
		case ED_SOCKADDR:
			setValue((struct sockaddr_in *)&o.remoteAddr);
		break;
		default:
		break;
	}

	return *this;
}
