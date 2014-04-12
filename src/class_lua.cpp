#include "class_lua.h"
#include "error.h"
#include "strl.h"

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
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, CLASS_MODULE_NAME);

	return MicroMacro::ERR_OK;
}



int Class_lua::_new(lua_State *L)
{
	static const luaL_Reg class_meta[] = {
		{"__call", __call},
		{"__tostring", __tostring},
		{NULL, NULL}
	};

/*	static const luaL_Reg class_methods[] = {
		{NULL, NULL}
	};
*/
	int top = lua_gettop(L);
	if( top != 0 && top != 1 )
		wrongArgs(L);

	// Basic class setup
	lua_newtable(L);
	int newtab_index = lua_gettop(L);

	if( top == 0 ) // Create a base class
	{
		//luaL_newmetatable(L, "MicroMacro.class");
		lua_newtable(L);
		//int metaTableId = lua_gettop(L);
		luaL_setfuncs(L, class_meta, 0);

		// metatable.__index = <methods>
		//luaL_newlib(L, class_methods);
		lua_pushvalue(L, newtab_index);
		lua_setfield(L, newtab_index/*-2*/, "__index");

		// metatable.__metatable = <metatable>
		//luaL_newlib(L, class_meta);
		//lua_setfield(L, metaTableId, "__metatable");

		lua_setmetatable(L, newtab_index/*-2*/);

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
		lua_setfield(L, newtab_index/*-2*/, "__index");

		lua_setmetatable(L, newtab_index);

		// Set parent
		lua_pushvalue(L, 1); // Copy table 1
		lua_setfield(L, newtab_index, "parent");
	}

	return 1;
}

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

int Class_lua::__tostring(lua_State *L)
{
	checkType(L, LT_TABLE, 1);
	char buffer[32];

	slprintf(buffer, sizeof(buffer)-1, "Class: 0x%X", lua_topointer(L, 1));
	lua_pushstring(L, buffer);
	return 1;
}
