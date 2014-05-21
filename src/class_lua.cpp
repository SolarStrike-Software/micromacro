/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "class_lua.h"
#include "error.h"
#include "strl.h"
#include "vector3d_lua.h"
#include "types.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

int Class_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"new", Class_lua::_new},
		{"vector3d", Class_lua::vector3d},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, CLASS_MODULE_NAME);

	return MicroMacro::ERR_OK;
}


/*	class.new([table baseclass])
	Returns:	table (new class)

	Create a new class. If baseclass is given, creates a child of it,
	and set the 'parent' variable to its base.

	This will *not* call the constructor. That is what the __call
	operator on the returned class is for: to create an instance of the class.
*/
int Class_lua::_new(lua_State *L)
{
	static const luaL_Reg class_meta[] = {
		{"__call", __call},
		{"__tostring", __tostring},
		{NULL, NULL}
	};

	int top = lua_gettop(L);
	if( top != 0 && top != 1 )
		wrongArgs(L);

	// Basic class setup
	lua_newtable(L);
	int newtab_index = lua_gettop(L);

	if( top == 0 ) // Create a base class
	{
		lua_newtable(L);
		luaL_setfuncs(L, class_meta, 0);

		lua_pushvalue(L, newtab_index);
		lua_setfield(L, newtab_index, "__index");

		lua_setmetatable(L, newtab_index);

		lua_pushstring(L, "__call");
		lua_pushcfunction(L, Class_lua::__call);
		lua_settable(L, -3);
	}
	else if( top == 1 ) // Create a subclass
	{
		// Copy methods & variables
		lua_pushnil(L);
		while( lua_next(L, 1) )
		{
			lua_pushvalue(L, -2); // Make a copy of key for next iteration
			lua_insert(L, -2); // Move our copy down the stack so it remains after the following
			lua_settable(L, newtab_index);
		}

		lua_newtable(L);
		luaL_setfuncs(L, class_meta, 0);

		lua_pushvalue(L, newtab_index);
		lua_setfield(L, newtab_index, "__index");

		lua_setmetatable(L, newtab_index);

		// Set parent
		lua_pushvalue(L, 1); // Copy table 1
		lua_setfield(L, newtab_index, "parent");
	}

	return 1;
}

/*	class operator (...)
	Returns:	table [class]

	This creates an instance of the given class, sets parent,
	and calls its constructor function with the given parameters
*/
int Class_lua::__call(lua_State *L)
{
	int top = lua_gettop(L);
	int ctor_args = top - 1;
	checkType(L, LT_TABLE, 1);

	// Create a new table
	lua_newtable(L);
	int newtab_index = lua_gettop(L);

	// Copy methods & variables
	lua_pushnil(L);
	while( lua_next(L, 1) )
	{
		lua_pushvalue(L, -2); // Make a copy of key for next iteration
		lua_insert(L, -2); // Move our copy down the stack so it remains after the following
		lua_settable(L, newtab_index);
	}

	// Copy metatable
	lua_getmetatable(L, 1);
	lua_setmetatable(L, -2);

	// Set parent
	lua_pushvalue(L, 1); // Copy table 1
	lua_setfield(L, newtab_index, "parent");

	// Move newtab to index 2; NOTE: newtab_index is now invalid!
	lua_insert(L, 2);


	// If it has a constructor... run it.
	lua_getfield(L, 2, "constructor");
	if( lua_isfunction(L, -1) )
	{
		lua_insert(L, 3); // Move to index 3

		// Create a copy of our index (self), move it to 4
		lua_pushvalue(L, 2);
		lua_insert(L, 4);

		// And run it!
		if( lua_pcall(L, ctor_args+1, 0, 0) != LUA_OK )
		{ // Uh oh... something bad happened. Report it.
			luaL_error(L, lua_tostring(L, -1));
		}
	}
	else
		lua_pop(L, 1); // Pop it if we can't run it

	//lua_pop(L, lua_gettop(L)-2); // For debugging; remove all but the bottom 2

	return 1;
}


/*	class __tostring() metamethod
	Returns:	string (Class: 0x%X)

	This is the default "tostring" metamethod for classes
	that do not overload it in their metatables.
	Nothing fancy here.
*/
int Class_lua::__tostring(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	char buffer[32];

	slprintf(buffer, sizeof(buffer)-1, "Class: 0x%X", lua_topointer(L, 1));
	lua_pushstring(L, buffer);
	return 1;
}


/*	class.vector3d([number x, number y, number z])
	Returns:	table (class)

	Create a new table (class) of vector3d.
	If x and y are given, the new vector3d retains
	the given values.

	The vector3d class contains metamethods for
	operations such as vector scaling and dot product.
*/
int Class_lua::vector3d(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 0 && top !=2 && top != 3 )
		wrongArgs(L);

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
