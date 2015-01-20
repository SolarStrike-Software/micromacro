/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include <ncursesw/ncurses.h>
#include "luatypes.h"
#include "types.h"
#include "error.h"
#include "strl.h"
#include "wininclude.h"
#include "event.h"
#include "macro.h"
#include "ncurses_lua.h"

#include <math.h>
#include <cmath>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

namespace LuaType
{
	// Metatable names
	const char *metatable_ncursesWindow = "ncurses.window";
	const char *metatable_handle = "process.handle";
	const char *metatable_windowDC = "window.windowDC";
	const char *metatable_audioResource = "audio.audioResource";
}

int registerLuaTypes(lua_State *L)
{
	// Ncurses windows
	luaL_newmetatable(L, LuaType::metatable_ncursesWindow);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, LuaType::ncursesWindow_gc);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::ncursesWindow_tostring);
	lua_settable(L, -3);
	lua_pop(L, 1); // Pop metatable


	// Handles
	luaL_newmetatable(L, LuaType::metatable_handle);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, LuaType::handle_gc);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::handle_tostring);
	lua_settable(L, -3);
	lua_pop(L, 1); // Pop metatable


	// Audio
	luaL_newmetatable(L, LuaType::metatable_audioResource);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, LuaType::audioResource_gc);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::audioResource_tostring);
	lua_settable(L, -3);
	lua_pop(L, 1); // Pop metatable

	return MicroMacro::ERR_OK;
}

int LuaType::ncursesWindow_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WINDOW **pw = (WINDOW **)lua_touserdata(L, 1);

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Ncurses Window: 0x%X", *pw);
	lua_pushstring(L, buffer);
	return 1;
}

int LuaType::ncursesWindow_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WINDOW **pw = (WINDOW **)lua_touserdata(L, 1);

	/* We really don't want to delete stdscr! */
	Ncurses_lua::safeDestroy(*pw);

	*pw = NULL;
	return 0;
}

int LuaType::handle_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	HANDLE *pHandle = (HANDLE *)lua_touserdata(L, 1);

	CloseHandle(*pHandle);
	*pHandle = NULL;
	return 0;
}

int LuaType::handle_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	HANDLE *pHandle = (HANDLE *)lua_touserdata(L, 1);

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Process handle: 0x%X", *pHandle);
	lua_pushstring(L, buffer);
	return 1;
}
/*
int LuaType::windowDC_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WindowDCPair *pWinDC = static_cast<WindowDCPair *>(lua_touserdata(L, 1));

	ReleaseDC(pWinDC->hwnd, pWinDC->hdc);
	pWinDC->hwnd = NULL; pWinDC->hdc = NULL;
	return 0;
}

int LuaType::windowDC_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WindowDCPair *pWinDC = static_cast<WindowDCPair *>(lua_touserdata(L, 1));

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Window device context 0x%X", *pWinDC);
	lua_pushstring(L, buffer);
	return 1;
}
*/

// Flush the sound data
int LuaType::audioResource_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	if( pResource->buffer != AL_NONE )
		alDeleteBuffers(1, &pResource->buffer);
	if( pResource->source != AL_NONE )
		alDeleteSources(1, &pResource->source);

	pResource->buffer = AL_NONE;
	pResource->source = AL_NONE;
	return 0;
}

int LuaType::audioResource_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Audio resource 0x%X", pResource);
	lua_pushstring(L, buffer);
	return 1;
}
