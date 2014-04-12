/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "global_addon.h"
#include "error.h"
#include "wininclude.h"
#include "filesystem.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

lua_CFunction Global_addon::sprintf = 0;
std::vector<std::string> Global_addon::includedList;

int Global_addon::regmod(lua_State *L)
{
	// Create a shortcut to string.format as sprintf
	lua_getglobal(L, "string");
	lua_getfield(L, -1, "format");
	sprintf = lua_tocfunction(L, -1);
	lua_setglobal(L, "sprintf");
	lua_pop(L, 1); // Pop string module

	// Other functions
	lua_pushcfunction(L, Global_addon::printf);
	lua_setglobal(L, "printf");

	lua_pushcfunction(L, Global_addon::unpack2);
	lua_setglobal(L, "unpack2");

	lua_pushcfunction(L, Global_addon::include);
	lua_setglobal(L, "include");

	// Clear our included list (in case it is dirty from last time)
	includedList.clear();

	return MicroMacro::ERR_OK;
}

int Global_addon::printf(lua_State *L)
{
	//int top = lua_gettop(L);
	sprintf(L); // Format it

	if( lua_isstring(L, -1) ) // If we can, print it!
		::printf(lua_tostring(L, -1));
	return 0;
}

// Takes vararg and retuns them as a table
int Global_addon::unpack2(lua_State *L)
{
	int top = lua_gettop(L);
	if( top == 0 )
		return 0;

	lua_newtable(L);
	for(int i = 1; i <= top; i++)
	{
		lua_pushnumber(L, i); // Key
		lua_pushvalue(L, i); // Value
		lua_settable(L, -3); // Set
	}

	return 1;
}

int Global_addon::include(lua_State *L)
{
	bool forceRun = false;
	char *filename;
	int top = lua_gettop(L);

	if( top != 1 && top != 2 )
		wrongArgs(L);

	checkType(L, LT_STRING, 1);
	filename = (char *)lua_tostring(L, 1);

	if( top == 2 )
	{
		checkType(L, LT_BOOLEAN, 2);
		forceRun = lua_toboolean(L, 2);
	}

	// Get the full (non-relative) path and filename
	char *newFilenamePtr = NULL; // This will point to the filename in fullNewPath below
	char fullNewPath[MAX_PATH+1];
	GetFullPathName(filename, MAX_PATH, fullNewPath, &newFilenamePtr);

	// Make sure we even need to run it
	if( !forceRun )
	{
		for(unsigned int i = 0; i < includedList.size(); i++)
		{
			if( includedList.at(i) == fullNewPath )
				return 0; // Already included, skip it
		}
	}

	// Copy current CWD
	char cwdBuffer[MAX_PATH+1];
	GetCurrentDirectory(MAX_PATH,(LPTSTR)&cwdBuffer);

	// Set temporary CWD
	SetCurrentDirectory(::getFilePath(fullNewPath, false).c_str());

	// And run it!
	if( luaL_dofile(L, newFilenamePtr) != LUA_OK )
	{
		luaL_error(L, lua_tostring(L, -1));
	}

	// Reset original CWD
	SetCurrentDirectory((LPCTSTR)&cwdBuffer);

	// Insert it into our included list
	includedList.push_back(fullNewPath);

	// Return count is stack size - original
	return lua_gettop(L) - top;
}
