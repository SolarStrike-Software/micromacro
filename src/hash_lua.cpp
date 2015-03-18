/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "hash_lua.h"
#include "sha1.h"
#include "error.h"
#include "macro.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

int Hash_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"sha1", Hash_lua::sha1},
		{"sha1_file", Hash_lua::sha1_file},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, HASH_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

/*	sha1(string str)
	Returns:	string

	Returns the SHA1 hash of a string.
*/
int Hash_lua::sha1(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

    size_t strlen;
    const char *str = lua_tolstring(L, 1, &strlen);
    std::string szStr = std::string(str, strlen);

    lua_pushstring(L, ::sha1(szStr).c_str());
    return 1;
}

/*	sha1_file(string filename)
	Returns:	string

	Returns the SHA1 hash of a file.
*/
int Hash_lua::sha1_file(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

    size_t filenamelen;
    const char *filename = lua_tolstring(L, 1, &filenamelen);

    lua_pushstring(L, ::SHA1::from_file(filename).c_str());
    return 1;
}
