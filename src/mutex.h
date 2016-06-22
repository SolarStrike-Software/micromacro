/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef MUTEX_H
#define MUTEX_H

	#include "wininclude.h"
	#include <string>

	namespace MicroMacro
	{
		// Just a very simple mutex wrapper to make our jobs easier.
		class Mutex
		{
			private:
				HANDLE handle;
				std::string prevOrigin;

			public:
				Mutex();
				~Mutex();
				int lock(int = INFINITE, std::string = "");
				int unlock(std::string = "");

		};
	}

#endif
