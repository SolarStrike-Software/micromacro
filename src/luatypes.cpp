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

#include <math.h>
#include <cmath>

/*extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}*/

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

namespace LuaType
{
	/*const luaL_Reg vector2d_methods[] = {
		{"set", vector2d_set},
		{NULL, NULL}
	};*/

	// Metatable names
	const char *metatable_int64 = "int64";
	const char *metatable_ncursesWindow = "ncurses.window";
	const char *metatable_handle = "process.handle";
	const char *metatable_windowDC = "window.windowDC";
	const char *metatable_audioResource = "audio.audioResource";
	const char *metatable_vector2d = "vector2d";

	// Parts of int64
	const char *highpart_name = "high";
	const char *lowpart_name = "low";
}

int registerLuaTypes(lua_State *L)
{
	/* int64 */
	luaL_newmetatable(L, LuaType::metatable_int64);
	lua_pushstring(L, "__add");
	lua_pushcfunction(L, LuaType::int64_add);
	lua_settable(L, -3);
	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, LuaType::int64_subtract);
	lua_settable(L, -3);
	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, LuaType::int64_multiply);
	lua_settable(L, -3);
	lua_pushstring(L, "__div");
	lua_pushcfunction(L, LuaType::int64_divide);
	lua_settable(L, -3);
	lua_pushstring(L, "__eq");
	lua_pushcfunction(L, LuaType::int64_eq);
	lua_settable(L, -3);
	lua_pushstring(L, "__lt");
	lua_pushcfunction(L, LuaType::int64_lt);
	lua_settable(L, -3);
	lua_pushstring(L, "__gt");
	lua_pushcfunction(L, LuaType::int64_gt);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::int64_tostring);
	lua_settable(L, -3);
	lua_pop(L, 1); // Pop metatable

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

	// Vector2d
	luaL_newmetatable(L, LuaType::metatable_vector2d);
	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::vector2d_tostring);
	lua_settable(L, -3);

	lua_pushstring(L, "__add");
	lua_pushcfunction(L, LuaType::vector2d_add);
	lua_settable(L, -3);

	lua_pushstring(L, "__sub");
	lua_pushcfunction(L, LuaType::vector2d_sub);
	lua_settable(L, -3);

	lua_pushstring(L, "__mul");
	lua_pushcfunction(L, LuaType::vector2d_mul);
	lua_settable(L, -3);

	lua_pushstring(L, "__div");
	lua_pushcfunction(L, LuaType::vector2d_div);
	lua_settable(L, -3);

	lua_pop(L, 1); // Pop metatable

	return MicroMacro::ERR_OK;
}

