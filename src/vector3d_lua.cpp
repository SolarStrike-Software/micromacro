/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "vector3d_lua.h"
#include "luatypes.h"
#include "types.h"
#include "error.h"
#include "strl.h"

#include <math.h>
#include <cmath>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

int Vector3d_lua::regmod(lua_State *L)
{
	const luaL_Reg meta[] = {
		{"__tostring", tostring},
		{"__add", add},
		{"__sub", sub},
		{"__div", div},
		{"__mul", mul},
		{NULL, NULL}
	};

	const luaL_Reg methods[] = {
		{"set", set},
		{"length", length},
		{"normal", normal},
		{"rotateAboutX", rotateAboutX},
		{"rotateAboutY", rotateAboutY},
		{"rotateAboutZ", rotateAboutZ},
		{"rotateAbout", rotateAbout},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LuaType::metatable_vector3d);
	luaL_setfuncs(L, meta, 0);
	luaL_newlib(L, methods);
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1); // Pop table

	return MicroMacro::ERR_OK;
}

int Vector3d_lua::tostring(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	Vector3d vec = lua_tovector3d(L, 1);

	char buffer[64];
	slprintf(buffer, sizeof(buffer)-1, "(%0.1f, %0.1f, %0.1f)", vec.x, vec.y, vec.z);

	lua_pushstring(L, buffer);
	return 1;
}

int Vector3d_lua::add(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE, 2);

	Vector3d vec1 = lua_tovector3d(L, 1);
	Vector3d vec2 = lua_tovector3d(L, 2);
	Vector3d result;
	result.x = vec1.x + vec2.x;
	result.y = vec1.y + vec2.y;
	result.z = vec1.z + vec2.z;

	lua_pushvector3d(L, result);
	return 1;
}

int Vector3d_lua::sub(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	Vector3d vec1 = lua_tovector3d(L, 1);
	Vector3d vec2 = lua_tovector3d(L, 2);
	Vector3d result;
	result.x = vec1.x - vec2.x;
	result.y = vec1.y - vec2.y;
	result.z = vec1.z - vec2.z;

	lua_pushvector3d(L, result);
	return 1;
}

int Vector3d_lua::mul(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE, 2);

	Vector3d result;
	if( lua_istable(L, 2) )
	{
		Vector3d vec1 = lua_tovector3d(L, 1);
		Vector3d vec2 = lua_tovector3d(L, 2);

		result.x = vec1.x * vec2.x;
		result.y = vec1.y * vec2.y;
		result.z = vec1.z * vec2.z;
	}
	else
	{
		Vector3d vec1 = lua_tovector3d(L, 1);
		double scale = lua_tonumber(L, 2);

		result.x = vec1.x * scale;
		result.y = vec1.y * scale;
		result.z = vec1.z * scale;
	}

	lua_pushvector3d(L, result);
	return 1;
}

int Vector3d_lua::div(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_TABLE | LT_NUMBER, 2);

	Vector3d result;

	if( lua_istable(L, 2) )
	{
		Vector3d vec1 = lua_tovector3d(L, 1);
		Vector3d vec2 = lua_tovector3d(L, 2);

		result.x = vec1.x / vec2.x;
		result.y = vec1.y / vec2.y;
		result.z = vec1.z / vec2.z;
	}
	else
	{
		Vector3d vec1 = lua_tovector3d(L, 1);
		double scale = lua_tonumber(L, 2);

		result.x = vec1.x / scale;
		result.y = vec1.y / scale;
		result.z = vec1.z / scale;
	}

	// Prevent division by zero
	if( std::isnan(result.x) || std::isinf(result.x)
		|| std::isnan(result.y) || std::isinf(result.y)
		|| std::isnan(result.z) || std::isinf(result.z) )
	{
		pushLuaErrorEvent(L, "Attempt to divide by zero or illegal operation.");
		return 0;
	}

	lua_pushvector3d(L, result);
	return 1;
}

