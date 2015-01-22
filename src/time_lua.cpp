/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "time_lua.h"
#include "timer.h"
#include "luatypes.h"
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
	Returns:	integer

	Returns the current high-precision time as an int64.
*/
int Time_lua::getNow(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	TimeType now = ::getNow();
	lua_pushinteger(L, now.QuadPart);

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

/*	time.diff(number t2, number t1)
    time.diff(number t1)
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
	checkType(L, LT_NUMBER, 1);

    if( top == 2 )
    {
		checkType(L, LT_NUMBER, 2);

        t2.QuadPart = lua_tointeger(L, 1);
        t1.QuadPart = lua_tointeger(L, 2);
    }
    else
    {
        t2 = ::getNow();
        t1.QuadPart = lua_tointeger(L, 1);
    }

	double dt = ::deltaTime(t2, t1);
	lua_pushnumber(L, dt);
	return 1;
}
