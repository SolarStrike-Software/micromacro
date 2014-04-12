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

int Log_lua::getFilename(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);
	lua_pushstring(L, Logger::instance()->get_filename().c_str());
	return 1;
}

int Log_lua::add(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->add(str);
	return 0;
}

int Log_lua::addRaw(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->add_raw(str);
	return 0;
}

int Log_lua::isOpen(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);
	lua_pushboolean(L, Logger::instance()->is_open());
	return 1;
}
