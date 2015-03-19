/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef OS_H
#define OS_H

	#include "wininclude.h"
	#include <string>

	namespace OS
	{
		std::string getPrivName(DWORD);
		std::string getOsName();
		DWORD getUserPriv();
		int modifyPermission(HANDLE, const char *, bool);
	}

#endif
