/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "math_addon.h"
#include "error.h"

#include <math.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

int Math_addon::regmod(lua_State *L)
{
	lua_getglobal(L, MATH_MODULE_NAME);

	lua_pushcfunction(L, Math_addon::round);
	lua_setfield(L, -2, "round");

	lua_pushcfunction(L, Math_addon::distance);
	lua_setfield(L, -2, "distance");

	lua_pop(L, 1); // Pop math module

	return MicroMacro::ERR_OK;
}

/*	math.round(value)
	Returns:	number

	Rounds a number to the nearest whole value
*/
int Math_addon::round(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);

	checkType(L, LT_NUMBER, 1);

	double value = lua_tonumber(L, 1);
	lua_pushnumber(L, ::round(value));
	return 1;
}

/*	math.distance(x1, y1, x2, y2)
    math.distance(x1, y1, z1, x2, y2, z2)
	Returns:	number

	Fairly standard distance function: returns the
	distance between two points.
    Accepts either 2D or 3D points.
*/
int Math_addon::distance(lua_State *L)
{
    int top = lua_gettop(L);
	if( top != 4 && top != 6 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);
	checkType(L, LT_NUMBER, 4);
    if( top >= 6 )
    {
        checkType(L, LT_NUMBER, 5);
        checkType(L, LT_NUMBER, 6);
    }

    if( top == 4 )
    {
        double x1 = lua_tonumber(L, 1);
        double y1 = lua_tonumber(L, 2);
        double x2 = lua_tonumber(L, 3);
        double y2 = lua_tonumber(L, 4);

        lua_pushnumber(L, sqrt((y2-y1)*(y2-y1) + (x2-x1)*(x2-x1)));
    }
    else if( top == 6 )
    {
        double x1 = lua_tonumber(L, 1);
        double y1 = lua_tonumber(L, 2);
        double z1 = lua_tonumber(L, 3);
        double x2 = lua_tonumber(L, 4);
        double y2 = lua_tonumber(L, 5);
        double z2 = lua_tonumber(L, 6);

        lua_pushnumber(L, sqrt((z2-z1)*(z2-z1) + (y2-y1)*(y2-y1) + (x2-x1)*(x2-x1)));
    }

	return 1;
}
