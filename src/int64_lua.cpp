/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include "int64_lua.h"
#include "luatypes.h"
#include "error.h"
#include "wininclude.h"
#include "strl.h"

// Parts of int64
const char *LuaType::metatable_int64 = "int64";
const char *LuaType::int64_highpart_name = "high";
const char *LuaType::int64_lowpart_name = "low";

int Int64_lua::regmod(lua_State *L)
{
	const luaL_Reg meta[] = {
		{"__tostring", tostring},
		{"__add", add},
		{"__sub", sub},
		{"__div", div},
		{"__mul", mul},
		{"__eq", eq},
		{"__lt", lt},
		{"__gt", gt},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LuaType::metatable_int64);
	luaL_setfuncs(L, meta, 0);

	lua_pop(L, 1); // Pop table

	return MicroMacro::ERR_OK;
}

int Int64_lua::tostring(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	LARGE_INTEGER t = lua_toint64(L, 1);

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "%lld", t.QuadPart);
	lua_pushstring(L, buffer);
	return 1;
}

int Int64_lua::add(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t1 = lua_toint64(L, 1);
	LARGE_INTEGER t2, t3;

	if( lua_isnumber(L, 2) )
		t2.QuadPart = lua_tointeger(L, 2);
	else
		t2 = lua_toint64(L, 2);

	t3.QuadPart = t1.QuadPart + t2.QuadPart;
	lua_pushint64(L, t3);
	return 1;
}

int Int64_lua::sub(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t1 = lua_toint64(L, 1);
	LARGE_INTEGER t2, t3;

	if( lua_isnumber(L, 2) )
		t2.QuadPart = lua_tointeger(L, 2);
	else
		t2 = lua_toint64(L, 2);

	t3.QuadPart = t1.QuadPart - t2.QuadPart;
	lua_pushint64(L, t3);
	return 1;
}

int Int64_lua::mul(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t1 = lua_toint64(L, 1);
	LARGE_INTEGER t2, t3;

	if( lua_isnumber(L, 2) )
		t2.QuadPart = lua_tointeger(L, 2);
	else
		t2 = lua_toint64(L, 2);

	t3.QuadPart = t1.QuadPart * t2.QuadPart;
	lua_pushint64(L, t3);
	return 1;
}

int Int64_lua::div(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t1 = lua_toint64(L, 1);
	LARGE_INTEGER t2, t3;

	if( lua_isnumber(L, 2) )
		t2.QuadPart = lua_tointeger(L, 2);
	else
		t2 = lua_toint64(L, 2);

	t3.QuadPart = t1.QuadPart / t2.QuadPart;
	lua_pushint64(L, t3);
	return 1;
}

int Int64_lua::eq(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t1 = lua_toint64(L, 1);
	LARGE_INTEGER t2;

	if( lua_isnumber(L, 2) )
		t2.QuadPart = lua_tointeger(L, 2);
	else
		t2 = lua_toint64(L, 2);

	lua_pushboolean(L, t1.QuadPart == t2.QuadPart);
	return 1;
}

int Int64_lua::lt(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t1 = lua_toint64(L, 1);
	LARGE_INTEGER t2;

	if( lua_isnumber(L, 2) )
		t2.QuadPart = lua_tointeger(L, 2);
	else
		t2 = lua_toint64(L, 2);

	lua_pushboolean(L, t1.QuadPart < t2.QuadPart);
	return 1;
}

int Int64_lua::gt(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	LARGE_INTEGER t1 = lua_toint64(L, 1);
	LARGE_INTEGER t2;

	if( lua_isnumber(L, 2) )
		t2.QuadPart = lua_tointeger(L, 2);
	else
		t2 = lua_toint64(L, 2);

	lua_pushboolean(L, t1.QuadPart > t2.QuadPart);
	return 1;
}



bool lua_isint64(lua_State *L, int index)
{
	if( !lua_istable(L, index) )
		return false;

	bool retval = true;

	lua_pushstring(L, LuaType::int64_highpart_name);
	lua_gettable(L, -2);
	if( !lua_isnumber(L, -1) )
		retval = false;
	lua_pop(L, 1);

	lua_pushstring(L, LuaType::int64_lowpart_name);
	lua_gettable(L, -2);
	if( !lua_isnumber(L, -1) )
		retval = false;
	lua_pop(L, 1);

	return retval;
}

/* Read a LARGE_INTEGER from the Lua stack */
LARGE_INTEGER lua_toint64(lua_State *lstate, int index)
{
	LARGE_INTEGER retval;

	// Unnecessary
	lua_pushvalue(lstate, index); // Copy it so we don't screw up the stack

	lua_pushstring(lstate, LuaType::int64_highpart_name);
	lua_gettable(lstate, -2);
	if( lua_isnumber(lstate, -1) )
		retval.HighPart = (unsigned long)lua_tonumber(lstate, -1);
	else
		retval.HighPart = 0;
	lua_pop(lstate, 1);

	lua_pushstring(lstate, LuaType::int64_lowpart_name);
	lua_gettable(lstate, -2);
	if( lua_isnumber(lstate, -1) )
		retval.LowPart = (unsigned long)lua_tonumber(lstate, -1);
	else
		retval.LowPart = 0;
	lua_pop(lstate, 1);

	lua_pop(lstate, 1); // Pop our copy off the stack

	return retval;
}

void lua_pushint64(lua_State *L, LARGE_INTEGER value)
{
	lua_newtable(L);
	luaL_getmetatable(L, LuaType::metatable_int64);
	lua_setmetatable(L, -2);

	lua_pushstring(L, LuaType::int64_highpart_name); //key
	lua_pushnumber(L, (unsigned long)value.HighPart); //value
	lua_settable(L, -3);

	lua_pushstring(L, LuaType::int64_lowpart_name); //key
	lua_pushnumber(L, (unsigned long)value.LowPart); //value
	lua_settable(L, -3);
}
