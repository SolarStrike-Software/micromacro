/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "macro.h"
#include "logger.h"
#include "ncurses_lua.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

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
		e.idata1 = ((int)foregroundHwnd);
		eventQueue.push(e);
	}
}

HWND CMacro::getForegroundWindow()
{
	return foregroundHwnd;
}

std::queue<Event> *CMacro::getEventQueue()
{
	return &eventQueue;
}

void CMacro::flushEvents()
{
	// Quickest and easiest way is to just make a new queue, then swap.
	std::queue<Event> emptyQueue;
	swap(eventQueue, emptyQueue);
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
			try{ eventQueue.push(e); }
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
			try{ eventQueue.push(e); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}
	}

	// Handle gamepads
	for(unsigned int i = 0; i < GAMEPADS; i++)
	{
		// Check all buttons on this gamepad
		for(unsigned int b = 0; b < GAMEPAD_BUTTONS; b++)
		{
			if( hid.joyPressed(i, b) )
			{
				Event e;
				e.type = EVENT_GAMEPADPRESSED;
				e.idata1 = i;
				e.idata2 = b + 1;
				try{ eventQueue.push(e); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
			else if( hid.joyReleased(i, b) )
			{
				Event e;
				e.type = EVENT_GAMEPADRELEASED;
				e.idata1 = i;
				e.idata2 = b + 1;
				try{ eventQueue.push(e); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
		}

		// Check POV (D-pad)
		if( hid.joyPOVChanged(i) )
		{
			Event e;
			e.type = EVENT_GAMEPADPOVCHANGED;
			e.idata1 = i;
			e.fdata2 = hid.joyPOV(i)/100;
			try{ eventQueue.push(e); }
			catch( std::bad_alloc &ba ) { badAllocation(); }
		}

		// Check axis
		for(unsigned int a = 1; a <= GAMEPAD_AXIS_COUNT; a++)
		{
			if( hid.joyAxisChanged(i, a) )
			{
				Event e;
				e.type = EVENT_GAMEPADAXISCHANGED;
				e.idata1 = i;
				e.idata2 = a;
				e.fdata3 = hid.joyAxis(i, a)/65535.0f*100;
				try{ eventQueue.push(e); }
				catch( std::bad_alloc &ba ) { badAllocation(); }
			}
		}
	}

	return MicroMacro::ERR_OK;
}

int CMacro::handleEvents()
{
	int success = MicroMacro::ERR_OK;
	while( !eventQueue.empty() )
	{
		Event e = eventQueue.front();
		success = engine.runEvent(e);
		eventQueue.pop();

		if( success != MicroMacro::ERR_OK )
		{
			lua_pop(engine.getLuaState(), 1);
			return success;
		}
	}

	return success;
}
