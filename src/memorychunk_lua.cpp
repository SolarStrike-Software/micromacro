/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "memorychunk_lua.h"
#include "error.h"
#include "strl.h"
#include "types.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

const char *LuaType::metatable_memorychunk = "memorychunk";

int MemoryChunk_lua::regmod(lua_State *L)
{
	const luaL_Reg meta[] = {
		{"__gc", gc},
		{"__tostring", tostring},
		{NULL, NULL}
	};

	const luaL_Reg methods[] = {
		{"getSize", getSize},
		{"getAddress", getAddress},
		{"getData", getData},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LuaType::metatable_memorychunk);
	luaL_setfuncs(L, meta, 0);
	luaL_newlib(L, methods);
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1); // Pop table

	return MicroMacro::ERR_OK;
}

int MemoryChunk_lua::gc(lua_State *L)
{
	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	delete []pChunk->data;
	pChunk->data = NULL;
	return 0;
}

int MemoryChunk_lua::tostring(lua_State *L)
{
	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	char buffer[64];
	slprintf(buffer, sizeof(buffer), "Memory chunk (0x%X - 0x%X)",
		pChunk->address, pChunk->address + pChunk->size);

	lua_pushstring(L, buffer);
	return 1;
}

/*	memorychunk:getSize()
	Returns:	number

    Returns the size, in bytes, of a memory chunk.
*/
int MemoryChunk_lua::getSize(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	lua_pushinteger(L, pChunk->size);

	return 1;
}

/*	memorychunk:getAddress()
	Returns:	number

    Returns the address that a memory chunk has started reading from.
*/
int MemoryChunk_lua::getAddress(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	lua_pushinteger(L, pChunk->address);

	return 1;
}

/*	memorychunk:getAddress()
	Returns:	number

    Extracts an actual, usable piece of data, such as a number or string, from a memory chunk.

    'type' should be a string that represents the type of data to read ("byte", "ubyte", "int", "uint", "float", "string", etc.)
    'offset' should be the number of bytes after the start address to read data from.
*/
int MemoryChunk_lua::getData(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 3 && top != 4 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_STRING, 2);
	checkType(L, LT_NUMBER, 3);

	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	std::string type = lua_tostring(L, 2);
	size_t offset = lua_tointeger(L, 3);
	int err = 0;

	if( type == "byte" ) {
		char data = getChunkVariable<char>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "ubyte" ) {
		unsigned char data = getChunkVariable<unsigned char>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "short" ) {
		short data = getChunkVariable<short>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "ushort" ) {
		unsigned short data = getChunkVariable<unsigned short>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "int" ) {
		int data = getChunkVariable<int>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "uint" ) {
		unsigned int data = getChunkVariable<unsigned int>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "int64" ) {
		long long data = getChunkVariable<long long>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "uint64" ) {
		unsigned long long data = getChunkVariable<unsigned long long>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "float" ) {
		float data = getChunkVariable<float>(pChunk, offset, err);
		lua_pushnumber(L, data);
	} else if( type == "double" ) {
		double data = getChunkVariable<double>(pChunk, offset, err);
		lua_pushnumber(L, data);
	} else if( type == "string" ) {
		checkType(L, LT_NUMBER, 4);
		unsigned int length = lua_tointeger(L, 4);
		std::string data = getChunkString(pChunk, offset, length, err);
		lua_pushstring(L, data.c_str());
	} else
	{
		luaL_error(L, "Invalid type given as first parameter.");
		return 0;
	}

	if( err )
	{ // Throw error
		lua_pop(L, lua_gettop(L) - top); // Remove anything we might've pushed
		pushLuaErrorEvent(L, "Attempt to get data that is out of bounds.");
		return 0;
	}

	return 1;
}
