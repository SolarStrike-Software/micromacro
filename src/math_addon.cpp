/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "math_addon.h"
#include "error.h"
#include "luatypes.h"
#include "types.h"

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

	lua_pushcfunction(L, Math_addon::distance);
	lua_setfield(L, -2, "distance");

	lua_pushcfunction(L, Math_addon::vector3d);
	lua_setfield(L, -2, "vector3d");

	lua_pop(L, 1); // Pop math module

	return MicroMacro::ERR_OK;
}

/*	math.distance(x1, y1, x2, y2)
	Returns:	number

	Fairly standard distance function: returns the
	distance between two points.
*/
int Math_addon::distance(lua_State *L)
{
	if( lua_gettop(L) != 4 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);
	checkType(L, LT_NUMBER, 4);

	double x1 = lua_tonumber(L, 1);
	double y1 = lua_tonumber(L, 2);
	double x2 = lua_tonumber(L, 3);
	double y2 = lua_tonumber(L, 4);

	lua_pushnumber(L, sqrt((y2-y1)*(y2-y1) + (x2-x1)*(x2-x1)));
	return 1;
}

/*	math.vector3d([number x, number y, number z])
	Returns:	table (class)

	Create a new table (class) of vector3d.
	If x and y are given, the new vector3d retains
	the given values.

	The vector3d class contains metamethods for
	operations such as vector scaling and dot product.
*/
int Math_addon::vector3d(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 0 && top !=2 && top != 3 )
		wrongArgs(L);

	//double x = 0.0; double y = 0.0; double z = 0.0;
	Vector3d vec(0.0, 0.0, 0.0);
	if( top == 3 )
	{
		checkType(L, LT_NUMBER, 1);
		checkType(L, LT_NUMBER, 2);
		checkType(L, LT_NUMBER, 3);
		vec.x = lua_tonumber(L, 1);
		vec.y = lua_tonumber(L, 2);
		vec.z = lua_tonumber(L, 3);
	}
	else if( top == 2 )
	{
		checkType(L, LT_NUMBER, 1);
		checkType(L, LT_NUMBER, 2);
		vec.x = lua_tonumber(L, 1);
		vec.y = lua_tonumber(L, 2);
		vec.z = 0;
	}

	lua_pushvector3d(L, vec);
	return 1;
}
