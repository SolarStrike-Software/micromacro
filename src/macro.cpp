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

#ifdef NETWORKING_ENABLED
	#include "socket_lua.h"
#endif

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

using MicroMacro::Event;
using MicroMacro::EventType;
using MicroMacro::Mutex;

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
}

CMacro::~CMacro()
{
	// Shut down Ncurses
	if( Ncurses_lua::is_initialized() )
		Ncurses_lua::cleanup(engine.getLuaState());

	engine.cleanup();
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
		Event *pe = new Event;
		pe->type = MicroMacro::EVENT_FOCUSCHANGED;
		pe->idata1 = ((size_t)foregroundHwnd);
		pushEvent(pe);
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
			Event *pe = new Event;
			pe->type = MicroMacro::EVENT_CONSOLERESIZED;
			pushEvent(pe);
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

void CMacro::pushEvent(Event *pe)
{
	if( eventQueueLock.lock(5, __FUNCTION__) )
	{
		eventQueue.push(pe);
		eventQueueLock.unlock(__FUNCTION__);
	}
}

void CMacro::flushEvents()
{
	if( eventQueueLock.lock(INFINITE, __FUNCTION__) )
	{
		// Foreach and delete all
		while( eventQueue.size() )
		{
			Event *pe = eventQueue.front();
			delete pe;
			eventQueue.pop();
		}
		/*
		// Quickest and easiest way is to just make a new queue, then swap.
		std::queue<Event *> emptyQueue;
		swap(eventQueue, emptyQueue);*/
		eventQueueLock.unlock(__FUNCTION__);
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
			Event *pe = new Event;
			if( i > VK_XBUTTON2 )
				pe->type = MicroMacro::EVENT_KEYRELEASED;
			else
				pe->type = MicroMacro::EVENT_MOUSERELEASED;
			pe->idata1 = i;
			pe->idata2 = hid.getToggleState(i);
			try{ pushEvent(pe); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}
		else if( hid.pressed(i) )
		{ // Key pressed
			Event *pe = new Event;
			if( i > VK_XBUTTON2 )
				pe->type = MicroMacro::EVENT_KEYPRESSED;
			else
				pe->type = MicroMacro::EVENT_MOUSEPRESSED;

			pe->idata1 = i;
			pe->idata2 = hid.getToggleState(i);
			try{ pushEvent(pe); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}
	}

	// Handle gamepads
	unsigned int polled = 0;
	for(unsigned int i = 0; i < GAMEPADS && polled < hid.getGamepadMaxIndex(); i++)
	{
		if( !hid.gamepadIsAvailable(i) )
			continue; // Gamepad is disconnected, skip it

		++polled; // Once we've successfully polled the number of detected gamepads, break

		// Check all buttons on this gamepad
		for(unsigned int b = 0; b < GAMEPAD_BUTTONS; b++)
		{
			if( hid.joyPressed(i, b) )
			{
				Event *pe = new Event;
				pe->type = MicroMacro::EVENT_GAMEPADPRESSED;
				pe->idata1 = i + 1;
				pe->idata2 = b + 1;
				try{ pushEvent(pe); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
			else if( hid.joyReleased(i, b) )
			{
				Event *pe = new Event;
				pe->type = MicroMacro::EVENT_GAMEPADRELEASED;
				pe->idata1 = i + 1;
				pe->idata2 = b + 1;
				try{ pushEvent(pe); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
		}

		// Check POV (D-pad)
		if( hid.joyPOVChanged(i) )
		{
			Event *pe = new Event;
			pe->type = MicroMacro::EVENT_GAMEPADPOVCHANGED;
			pe->idata1 = i + 1;
			pe->fdata2 = hid.joyPOV(i)/100;
			try{ pushEvent(pe); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}

		// Check axis
		for(unsigned int a = 1; a <= GAMEPAD_AXIS_COUNT; a++)
		{
			if( hid.joyAxisChanged(i, a) )
			{
				Event *pe = new Event;
				pe->type = MicroMacro::EVENT_GAMEPADAXISCHANGED;
				pe->idata1 = i + 1;
				pe->idata2 = a;
				pe->fdata3 = hid.joyAxis(i, a)/65535.0f*100;
				try{ pushEvent(pe); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
		}
	}

	return MicroMacro::ERR_OK;
}

int CMacro::handleEvents()
{
	int success = MicroMacro::ERR_OK;

	if( eventQueueLock.lock(INFINITE, __FUNCTION__) )
	{
		while( !eventQueue.empty() )
		{
			Event *pe = eventQueue.front();
			success = engine.runEvent(pe);
			delete pe;
			eventQueue.pop();

			if( success != MicroMacro::ERR_OK )
			{
				lua_pop(engine.getLuaState(), 1);
				break;
			}
		}
		eventQueueLock.unlock(__FUNCTION__);
	}

	#ifdef NETWORKING_ENABLED
	if( Socket_lua::socketListLock.lock(INFINITE, __FUNCTION__) )
	{
		SocketListIterator i;

		// Handle all queued events before considering deleting anything
		for(i = Socket_lua::socketList.begin(); i != Socket_lua::socketList.end(); ++i)
		{
			MicroMacro::Socket *pSocket = *i;
			if( pSocket->mutex.lock(INFINITE, __FUNCTION__) )
			{
				while( !pSocket->eventQueue.empty() )
				{
					MicroMacro::Event *pe = &pSocket->eventQueue.front();
					success = engine.runEvent(pe);
					pSocket->eventQueue.pop();

					if( success != MicroMacro::ERR_OK )
					{
						lua_pop(engine.getLuaState(), 1);
						break;
					}
				}

				pSocket->mutex.unlock(__FUNCTION__);
			}
		}


		// Now iterate over the list again and delete those marked for deletion
		i	=	Socket_lua::socketList.begin();
		while( i != Socket_lua::socketList.end() )
		{
			MicroMacro::Socket *pSocket = *i;
			if( pSocket->mutex.lock(INFINITE, __FUNCTION__) )
			{
				if( pSocket->deleteMe )
				{
					i = Socket_lua::socketList.erase(i);
					delete pSocket;
					pSocket = NULL;
				}
				else
				{
					pSocket->mutex.unlock(__FUNCTION__);
					++i;
				}
			}
		}

		// Finally we can unlock
		Socket_lua::socketListLock.unlock(__FUNCTION__);
	}
	#endif

	return success;
}
