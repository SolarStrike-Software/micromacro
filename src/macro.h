/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef MACRO_H
#define MACRO_H

	#include <queue>

	#include "luaengine.h"
	#include "settings.h"
	#include "hid.h"
	#include "event.h"

	class CMacro;
	typedef CMacro Macro;
	class CMacro
	{
		private:
			static CMacro *pinstance;

			int consoleCharWidth;
			int consoleCharHeight;
			DWORD consoleDefaultAttributes;

		protected:
			CMacro();
			~CMacro();
			CMacro(const CMacro &);
			CMacro &operator=(const CMacro &);

			DWORD procId;
			HANDLE appHandle;
			HWND appHwnd;
			HWND foregroundHwnd;
			LuaEngine engine;
			Settings settings;
			Hid hid;

			int lastConsoleSizeX;
			int lastConsoleSizeY;

			std::queue<Event> eventQueue;

		public:
			static CMacro *instance();
			int init();
			int cleanup();
			LuaEngine *getEngine();
			Settings *getSettings();
			Hid *getHid();

			DWORD getProcId();
			HWND getAppHwnd();
			HANDLE getAppHandle();
			void pollForegroundWindow();
			void pollConsoleResize();
			HWND getForegroundWindow();

			int getConsoleFontWidth();
			int getConsoleFontHeight();
			DWORD getConsoleDefaultAttributes();

			void pushEvent(Event &);
			void flushEvents();

			int handleHidInput();
			int handleEvents();

			HANDLE eventQueueLock;
	};

#endif
