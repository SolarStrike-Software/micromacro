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
#include "debugmessages.h"

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
		{"shellExec", System_lua::shellExec},
		{"getClipboard", System_lua::getClipboard},
		{"setClipboard", System_lua::setClipboard},
		{"getActiveCodePage", System_lua::getActiveCodePage},
		{"getConsoleCodePage", System_lua::getConsoleCodePage},
		{"setPriority", System_lua::setPriority},
		{"repollHid", System_lua::repollHid},
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

/*	system.shellExec(table execinfo)
	Returns:	boolean

	A binding for ShellExecuteEx(); executes a command
	based on the information passed into the table.
*/
int System_lua::shellExec(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);

	SHELLEXECUTEINFO sei;
	memset(&sei, 0, sizeof(sei));

	// Set required and default values.
	sei.cbSize = sizeof(sei);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.nShow = SW_SHOWNORMAL;

	lua_pushnil(L);
	while( lua_next(L, 1) )
	{
		const char *key = lua_tostring(L, -2);
		size_t len = strlen(key);

		if( strncmp(key, "fMask", len) == 0 )
		{
			sei.fMask = lua_tointeger(L, -1);
		}
		else if( strncmp(key, "hwnd", len) == 0 )
		{
			sei.hwnd = (HWND)lua_tointeger(L, -1);
		}
		else if( strncmp(key, "lpVerb", len) == 0 )
		{
			sei.lpVerb = lua_tostring(L, -1);
		}
		else if( strncmp(key, "lpFile", len) == 0 )
		{
			sei.lpFile = lua_tostring(L, -1);
		}
		else if( strncmp(key, "lpParameters", len) == 0 )
		{
			sei.lpParameters = lua_tostring(L, -1);
		}
		else if( strncmp(key, "lpDirectory", len) == 0 )
		{
			sei.lpDirectory = lua_tostring(L, -1);
		}
		else if( strncmp(key, "nShow", len) == 0 )
		{
			sei.nShow = lua_tointeger(L, -1);
		}
		else if( strncmp(key, "hInstApp", len) == 0 )
		{
			sei.hInstApp = (HINSTANCE)lua_tointeger(L, -1);
		}
		else if( strncmp(key, "lpIDList", len) == 0 )
		{
			sei.lpIDList = (LPVOID)lua_tointeger(L, -1);
		}
		else if( strncmp(key, "lpClass", len) == 0 )
		{
			sei.lpClass = lua_tostring(L, -1);
		}
		else if( strncmp(key, "hkeyClass", len) == 0 )
		{
			sei.hkeyClass = (HKEY)lua_tointeger(L, -1);
		}
		else if( strncmp(key, "dwHotKey", len) == 0 )
		{
			sei.dwHotKey = lua_tointeger(L, -1);
		}
		else if( strncmp(key, "hIcon", len) == 0 )
		{
			sei.hIcon = (HANDLE)lua_tointeger(L, -1);
		}
		/*else if( strncmp(key, "hMonitor", len) == 0 )
		{
			sei.hMonitor = (HANDLE)lua_tointeger(L, -1);
		}*/

		lua_pop(L, 1);
	}

	bool success = ShellExecuteEx(&sei);

	if( !success )
	{
		debugMessage("ShellExecuteEx() has returned in error. Code: %d\n", GetLastError());
		return 0;
	}

	// Wait for process to idle
	DWORD idle = WaitForInputIdle(sei.hProcess, SEI_IDLE_TIMEOUT_SECS*1000);
	if( idle != 0 )
	{
		if( idle == WAIT_TIMEOUT )
		{
			debugMessage("Launching process has timed out during wait for idle event.");
		}
		else if( idle == WAIT_FAILED)
		{	// Note, this is triggered if the target is a console app, even though no actual error has occurred.
			debugMessage("Launching process has encountered an error during wait for idle event. Error code: %d", GetLastError());
		}
	}

	// Extract PID
	DWORD pid = GetProcessId(sei.hProcess);
	if( sei.hProcess )
		CloseHandle(sei.hProcess);

	lua_pushinteger(L, pid);
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

/*	system.repollHid()
	Returns:	nil

	Repolls human input devices
*/
int System_lua::repollHid(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

    Macro::instance()->getHid()->repollGamepadMaxIndex();
    Macro::instance()->getHid()->poll();
	return 0;
}

