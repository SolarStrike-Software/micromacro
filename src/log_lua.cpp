/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "log_lua.h"
#include "error.h"
#include "logger.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}


int Log_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"getFilename", Log_lua::getFilename},
		{"add", Log_lua::add},
		{"addRaw", Log_lua::addRaw},
		{"isOpen", Log_lua::isOpen},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, LOG_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

/*	log.getFilename()
	Returns:	string

	Returns the filename of the file we are logging to.
*/
int Log_lua::getFilename(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);
	lua_pushstring(L, Logger::instance()->get_filename().c_str());
	return 1;
}

/*	log.add(string msg)
	Returns:	nil

	Adds 'msg' into our log file. This includes timestamp prefix,
	but not newline.
*/
int Log_lua::add(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->add("%s", str);
	return 0;
}

/*	log.addRaw(string msg)
	Returns:	nil

	Adds 'msg' into our log file. This differs from log.add()
	as it does not include the timestamp.
*/
int Log_lua::addRaw(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->add_raw(str);
	return 0;
}

/*	log.isOpen()
	Returns:	boolean

	If a log was successfully opened for writing, returns true.
	Else, returns false.
*/
int Log_lua::isOpen(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);
	lua_pushboolean(L, Logger::instance()->is_open());
	return 1;
}