int Vector3d_lua::set(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 3 && top != 4 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);

	if( top == 4 )
	{
		checkType(L, LT_NUMBER, 2);
		checkType(L, LT_NUMBER, 3);
		checkType(L, LT_NUMBER, 4);
		lua_setfield(L, 1, "z"); // Set Z to value on top of stack, pop it
		lua_setfield(L, 1, "y"); // Set Y to value on top of stack, pop it
		lua_setfield(L, 1, "x"); // Set X to value on top of stack, pop it
	}
	else if( top == 3 )
	{
		checkType(L, LT_NUMBER, 2);
		checkType(L, LT_NUMBER, 3);
		lua_pushnumber(L, 0); // Push 0 for Z

		lua_setfield(L, 1, "z"); // Set Z to value on top of stack, pop it
		lua_setfield(L, 1, "y"); // Set Y to value on top of stack, pop it
		lua_setfield(L, 1, "x"); // Set X to value on top of stack, pop it
	}

	return 0;
}

int Vector3d_lua::length(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);

	Vector3d vec = lua_tovector3d(L, 1);

	lua_pushnumber(L, sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z));
	return 1;
}

int Vector3d_lua::normal(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);

	Vector3d vec = lua_tovector3d(L, 1);

	double scale = sqrtf(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);

	// Prevent division by zero
	if( std::isnan(scale) || std::isinf(scale) )
	{
		pushLuaErrorEvent(L, "Attempt to divide by zero or illegal operation.");
		return 0;
	}

	vec.x /= scale;
	vec.y /= scale;
	vec.z /= scale;

	lua_pushvector3d(L, vec);
	return 1;
}

int Vector3d_lua::rotateAboutX(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_NUMBER, 2);

	Vector3d vec = lua_tovector3d(L, 1);
	double angle = lua_tonumber(L, 2);
	float s = sinf(angle);
	float c = cosf(angle);

	vec.y = c*vec.y - s*vec.z;
	vec.z = c*vec.z + s*vec.y;

	lua_pushvector3d(L, vec);
	return 1;
}

int Vector3d_lua::rotateAboutY(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_NUMBER, 2);

	Vector3d vec = lua_tovector3d(L, 1);
	double angle = lua_tonumber(L, 2);
	float s = sinf(angle);
	float c = cosf(angle);

	vec.x = c*vec.x + s*vec.z;
	vec.z = c*vec.z - s*vec.x;

	lua_pushvector3d(L, vec);
	return 1;
}

int Vector3d_lua::rotateAboutZ(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_NUMBER, 2);

	Vector3d vec = lua_tovector3d(L, 1);
	double angle = lua_tonumber(L, 2);
	float s = sinf(angle);
	float c = cosf(angle);

	vec.x = c*vec.x - s*vec.y;
	vec.y = c*vec.y + s*vec.x;

	lua_pushvector3d(L, vec);
	return 1;
}

int Vector3d_lua::rotateAbout(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);
	checkType(L, LT_TABLE, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_TABLE, 3);

	Vector3d vec = lua_tovector3d(L, 1);
	double angle = lua_tonumber(L, 2);
	Vector3d axis = lua_tovector3d(L, 3);
	float s = sinf(angle);
	float c = cosf(angle);
	float k = 1.0f - c;

	vec.x = vec.x * (c + k*axis.x*axis.x)
			+ vec.y * (k*axis.x*axis.y - s*axis.z)
            + vec.z * (k*axis.x*axis.z + s*axis.y);
	vec.y = vec.x * (k*axis.x*axis.y + s*axis.z)
			+ vec.y * (c + k*axis.y*axis.y)
            + vec.z * (k*axis.y*axis.z - s*axis.x);
	vec.z = vec.x * (k*axis.x*axis.z - s*axis.y)
			+ vec.y * (k*axis.y*axis.z + s*axis.x)
            + vec.z * (c + k*axis.z*axis.z);

	lua_pushvector3d(L, vec);
	return 1;
}
