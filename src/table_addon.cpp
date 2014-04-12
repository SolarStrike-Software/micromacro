/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "table_addon.h"
#include "error.h"
#include "strl.h"

#include <string.h>
#include <string>
#include <cmath>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}


int Table_addon::regmod(lua_State *L)
{
	lua_getglobal(L, TABLE_MODULE_NAME);

	lua_pushcfunction(L, Table_addon::copy);
	lua_setfield(L, -2, "copy");

	lua_pushcfunction(L, Table_addon::find);
	lua_setfield(L, -2, "find");

	lua_pushcfunction(L, Table_addon::print);
	lua_setfield(L, -2, "print");

	lua_pop(L, 1); // Pop module off stack

	return MicroMacro::ERR_OK;
}

/*	table.copy(table orig)
	Returns:	table

	Actually does a full copy of a table, instead of referencing the original.
	This also recursively copies sub-tables.
*/
int Table_addon::copy(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);

	lua_newtable(L);
	int newtab_index = lua_gettop(L);

	// Copy methods & variables
	lua_pushnil(L);
	while( lua_next(L, 1) )
	{
		lua_pushvalue(L, -2); // Make a copy of key for next iteration
		lua_insert(L, -2); // Move our copy down the stack so it remains after the following

		if( lua_istable(L, -1) )
		{ // We need to copy this sub-table
			copy(L);
		}
		else
			lua_settable(L, newtab_index); // We can just copy it directly
	}

	// Copy metatable
	lua_getmetatable(L, 1);
	lua_setmetatable(L, -2);

	return 1;
}

/*	table.find(table haystack, value)
	Returns (on success):	key
	Returns (on failure):	nil

	Checks table 'haystack' for anything that matches 'value'.
	If found, returns the table's key that contains the value.
	If no match is found, returns nil.
*/
int Table_addon::find(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);

	checkType(L, LT_TABLE, 1);
	// Index 2 can be anything

	int valtype = lua_type(L, 2);

	lua_pushnil(L);
	while( lua_next(L, 1) )
	{
		bool found = false;
		if( lua_type(L, -1) == valtype )
		{ // Types match, compare value
			switch( valtype )
			{
				case LUA_TNUMBER:
				{
					double usernum = lua_tonumber(L, 2);
					double tabnum = lua_tonumber(L, -1);
					if( fabs(usernum-tabnum) < DOUBLE_EPSILON ) // Compare values, make sure within EPSILON
						found = true;
				}
				break;
				case LUA_TSTRING:
				{
					const char *userstr = lua_tostring(L, 2);
					const char *tabstr = lua_tostring(L, -1);
					if( strcmp(userstr, tabstr) == 0 )
						found = true;
				}
				case LUA_TNIL:
				{ // NOTE: This *probably* should never happen, as nil values should be cleaned by GC.
					// However, it is better safe than sorry.
					found = true;
				}
				break;
				default: // Not a number or string, just check pointer
				{
					int *userptr = (int *)lua_topointer(L, 2);
					int *tabptr = (int *)lua_topointer(L, -1);

					if( *userptr == *tabptr )
						found = true;
				}
				break;
			}

			if( found )
			{
				lua_pop(L, 1); // Pop value, return key
				return 1;
			}
		}

		lua_pop(L, 1); // Pop value, keep key for next iteration
	}

	return 0;
}

/*	table.print(table tab [, number depth])
	Returns:	nil

	Recursively dump the table to the standard output.
	When called from Lua, you probably shouldn't include the depth...
*/
int Table_addon::print(lua_State *L)
{
	int depth = 0;
	if( lua_gettop(L) >= 2 && lua_isnumber(L, -1) )
	{
		depth = lua_tointeger(L, -1);
		lua_pop(L, 1); // We can pop depth; don't need it
	}
	else
		depth = 0;

	int tabIndex = lua_gettop(L);
	checkType(L, LT_TABLE, tabIndex);

	// Now iterate this table
	lua_pushnil(L);
	while( lua_next(L, tabIndex) )
	{
		// Print descriptor
		std::string depthStr = std::string(depth, '\t');

		std::string key;
		if( lua_isnumber(L, -2) )
		{
			// We need to convert this to a string, but DO NOT CALL lua_tostring()!
			// Doing so will modify the key on stack and confuse lua_next()
			int index = lua_tointeger(L, -2);
			char buffer[32];
			slprintf(buffer, sizeof(buffer)-1, "%d", index);
			key = buffer;
		}
		else
			key = lua_tostring(L, -2);

		if( lua_istable(L, -1) && depth < TABLE_PRINT_MAXDEPTH )
		{
			printf("%s%s:\ttable: 0x%X\n", depthStr.c_str(), key.c_str(), (unsigned int)lua_topointer(L, -1));
			// Recurse
			lua_pushinteger(L, depth+1); // Push depth
			print(L);
		}
		else
			printf("%s%s:\t%s\n", depthStr.c_str(), key.c_str(), lua_tostring(L, -1));

		// Pop value, keep key
		lua_pop(L, 1);
	}


	return 0;
}
