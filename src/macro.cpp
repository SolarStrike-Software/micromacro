/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "macro.h"
#include "logger.h"
#include "ncurses_lua.h"
#include "strl.h"
#include "debugmessages.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#ifdef __cplusplus
extern "C" {
	#endif
	BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsoleOutput,BOOL bMaximumWindow,PCONSOLE_FONT_INFO lpConsoleCurrentFont);
	#ifdef __cplusplus
}
#endif

CMacro *CMacro::pinstance = 0;
CMacro *CMacro::instance()
{
	try {
		if( pinstance == 0 )
			pinstance = new Macro;
	}
	catch( std::bad_alloc &ba ) { badAllocation(); }

	return pinstance;
}

CMacro::CMacro()
{
	appHwnd = NULL;
	appHandle = NULL;
	procId = 0;
	foregroundHwnd = NULL;
	consoleCharWidth = 0;
	consoleCharHeight = 0;
	consoleDefaultAttributes = 0;
	lastConsoleSizeX = 0;
	lastConsoleSizeY = 0;
	eventQueueLock = NULL;
}

CMacro::~CMacro()
{
	// Shut down Ncurses
	if( Ncurses_lua::is_initialized() )
		Ncurses_lua::cleanup(engine.getLuaState());

	engine.cleanup();

	if( eventQueueLock )
	{
		CloseHandle(eventQueueLock);
		eventQueueLock = NULL;
	}
}

int CMacro::init()
{
	int success;

	success = engine.init();
	if( success != MicroMacro::ERR_OK )
		return success;

	hid.init();

	getAppHwnd(); 				// Cache it for later
	getAppHandle();				// ^
	getProcId();				// ^
	foregroundHwnd = appHwnd; 	// Assume we're focusing this window

	// Query font info
	CONSOLE_FONT_INFO fontInfo;
	GetCurrentConsoleFont(getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/, false, &fontInfo);
	consoleCharWidth = fontInfo.dwFontSize.X;
	consoleCharHeight = fontInfo.dwFontSize.Y;

	// Get current console size
	RECT winrect;
	if( GetWindowRect(getAppHwnd(), &winrect) )
	{
		lastConsoleSizeX = winrect.right - winrect.left;
		lastConsoleSizeY = winrect.bottom - winrect.top;
	}
	else
	{
		lastConsoleSizeX = 0;
		lastConsoleSizeY = 0;
	}

	// Obtain the default attributes
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(getAppHandle(), &csbi);
	consoleDefaultAttributes = csbi.wAttributes;

	// Create lock for event queue
	eventQueueLock = CreateMutex(NULL, FALSE, NULL);
	if( !eventQueueLock )
	{
		Logger::instance()->add("CreateMutex() failed in CMacro::init(); err code: %d", GetLastError());
		return MicroMacro::ERR_ERR;
	}

	return MicroMacro::ERR_OK;
}

int CMacro::cleanup()
{
	int success;
	success = engine.cleanup();
	if( success != MicroMacro::ERR_OK )
		return success;

	return MicroMacro::ERR_OK;
}

LuaEngine *CMacro::getEngine()
{
	return &engine;
}

Settings *CMacro::getSettings()
{
	return &settings;
}

Hid *CMacro::getHid()
{
	return &hid;
}

DWORD CMacro::getProcId()
{
	if( procId == 0 )
		GetWindowThreadProcessId(getAppHwnd(), &procId);

	return procId;
}

HWND CMacro::getAppHwnd()
{
	if( appHwnd == NULL ) // Automatically set
		appHwnd = GetConsoleWindow();

	return appHwnd;
}

HANDLE CMacro::getAppHandle()
{
	if( appHandle == NULL )
		appHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	return appHandle;
}

void CMacro::pollForegroundWindow()
{
	HWND lastForegroundHwnd = foregroundHwnd;
	foregroundHwnd = GetForegroundWindow();

	if( foregroundHwnd != lastForegroundHwnd )
	{
		// Trigger window focus change event
		Event e;
		e.type = EVENT_FOCUSCHANGED;
		e.idata1 = ((size_t)foregroundHwnd);
		pushEvent(e);
	}
}

void CMacro::pollConsoleResize()
{
	RECT winrect;
	if( GetWindowRect(getAppHwnd(), &winrect) )
	{
		int w = winrect.right - winrect.left;
		int h = winrect.bottom - winrect.top;
		if( w != lastConsoleSizeX ||
			h != lastConsoleSizeY )
		{
			// Update to our new size
			lastConsoleSizeX = w;
			lastConsoleSizeY = h;

			// Trigger window resize event
			Event e;
			e.type = EVENT_CONSOLERESIZED;
			pushEvent(e);
		}
	}
}

HWND CMacro::getForegroundWindow()
{
	return foregroundHwnd;
}

int CMacro::getConsoleFontWidth()
{
	return consoleCharWidth;
}