int LuaType::int64_add(lua_State *lstate)
{
	if( lua_gettop(lstate) != 2 )
		wrongArgs(lstate);
	checkType(lstate, LT_TABLE | LT_NUMBER, 1);
	checkType(lstate, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t3, t2, t1; // t3 = return value, t1/t2 = inputs

	// Get info from arg 1
	if( lua_isnumber(lstate, 1) )
	{
		t2.HighPart = 0; t2.LowPart = (unsigned int)lua_tonumber(lstate, 1);
	}
	else
		t2 = lua_toint64(lstate, 1);

	// Get info from arg 2
	if( lua_isnumber(lstate, 2) )
	{
		t1.HighPart = 0; t1.LowPart = (unsigned int)lua_tonumber(lstate, 2);
	}
	else
		t1 = lua_toint64(lstate, 2);


	t3.QuadPart = t2.QuadPart + t1.QuadPart;

	lua_newtable(lstate);
	luaL_getmetatable(lstate, metatable_int64);
	lua_setmetatable(lstate, -2);

	lua_pushstring(lstate, LuaType::highpart_name); //key
	lua_pushnumber(lstate, t3.HighPart); //value
	lua_settable(lstate, -3);

	lua_pushstring(lstate, LuaType::lowpart_name); //key
	lua_pushnumber(lstate, t3.LowPart); //value
	lua_settable(lstate, -3);

	return 1;
}

int LuaType::int64_subtract(lua_State *lstate)
{
	if( lua_gettop(lstate) != 2 )
		wrongArgs(lstate);
	checkType(lstate, LT_TABLE | LT_NUMBER, 1);
	checkType(lstate, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t3, t2, t1; // t3 = return value, t1/t2 = inputs

	// Get info from arg 1
	if( lua_isnumber(lstate, 1) )
	{
		t2.HighPart = 0; t2.LowPart = (unsigned int)lua_tonumber(lstate, 1);
	}
	else
		t2 = lua_toint64(lstate, 1);

	// Get info from arg 2
	if( lua_isnumber(lstate, 2) )
	{
		t1.HighPart = 0; t1.LowPart = (unsigned int)lua_tonumber(lstate, 2);
	}
	else
		t1 = lua_toint64(lstate, 2);


	t3.QuadPart = t2.QuadPart - t1.QuadPart;

	lua_newtable(lstate);
	luaL_getmetatable(lstate, metatable_int64);
	lua_setmetatable(lstate, -2);

	lua_pushstring(lstate, LuaType::highpart_name); //key
	lua_pushnumber(lstate, t3.HighPart); //value
	lua_settable(lstate, -3);

	lua_pushstring(lstate, LuaType::lowpart_name); //key
	lua_pushnumber(lstate, t3.LowPart); //value
	lua_settable(lstate, -3);

	return 1;
}

int LuaType::int64_multiply(lua_State *lstate)
{
	if( lua_gettop(lstate) != 2 )
		wrongArgs(lstate);
	checkType(lstate, LT_TABLE | LT_NUMBER, 1);
	checkType(lstate, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t3, t2, t1; // t3 = return value, t1/t2 = inputs

	// Get info from arg 1
	if( lua_isnumber(lstate, 1) )
	{
		t2.HighPart = 0; t2.LowPart = (unsigned int)lua_tonumber(lstate, 1);
	}
	else
		t2 = lua_toint64(lstate, 1);

	// Get info from arg 2
	if( lua_isnumber(lstate, 2) )
	{
		t1.HighPart = 0; t1.LowPart = (unsigned int)lua_tonumber(lstate, 2);
	}
	else
		t1 = lua_toint64(lstate, 2);


	t3.QuadPart = t2.QuadPart * t1.QuadPart;

	lua_newtable(lstate);
	luaL_getmetatable(lstate, metatable_int64);
	lua_setmetatable(lstate, -2);

	lua_pushstring(lstate, LuaType::highpart_name); //key
	lua_pushnumber(lstate, t3.HighPart); //value
	lua_settable(lstate, -3);

	lua_pushstring(lstate, LuaType::lowpart_name); //key
	lua_pushnumber(lstate, t3.LowPart); //value
	lua_settable(lstate, -3);

	return 1;
}

int LuaType::int64_divide(lua_State *lstate)
{
	if( lua_gettop(lstate) != 2 )
		wrongArgs(lstate);
	checkType(lstate, LT_TABLE | LT_NUMBER, 1);
	checkType(lstate, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t3, t2, t1; // t3 = return value, t1/t2 = inputs

	// Get info from arg 1
	if( lua_isnumber(lstate, 1) )
	{
		t2.HighPart = 0; t2.LowPart = (unsigned int)lua_tonumber(lstate, 1);
	}
	else
		t2 = lua_toint64(lstate, 1);

	// Get info from arg 2
	if( lua_isnumber(lstate, 2) )
	{
		t1.HighPart = 0; t1.LowPart = (unsigned int)lua_tonumber(lstate, 2);
	}
	else
		t1 = lua_toint64(lstate, 2);


	t3.QuadPart = t2.QuadPart / t1.QuadPart;

	lua_newtable(lstate);
	luaL_getmetatable(lstate, metatable_int64);
	lua_setmetatable(lstate, -2);

	lua_pushstring(lstate, LuaType::highpart_name); //key
	lua_pushnumber(lstate, t3.HighPart); //value
	lua_settable(lstate, -3);

	lua_pushstring(lstate, LuaType::lowpart_name); //key
	lua_pushnumber(lstate, t3.LowPart); //value
	lua_settable(lstate, -3);

	return 1;
}

int LuaType::int64_eq(lua_State *lstate)
{
	if( lua_gettop(lstate) != 2 )
		wrongArgs(lstate);
	checkType(lstate, LT_TABLE | LT_NUMBER, 1);
	checkType(lstate, LT_TABLE | LT_NUMBER, 2);

	unsigned long high2, low2, high1, low1;
	LARGE_INTEGER t2, t1;

	// Get info from arg 2
	if( lua_isnumber(lstate, 2) )
	{
		high1 = 0; low1 = (int)lua_tonumber(lstate, 2);
	}
	else
	{ // Must be a table
		lua_pushstring(lstate, LuaType::highpart_name);
		lua_gettable(lstate, -2);
		high1 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);

		lua_pushstring(lstate, LuaType::lowpart_name);
		lua_gettable(lstate, -2);
		low1 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);
	}

	// Switch tables now
	lua_pop(lstate, 1);

	// Get info from arg 1
	if( lua_isnumber(lstate, 1) )
	{
		high2 = 0; low2 = (int)lua_tonumber(lstate, 1);
	}
	else
	{
		lua_pushstring(lstate, LuaType::highpart_name);
		lua_gettable(lstate, -2);
		high2 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);

		lua_pushstring(lstate, LuaType::lowpart_name);
		lua_gettable(lstate, -2);
		low2 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);
	}

	t2.HighPart = high2; t2.LowPart = low2;
	t1.HighPart = high1; t1.LowPart = low1;

	if( t1.QuadPart == t2.QuadPart )
		lua_pushboolean(lstate, true);
	else
		lua_pushboolean(lstate, false);

	return 1;
}

