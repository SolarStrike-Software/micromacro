/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include <ncursesw/ncurses.h>
#include "luatypes.h"
#include "types.h"
#include "error.h"
#include "strl.h"
#include "wininclude.h"
#include "event.h"
#include "macro.h"

#include <math.h>
#include <cmath>

/*extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}*/

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

namespace LuaType
{
	// Metatable names
	const char *metatable_int64 = "int64";
	const char *metatable_ncursesWindow = "ncurses.window";
	const char *metatable_handle = "process.handle";
	const char *metatable_windowDC = "window.windowDC";
	const char *metatable_audioResource = "audio.audioResource";
	//const char *metatable_vector2d = "vector2d";
	const char *metatable_vector3d = "vector3d";
	const char *metatable_memorychunk = "memorychunk";
}

int registerLuaTypes(lua_State *L)
{
	// Ncurses windows
	luaL_newmetatable(L, LuaType::metatable_ncursesWindow);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, LuaType::ncursesWindow_gc);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::ncursesWindow_tostring);
	lua_settable(L, -3);
	lua_pop(L, 1); // Pop metatable


	// Handles
	luaL_newmetatable(L, LuaType::metatable_handle);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, LuaType::handle_gc);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::handle_tostring);
	lua_settable(L, -3);
	lua_pop(L, 1); // Pop metatable


	// Audio
	luaL_newmetatable(L, LuaType::metatable_audioResource);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, LuaType::audioResource_gc);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::audioResource_tostring);
	lua_settable(L, -3);
	lua_pop(L, 1); // Pop metatable


	// MemoryChunk
	luaL_newmetatable(L, LuaType::metatable_memorychunk);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, LuaType::memorychunk_gc);
	lua_settable(L, -3);

	lua_pushstring(L, "__tostring");
	lua_pushcfunction(L, LuaType::memorychunk_tostring);
	lua_settable(L, -3);

	luaL_newlib(L, LuaType::memorychunk_methods);
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1); // Pop metatable

	return MicroMacro::ERR_OK;
}

int LuaType::ncursesWindow_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WINDOW **pw = (WINDOW **)lua_touserdata(L, 1);

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Ncurses Window: 0x%X", *pw);
	lua_pushstring(L, buffer);
	return 1;
}

int LuaType::ncursesWindow_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WINDOW **pw = (WINDOW **)lua_touserdata(L, 1);

	/* We really don't want to delete stdscr! */
	if( *pw != ::stdscr )
		delwin(*pw);

	*pw = NULL;
	return 0;
}

int LuaType::handle_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	HANDLE *pHandle = (HANDLE *)lua_touserdata(L, 1);

	CloseHandle(*pHandle);
	*pHandle = NULL;
	return 0;
}

int LuaType::handle_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	HANDLE *pHandle = (HANDLE *)lua_touserdata(L, 1);

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Process handle: 0x%X", *pHandle);
	lua_pushstring(L, buffer);
	return 1;
}
/*
int LuaType::windowDC_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WindowDCPair *pWinDC = static_cast<WindowDCPair *>(lua_touserdata(L, 1));

	ReleaseDC(pWinDC->hwnd, pWinDC->hdc);
	pWinDC->hwnd = NULL; pWinDC->hdc = NULL;
	return 0;
}

int LuaType::windowDC_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	WindowDCPair *pWinDC = static_cast<WindowDCPair *>(lua_touserdata(L, 1));

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Window device context 0x%X", *pWinDC);
	lua_pushstring(L, buffer);
	return 1;
}
*/

// Flush the sound data
int LuaType::audioResource_gc(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	if( pResource->buffer != AL_NONE )
		alDeleteBuffers(1, &pResource->buffer);
	if( pResource->source != AL_NONE )
		alDeleteSources(1, &pResource->source);

	pResource->buffer = AL_NONE;
	pResource->source = AL_NONE;
	return 0;
}

int LuaType::audioResource_tostring(lua_State *L)
{
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	char buffer[128];
	slprintf(buffer, sizeof(buffer)-1, "Audio resource 0x%X", pResource);
	lua_pushstring(L, buffer);
	return 1;
}

int LuaType::memorychunk_gc(lua_State *L)
{
	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	delete []pChunk->data;
	pChunk->data = NULL;
	return 0;
}

int LuaType::memorychunk_tostring(lua_State *L)
{
	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	char buffer[64];
	slprintf(buffer, sizeof(buffer), "Memory chunk (0x%X - 0x%X)",
		pChunk->address, pChunk->address + pChunk->size);

	lua_pushstring(L, buffer);
	return 1;
}

int LuaType::memorychunk_getSize(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	lua_pushinteger(L, pChunk->size);

	return 1;
}

int LuaType::memorychunk_getAddress(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	lua_pushinteger(L, pChunk->address);

	return 1;
}

int LuaType::memorychunk_getData(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 3 && top != 4 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_STRING, 2);
	checkType(L, LT_NUMBER, 3);

	MemoryChunk *pChunk = static_cast<MemoryChunk *>(lua_touserdata(L, 1));
	std::string type = lua_tostring(L, 2);
	unsigned int offset = lua_tointeger(L, 3);
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
	} else if( type == "float" ) {
		float data = getChunkVariable<float>(pChunk, offset, err);
		lua_pushinteger(L, data);
	} else if( type == "double" ) {
		double data = getChunkVariable<double>(pChunk, offset, err);
		lua_pushinteger(L, data);
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

Vector3d lua_tovector3d(lua_State *L, int index)
{
	Vector3d vec;
	lua_getfield(L, index, "x");
	vec.x = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "y");
	vec.y = lua_tonumber(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "z");
	vec.z = lua_tonumber(L, -1);
	lua_pop(L, 1);

	return vec;
}

void lua_pushvector3d(lua_State *L, Vector3d &vec)
{
	lua_newtable(L);

	luaL_getmetatable(L, LuaType::metatable_vector3d);
	lua_setmetatable(L, -2);

	lua_pushnumber(L, vec.x);
	lua_setfield(L, -2, "x");

	lua_pushnumber(L, vec.y);
	lua_setfield(L, -2, "y");

	lua_pushnumber(L, vec.z);
	lua_setfield(L, -2, "z");
}
