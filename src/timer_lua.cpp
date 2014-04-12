/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "timer_lua.h"
#include "timer.h"
#include "luatypes.h"
#include "error.h"
extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}



int Timer_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"getNow", Timer_lua::getNow},
		{"deltaTime", Timer_lua::deltaTime},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, TIMER_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

int Timer_lua::getNow(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);


	TimeType now = ::getNow();
	unsigned long high, low;
	high = (unsigned long)now.HighPart;
	low = (unsigned long)now.LowPart;

	lua_newtable(L);
	luaL_getmetatable(L, LuaType::metatable_int64);
	lua_setmetatable(L, -2);

	lua_pushstring(L, LuaType::highpart_name); //key
	lua_pushnumber(L, high); //value
	lua_settable(L, -3);

	lua_pushstring(L, LuaType::lowpart_name); //key
	lua_pushnumber(L, low); //value
	lua_settable(L, -3);

	return 1;
}

int Timer_lua::deltaTime(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE, 2);

	if( !lua_isint64(L, 1) )
		luaL_typerror(L, 1, LuaType::metatable_int64);
	if( !lua_isint64(L, 2) )
		luaL_typerror(L, 2, LuaType::metatable_int64);

	TimeType t2, t1;
	t2 = lua_toint64(L, 1);
	t1 = lua_toint64(L, 2);

	double dt = ::deltaTime(t2, t1);
	lua_pushnumber(L, dt);
	return 1;
}
