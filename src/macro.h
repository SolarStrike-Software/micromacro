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

			std::queue<Event> eventQueue;

		public:
			static CMacro *instance();
			int init();
			int cleanup();
			LuaEngine *getEngine();
			Settings *getSettings();
			Hid *getHid();

			//void setAppHwnd(HWND);
			DWORD getProcId();
			HWND getAppHwnd();
			HANDLE getAppHandle();
			void pollForegroundWindow();
			HWND getForegroundWindow();

			std::queue<Event> *getEventQueue();
			void flushEvents();

			int handleHidInput();
			int handleEvents();
	};

#endif