int LuaType::int64_lt(lua_State *lstate)
{
	if( lua_gettop(lstate) != 2 )
		wrongArgs(lstate);
	checkType(lstate, LT_TABLE | LT_NUMBER, 1);
	checkType(lstate, LT_TABLE | LT_NUMBER, 2);

	unsigned long high2, low2, high1, low1;
	LARGE_INTEGER t2, t1;

	// Get info from arg 2
	if( lua_isnumber(lstate, 2) )
	{
		high1 = 0; low1 = (int)lua_tonumber(lstate, 2);
	}
	else
	{ // Must be a table
		lua_pushstring(lstate, LuaType::highpart_name);
		lua_gettable(lstate, -2);
		high1 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);

		lua_pushstring(lstate, LuaType::lowpart_name);
		lua_gettable(lstate, -2);
		low1 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);
	}

	// Switch tables now
	lua_pop(lstate, 1);

	// Get info from arg 1
	if( lua_isnumber(lstate, 1) )
	{
		high2 = 0; low2 = (int)lua_tonumber(lstate, 1);
	}
	else
	{
		lua_pushstring(lstate, LuaType::highpart_name);
		lua_gettable(lstate, -2);
		high2 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);

		lua_pushstring(lstate, LuaType::lowpart_name);
		lua_gettable(lstate, -2);
		low2 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);
	}

	t2.HighPart = high2; t2.LowPart = low2;
	t1.HighPart = high1; t1.LowPart = low1;

	if( t2.QuadPart < t1.QuadPart )
		lua_pushboolean(lstate, true);
	else
		lua_pushboolean(lstate, false);

	return 1;
}

int LuaType::int64_gt(lua_State *lstate)
{
	if( lua_gettop(lstate) != 2 )
		wrongArgs(lstate);
	checkType(lstate, LT_TABLE | LT_NUMBER, 1);
	checkType(lstate, LT_TABLE | LT_NUMBER, 2);

	unsigned long high2, low2, high1, low1;
	LARGE_INTEGER t2, t1;

	// Get info from arg 2
	if( lua_isnumber(lstate, 2) )
	{
		high1 = 0; low1 = (int)lua_tonumber(lstate, 2);
	}
	else
	{ // Must be a table
		lua_pushstring(lstate, LuaType::highpart_name);
		lua_gettable(lstate, -2);
		high1 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);

		lua_pushstring(lstate, LuaType::lowpart_name);
		lua_gettable(lstate, -2);
		low1 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);
	}

	// Switch tables now
	lua_pop(lstate, 1);

	// Get info from arg 1
	if( lua_isnumber(lstate, 1) )
	{
		high2 = 0; low2 = (int)lua_tonumber(lstate, 1);
	}
	else
	{
		lua_pushstring(lstate, LuaType::highpart_name);
		lua_gettable(lstate, -2);
		high2 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);

		lua_pushstring(lstate, LuaType::lowpart_name);
		lua_gettable(lstate, -2);
		low2 = (unsigned long)lua_tonumber(lstate, -1);
		lua_pop(lstate, 1);
	}

	t2.HighPart = high2; t2.LowPart = low2;
	t1.HighPart = high1; t1.LowPart = low1;

	if( t2.QuadPart > t1.QuadPart )
		lua_pushboolean(lstate, true);
	else
		lua_pushboolean(lstate, false);

	return 1;
}

int LuaType::int64_tostring(lua_State *lstate)
{
	checkType(lstate, LT_TABLE, 1);
	LARGE_INTEGER t = lua_toint64(lstate, 1);

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "int64(%u,%u)", (unsigned int)t.HighPart, (unsigned int)t.LowPart);
	lua_pushstring(lstate, buffer);
	return 1;
}


/* Read a LARGE_INTEGER from the Lua stack */
LARGE_INTEGER lua_toint64(lua_State *lstate, int index)
{
	LARGE_INTEGER retval;

	// Unnecessary
	lua_pushvalue(lstate, index); // Copy it so we don't screw up the stack

	lua_pushstring(lstate, LuaType::highpart_name);
	lua_gettable(lstate, -2);
	if( lua_isnumber(lstate, -1) )
		retval.HighPart = (unsigned long)lua_tonumber(lstate, -1);
	else
		retval.HighPart = 0;
	lua_pop(lstate, 1);

	lua_pushstring(lstate, LuaType::lowpart_name);
	lua_gettable(lstate, -2);
	if( lua_isnumber(lstate, -1) )
		retval.LowPart = (unsigned long)lua_tonumber(lstate, -1);
	else
		retval.LowPart = 0;
	lua_pop(lstate, 1);

	lua_pop(lstate, 1); // Pop our copy off the stack

	return retval;
}

