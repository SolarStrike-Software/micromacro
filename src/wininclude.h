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

	#ifndef WM_MOUSEHWHEEL
		#define WM_MOUSEHWHEEL				0x020E
	#endif


	#ifndef VK_MOUSEMOVE
		#define VK_MOUSEMOVE				0x200
	#endif

	#ifndef VK_MOUSEWHEEL
		#define VK_MOUSEWHEEL				0x201
	#endif

	#ifdef KEY_EVENT
		#undef KEY_EVENT
	#endif
#endif
