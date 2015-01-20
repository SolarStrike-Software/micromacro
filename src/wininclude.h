/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef WININCLUDE_H
#define WININCLUDE_H

	#ifndef WINVER
		#define WINVER 0x0501
	#endif

	#ifdef _WIN32_WINNT
		#undef _WIN32_WINNT
		#define _WIN32_WINNT 0x0501
	#endif

	#ifndef WIN32
		#ifndef MAX_PATH
			#include <limits.h>
			#define MAX_PATH		PATH_MAX
		#endif
	#else
		#include <windows.h>
		#include <psapi.h>
		//#include <conio.h>
		#include <lm.h>
	#endif

	#ifndef WPF_ASYNCWINDOWPLACEMENT
		#define WPF_ASYNCWINDOWPLACEMENT	0x0004
	#endif

#endif
