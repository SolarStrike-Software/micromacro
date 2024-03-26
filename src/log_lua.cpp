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

typedef struct LOG_LEVEL_NAME_PAIR
{
    const char *name;
    int level;
} LOG_LEVEL_NAME_PAIR;


int Log_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"getFilename", Log_lua::getFilename},
		{"add", Log_lua::add},
		{"addRaw", Log_lua::addRaw},
		{"isOpen", Log_lua::isOpen},
		{"setLevel", Log_lua::setLevel},
		{"emergency", Log_lua::emergency},
		{"alert", Log_lua::alert},
		{"critical", Log_lua::critical},
		{"error", Log_lua::error},
		{"warning", Log_lua::warning},
		{"notice", Log_lua::notice},
		{"info", Log_lua::info},
		{"debug", Log_lua::debug},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);

	//
    static const LOG_LEVEL_NAME_PAIR _levels[] = {
	    {"emergency", LogLevel::emergency},
	    {"alert", LogLevel::alert},
	    {"critical", LogLevel::critical},
	    {"critical", LogLevel::critical},
	    {"error", LogLevel::error},
	    {"warning", LogLevel::warning},
	    {"notice", LogLevel::notice},
	    {"info", LogLevel::info},
	    {"debug", LogLevel::debug},
	};

    // push 'level' table
    lua_newtable(L);
    int newtab_index = lua_gettop(L);

    // Push our log levels as variables into the table
    int i = 0;
    while(_levels[i].name)
    {
        lua_pushinteger(L, _levels[i].level);
        lua_setfield(L, -2, _levels[i].name);
        i++;
    }
    lua_setfield(L, -2, "level"); // set the table, keyed/named as "level"

    // Set the log module onto the stack to finish off
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

/*	log.setLevel(int level)
	Returns:	nil
*/
int Log_lua::setLevel(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	LogLevel level = static_cast<LogLevel>(lua_tointeger(L, 1));
	Logger::instance()->setLevel(level);
	return 0;
}

/*	log.emergency(string msg)
	Returns:	nil
*/
int Log_lua::emergency(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->emergency(str);
	return 0;
}

/*	log.alert(string msg)
	Returns:	nil
*/
int Log_lua::alert(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->alert(str);
	return 0;
}

/*	log.critical(string msg)
	Returns:	nil
*/
int Log_lua::critical(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->critical(str);
	return 0;
}

/*	log.error(string msg)
	Returns:	nil
*/
int Log_lua::error(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->error(str);
	return 0;
}

/*	log.warning(string msg)
	Returns:	nil
*/
int Log_lua::warning(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->warning(str);
	return 0;
}

/*	log.notice(string msg)
	Returns:	nil
*/
int Log_lua::notice(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->notice(str);
	return 0;
}

/*	log.info(string msg)
	Returns:	nil
*/
int Log_lua::info(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->info(str);
	return 0;
}

/*	log.debug(string msg)
	Returns:	nil
*/
int Log_lua::debug(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	const char *str = lua_tostring(L, 1);
	Logger::instance()->debug(str);
	return 0;
}