int CMacro::getConsoleFontHeight()
{
	return consoleCharHeight;
}

DWORD CMacro::getConsoleDefaultAttributes()
{
	return consoleDefaultAttributes;
}

void CMacro::pushEvent(Event &e)
{
	if( eventQueueLock )
	{
		DWORD dwWaitResult = WaitForSingleObject(eventQueueLock, INFINITE);
		switch(dwWaitResult)
		{
			case WAIT_OBJECT_0:
					eventQueue.push(e);

				if( !ReleaseMutex(eventQueueLock) )
				{ // Uh oh... That's not good.
					char errBuff[1024];
					slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
						"CMacro", __FUNCTION__);
					fprintf(stderr, errBuff);
					Logger::instance()->add(errBuff);
				}
			break;

			case WAIT_ABANDONED: // TODO: What should we do here? Error?
			break;
		}
	}
}

void CMacro::flushEvents()
{
	DWORD dwWaitResult = WaitForSingleObject(eventQueueLock, INFINITE);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
		{
			// Quickest and easiest way is to just make a new queue, then swap.
			std::queue<Event> emptyQueue;
			swap(eventQueue, emptyQueue);

			if( !ReleaseMutex(eventQueueLock) )
			{ // Uh oh... That's not good.
				char errBuff[1024];
				slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
					"CMacro", __FUNCTION__);
				fprintf(stderr, errBuff);
				Logger::instance()->add(errBuff);
			}
		}
		break;

		case WAIT_ABANDONED: // TODO: What should we do here? Error?
		break;
	}
}

int CMacro::handleHidInput()
{
	hid.poll();

	// Handle keyboard/mouse
	for(unsigned int i = 0; i < KS_SIZE; i++)
	{
		if( hid.released(i) )
		{ // Key released
			Event e;
			if( i > VK_XBUTTON2 )
				e.type = EVENT_KEYRELEASED;
			else
				e.type = EVENT_MOUSERELEASED;
			e.idata1 = i;
			e.idata2 = hid.getToggleState(i);
			try{ pushEvent(e); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}
		else if( hid.pressed(i) )
		{ // Key pressed
			Event e;
			if( i > VK_XBUTTON2 )
				e.type = EVENT_KEYPRESSED;
			else
				e.type = EVENT_MOUSEPRESSED;

			e.idata1 = i;
			e.idata2 = hid.getToggleState(i);
			try{ pushEvent(e); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}
	}

	// Handle gamepads
	for(unsigned int i = 0; i < hid.getGamepadMaxIndex(); i++)
	{
		// Check all buttons on this gamepad
		for(unsigned int b = 0; b < GAMEPAD_BUTTONS; b++)
		{
			if( hid.joyPressed(i, b) )
			{
				Event e;
				e.type = EVENT_GAMEPADPRESSED;
				e.idata1 = i + 1;
				e.idata2 = b + 1;
				try{ pushEvent(e); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
			else if( hid.joyReleased(i, b) )
			{
				Event e;
				e.type = EVENT_GAMEPADRELEASED;
				e.idata1 = i + 1;
				e.idata2 = b + 1;
				try{ pushEvent(e); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
		}

		// Check POV (D-pad)
		if( hid.joyPOVChanged(i) )
		{
			Event e;
			e.type = EVENT_GAMEPADPOVCHANGED;
			e.idata1 = i + 1;
			e.fdata2 = hid.joyPOV(i)/100;
			try{ pushEvent(e); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}

		// Check axis
		for(unsigned int a = 1; a <= GAMEPAD_AXIS_COUNT; a++)
		{
			if( hid.joyAxisChanged(i, a) )
			{
				Event e;
				e.type = EVENT_GAMEPADAXISCHANGED;
				e.idata1 = i + 1;
				e.idata2 = a;
				e.fdata3 = hid.joyAxis(i, a)/65535.0f*100;
				try{ pushEvent(e); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
		}
	}

	return MicroMacro::ERR_OK;
}

int CMacro::handleEvents()
{
	int success = MicroMacro::ERR_OK;


	DWORD dwWaitResult = WaitForSingleObject(eventQueueLock, INFINITE);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
			while( !eventQueue.empty() )
			{
				Event e = eventQueue.front();
				success = engine.runEvent(e);
				eventQueue.pop();

				if( success != MicroMacro::ERR_OK )
				{
					lua_pop(engine.getLuaState(), 1);
					break;
				}
			}

			if( !ReleaseMutex(eventQueueLock) )
			{ // Uh oh... That's not good.
				char errBuff[1024];
				slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
					"CMacro", __FUNCTION__);
				fprintf(stderr, errBuff);
				Logger::instance()->add(errBuff);
			}
		break;

		case WAIT_ABANDONED: // TODO: What should we do here? Error?
		break;
	}

	return success;
}
