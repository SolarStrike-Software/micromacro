/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "system_lua.h"
#include "error.h"
#include "wininclude.h"
#include "macro.h"
#include "strl.h"
#include "event.h"

#include <stdio.h>
#include <string>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

int System_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"rest", System_lua::rest},
		{"exec", System_lua::exec},
		{"getClipboard", System_lua::getClipboard},
		{"setClipboard", System_lua::setClipboard},
		{"getActiveCodePage", System_lua::getActiveCodePage},
		{"getConsoleCodePage", System_lua::getConsoleCodePage},
		{"setPriority", System_lua::setPriority},
		{"getConsoleAttributes", System_lua::getConsoleAttributes},
		{"setConsoleAttributes", System_lua::setConsoleAttributes},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, SYSTEM_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

/*	system.rest(number msec)
	Returns:	nil

	Put the process to sleep for 'msec' milliseconds.
*/
int System_lua::rest(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	unsigned int msec = (unsigned int)lua_tonumber(L, 1);
	::Sleep(msec);

	return 0;
}

/*	system.exec(string cmd)
	Returns:	string

	Run the given command and return its output as a string.
*/
int System_lua::exec(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	std::string szResult;
	char buffer[1024];
	const char *cmd = lua_tostring(L, 1);
	FILE *file;
	file = popen(cmd, "r");
	if( !file )
		return 0;

	while( fgets(buffer, sizeof(buffer)-1, file) != NULL )
		szResult += buffer;
	pclose(file);

	lua_pushstring(L, szResult.c_str());
	return 1;
}

/*	system.getClipboard()
	Returns:	string

	Returns the system clipboard data as a string.
*/
int System_lua::getClipboard(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	HGLOBAL hGlobal = NULL;
	if( !IsClipboardFormatAvailable(CF_TEXT) )
		return 0;

	OpenClipboard(Macro::instance()->getAppHwnd());
	hGlobal = GetClipboardData(CF_TEXT);
	PSTR pClip;

	bool success = true;
	if( !hGlobal )
		success = false;
	else
	{
		pClip = (CHAR *)GlobalLock(hGlobal);
		if( !pClip ) { // Throw error (see below)
			CloseClipboard();
			success = false;
		}
	}

	if( !success ) // Throw error (for real this time)
	{
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failure to read clipboard data. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());

		return 0;
	}

	lua_pushstring(L, pClip); // Push the value before destroying it

	// Make sure to free up our resources
	GlobalUnlock(hGlobal);
	CloseClipboard();

	return 1;
}

/*	system.setClipboard(string data)
	Returns:	boolean

	Sets the system's clipboard to 'data'.
	Returns true on success, false on failure.
*/
int System_lua::setClipboard(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	size_t dataLen;
	const char *data = lua_tolstring(L, 1, &dataLen);
	HGLOBAL hGlobal = NULL;
	hGlobal = GlobalAlloc(GMEM_MOVEABLE, dataLen+1);
	int success = true;

	if( !hGlobal ) // Failed to allocate
		success = false;
	else
	{
		PSTR pClip = (CHAR *)GlobalLock(hGlobal);
		if( !pClip )
		{
			GlobalFree(hGlobal);
			success = false;
		}
		else
		{
			strlcpy(pClip, data, dataLen);
			GlobalUnlock(pClip);
			OpenClipboard(Macro::instance()->getAppHwnd());
			EmptyClipboard();
			SetClipboardData(CF_TEXT, pClip);
			CloseClipboard();
		}
	}

	if( !success ) // Throw error
	{
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failure to set clipboard data. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());
	}

	lua_pushboolean(L, success);
	return 1;
}

/*	system.getActiveCodePage()
	Returns:	number	acp

	Returns the system's current code page as an integer.
*/
int System_lua::getActiveCodePage(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);
	lua_pushinteger(L, GetACP());
	return 1;
}

/*	system.getConsoleCodePage()
	Returns:	number	ccp

	Returns the console's current code page as an integer.
*/
int System_lua::getConsoleCodePage(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);
	lua_pushinteger(L, GetConsoleCP());
	return 1;
}

/*	system.setPriority(string priority)
	Returns:	nil

	Change the process's priority.
	'priority' should be "high", "low", or "normal" (default)
*/
int System_lua::setPriority(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	unsigned int priority = 0;
	const char *priorityStr = lua_tostring(L, 1);
	if( strcmp(priorityStr, "high") )
		priority = ABOVE_NORMAL_PRIORITY_CLASS;
	else if( strcmp(priorityStr, "low" ) )
		priority = BELOW_NORMAL_PRIORITY_CLASS;
	else /*if( strcmp(priorityStr, "normal") )*/
		priority = NORMAL_PRIORITY_CLASS;

	SetPriorityClass(GetCurrentProcess(), priority);
	return 0;
}

/*	system.getConsoleAttributes()
	Returns (on success):	number windowWidth
							number windowHeight
							number bufferWidth
							number bufferHeight
							number cursorX
							number cursorY

	Returns (on failure): nil

	Returns a set of console attributes.
	Values are in characters, not pixels!
*/
int System_lua::getConsoleAttributes(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	CONSOLE_SCREEN_BUFFER_INFO lpcsbi;
	if( GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &lpcsbi) )
	{
		lua_pushnumber(L, lpcsbi.srWindow.Right + 1);
		lua_pushnumber(L, lpcsbi.srWindow.Bottom - lpcsbi.srWindow.Top + 1);
		lua_pushnumber(L, lpcsbi.dwSize.X);
		lua_pushnumber(L, lpcsbi.dwSize.Y);
		lua_pushnumber(L, lpcsbi.dwCursorPosition.X + 1);
		lua_pushnumber(L, lpcsbi.dwCursorPosition.Y + 1);
		return 6;
	}
	else
		return 0; // Error
}

/*	system.getConsoleAttributes(number windowWidth, number windowHeight,
								number bufferWidth, number bufferHeight)
	Returns:	nil

	Modify the console's attributes.
	Values should be in characters, not pixels.
*/
int System_lua::setConsoleAttributes(lua_State *L)
{
	int args = lua_gettop(L);
	if( args != 2 && args != 4 )
		wrongArgs(L);

	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	if( args >= 4 )
	{
		checkType(L, LT_NUMBER, 3);
		checkType(L, LT_NUMBER, 4);
	}

	// Get original buffer size
	CONSOLE_SCREEN_BUFFER_INFO lpcsbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &lpcsbi);

	// For window size
	SMALL_RECT rect;
	rect.Right = lua_tointeger(L, 1) - 1;
	rect.Bottom = lua_tointeger(L, 2) - 1;
	rect.Left = 0;
	rect.Top = 0;

	// For buffer size
	COORD coord;
	coord.X = lpcsbi.dwSize.X;
	coord.Y = lpcsbi.dwSize.Y;

	// Set buffer size
	if( args >= 4 )
	{
		coord.X = lua_tointeger(L, 3);
		coord.Y = lua_tointeger(L, 4);
	}

	// Ensure that the buffer will be of proper size
	COORD tmpCoord;
	tmpCoord.X = coord.X;
	tmpCoord.Y = coord.Y;
	if( lpcsbi.dwSize.X > tmpCoord.X )
		tmpCoord.X = lpcsbi.dwSize.X;
	if( lpcsbi.dwSize.Y > tmpCoord.Y )
		tmpCoord.Y = lpcsbi.dwSize.Y;

	// Note: We set the buffer both before and after because it is
	// easier than checking current vs. future screen size vs. buffer size
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), tmpCoord);
	SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), true, &rect);
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coord);

	return 0;
}
