/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "time_lua.h"
#include "timer.h"
#include "luatypes.h"
#include "int64_lua.h"
#include "error.h"
#include "macro.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}


int Time_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"getNow", Time_lua::getNow},
		{"deltaTime", Time_lua::deltaTime},
		{"diff", Time_lua::diff},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, TIME_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

/*	time.getNow()
	Returns:	table (int64)

	Returns the current high-precision time as an int64 table.
*/
int Time_lua::getNow(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	TimeType now = ::getNow();
	lua_pushint64(L, now);

	return 1;
}

/*	time.deltaTime()
	Returns:	number delta

	Returns the deltaTime for the current logic cycle
*/
int Time_lua::deltaTime(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	float dt = Macro::instance()->getEngine()->getDeltaTime();
	lua_pushnumber(L, dt);
	return 1;
}

/*	time.diff(int64 t2, int64 t1)
    time.diff(int64 t1)
	Returns:	number delta

	Compares two high-precision time values (from time.getNow())
	and returns the amount of time that has elapsed between them
	in seconds.
    If only one parameter is given, it compares that time value
    against now.
*/
int Time_lua::diff(lua_State *L)
{
    int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);

	TimeType t2, t1;

	checkType(L, LT_TABLE, 1);
	if( !lua_isint64(L, 1) )
		luaL_typerror(L, 1, LuaType::metatable_int64);

    if( top == 2 )
    {
        checkType(L, LT_TABLE, 2);
        if( !lua_isint64(L, 2) )
            luaL_typerror(L, 2, LuaType::metatable_int64);

        t2 = lua_toint64(L, 1);
        t1 = lua_toint64(L, 2);
    }
    else
    {
        t2 = ::getNow();
        t1 = lua_toint64(L, 1);
    }

	double dt = ::deltaTime(t2, t1);
	lua_pushnumber(L, dt);
	return 1;
}