bool lua_isint64(lua_State *L, int index)
{
	if( !lua_istable(L, index) )
		return false;

	bool retval = true;

	lua_pushstring(L, LuaType::highpart_name);
	lua_gettable(L, -2);
	if( !lua_isnumber(L, -1) )
		retval = false;
	lua_pop(L, 1);

	lua_pushstring(L, LuaType::lowpart_name);
	lua_gettable(L, -2);
	if( !lua_isnumber(L, -1) )
		retval = false;
	lua_pop(L, 1);

	return retval;
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
	if( *pw != ::stdscr )
		delwin(*pw);

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

int LuaType::vector2d_tostring(lua_State *L)
{
	checkType(L, LT_TABLE, 1);

	double x; double y;
	lua_getfield(L, 1, "x");
	x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "y");
	y = lua_tonumber(L, -1);
	lua_pop(L, 1);

	char buffer[32];
	slprintf(buffer, sizeof(buffer)-1, "(%0.1f, %0.1f)", x, y);

	lua_pushstring(L, buffer);
	return 1;
}

int LuaType::vector2d_add(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE, 2);

	double x1, y1, x2, y2;

	// Vector 1
	lua_getfield(L, 1, "x");
	x1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "y");
	y1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Vector 2
	lua_getfield(L, 2, "x");
	x2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 2, "y");
	y2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Add them to a new table, return it.
	lua_newtable(L);

	luaL_getmetatable(L, LuaType::metatable_vector2d);
	luaL_newlib(L, vector2d_methods);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	lua_pushstring(L, "x");
	lua_pushnumber(L, x1 + x2);
	lua_settable(L, -3);

	lua_pushstring(L, "y");
	lua_pushnumber(L, y1 + y2);
	lua_settable(L, -3);

	return 1;
}

int LuaType::vector2d_sub(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE, 2);

	double x1, y1, x2, y2;

	// Vector 1
	lua_getfield(L, 1, "x");
	x1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "y");
	y1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Vector 2
	lua_getfield(L, 2, "x");
	x2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 2, "y");
	y2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Add them to a new table, return it.
	lua_newtable(L);

	luaL_getmetatable(L, LuaType::metatable_vector2d);
	luaL_newlib(L, vector2d_methods);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	lua_pushstring(L, "x");
	lua_pushnumber(L, x1 - x2);
	lua_settable(L, -3);

	lua_pushstring(L, "y");
	lua_pushnumber(L, y1 - y2);
	lua_settable(L, -3);

	return 1;
}

int LuaType::vector2d_mul(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE, 2);

	double x1, y1, x2, y2;

	// Vector 1
	lua_getfield(L, 1, "x");
	x1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "y");
	y1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Vector 2
	lua_getfield(L, 2, "x");
	x2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 2, "y");
	y2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Add them to a new table, return it.
	lua_newtable(L);

	luaL_getmetatable(L, LuaType::metatable_vector2d);
	luaL_newlib(L, vector2d_methods);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	lua_pushstring(L, "x");
	lua_pushnumber(L, x1 * x2);
	lua_settable(L, -3);

	lua_pushstring(L, "y");
	lua_pushnumber(L, y1 * y2);
	lua_settable(L, -3);

	return 1;
}

int LuaType::vector2d_div(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE, 2);

	double x1, y1, x2, y2;

	// Vector 1
	lua_getfield(L, 1, "x");
	x1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "y");
	y1 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Vector 2
	lua_getfield(L, 2, "x");
	x2 = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 2, "y");
	y2 = lua_tonumber(L, -1);
	lua_pop(L, 1);


	double nx, ny;
	nx = x1 / x2;
	ny = y1 / y2;

	// Prevent division by zero
	if( std::isnan(nx) || std::isinf(nx) || std::isnan(ny) || std::isinf(ny) )
	{
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Attempt to divide by zero or illegal operation. %s:%d",
			script, line);

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
		return 0;
	}

	// Add them to a new table, return it.
	lua_newtable(L);

	luaL_getmetatable(L, LuaType::metatable_vector2d);
	luaL_newlib(L, vector2d_methods);
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);

	lua_pushstring(L, "x");
	lua_pushnumber(L, nx);
	lua_settable(L, -3);

	lua_pushstring(L, "y");
	lua_pushnumber(L, ny);
	lua_settable(L, -3);

	return 1;
}

int LuaType::vector2d_set(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);

	lua_setfield(L, 1, "y"); // Set Y to value on top of stack, pop it
	lua_setfield(L, 1, "x"); // Set X to value on top of stack, pop it
	return 0;
}

int LuaType::vector2d_length(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);

	double x; double y;
	lua_getfield(L, 1, "x");
	x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "y");
	y = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_pushnumber(L, sqrt(x*x + y*y));
	return 1;
}
