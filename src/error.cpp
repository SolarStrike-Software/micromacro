/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "error.h"
#include "strl.h"
#include "logger.h"
#include "macro.h"
#include "ncurses_lua.h"
#include <vector>
#include <string>
#include <string.h>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

const char *getErrorString(int errcode)
{
	switch(errcode)
	{
		case MicroMacro::ERR_OK:
			return "Everything is OK";
		break;
		case MicroMacro::ERR_CLOSE:
			return "Close request signal sent";
		break;
		case MicroMacro::ERR_DOUBLE_INIT:
			return "Double initialization not allowed";
		break;
		case MicroMacro::ERR_INIT_FAIL:
			return "Initialization failure";
		break;
		case MicroMacro::ERR_CLEANUP_FAIL:
			return "Failure to properly cleanup";
		break;
		case MicroMacro::ERR_NO_STATE:
			return "Attempt to use Lua state when no state has been created";
		break;
		case MicroMacro::ERR_RUN:
			return "Runtime error";
		break;
		case MicroMacro::ERR_MEM:
			return "Memory error";
		break;
		case MicroMacro::ERR_SYNTAX:
			return "Syntax error";
		break;
		case MicroMacro::ERR_FILE:
			return "File error";
		break;
		case MicroMacro::ERR_ERR:
			return "Error inside of an error: Errorception. I don\'t even know what\'s right anymore.";
		break;
		case MicroMacro::ERR_NOFUNCTION:
			return "Function does not exist or could not be found";
		break;

		case MicroMacro::ERR_UNKNOWN:
		default:
			return "Unknown or undefined error";
		break;
	}
}

std::string getWindowsErrorString(int errCode)
{
	char *tmp = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errCode, 0, (CHAR*)&tmp, 0, NULL);

	if( tmp == NULL )
		return "";

	std::string retval = tmp;
	LocalFree(tmp);

	retval.erase( retval.size()-2 ); // chop off trailing \r\n

	return retval;
}

void wrongArgsReal(lua_State *s, const char *name)
{
	luaL_error(s, "Wrong number of parameters supplied to %s().\n", name);
}

void badAllocationReal(const char *file, const char *func, int line)
{
	// Log the error
	char buffer[1024];
	#ifdef DISPLAY_DEBUG_MESSAGES
		sprintf(buffer, "Bad allocation, %s:%d in function %s\n", file, line, func);
	#else
		sprintf(buffer, "Bad allocation in function %s, line %d\n", func, line);
	#endif
	fprintf(stderr, buffer);
	Logger::instance()->add("%s", buffer);

	// Shut down Ncurses (if needed)
	if( Ncurses_lua::is_initialized() )
	{
		Ncurses_lua::cleanup(Macro::instance()->getEngine()->getLuaState());
	}

	// Let the user know something bad happened, give them time to see the error
	printf("\a\a\a"); // Ding!
	system("pause");

	// Time to go home
	exit(MicroMacro::ERR_MEM);
}


int checkType(lua_State *L, int acceptableTypes, int arg)
{
	int ok = false;
	std::vector<short> acceptableTypeIDs;
	acceptableTypeIDs.reserve(10);

	// Check each type
	if( acceptableTypes & LT_NIL )
	{
		acceptableTypeIDs.push_back(LUA_TNIL);
		if( lua_isnil(L, arg) )
			ok = true;
	}

	if( acceptableTypes & LT_NUMBER )
	{
		acceptableTypeIDs.push_back(LUA_TNUMBER);
		if( lua_isnumber(L, arg) )
			ok = true;
	}

	if( acceptableTypes & LT_STRING )
	{
		acceptableTypeIDs.push_back(LUA_TSTRING);
		if( lua_isstring(L, arg) )
			ok = true;
	}

	if( acceptableTypes & LT_BOOLEAN )
	{
		acceptableTypeIDs.push_back(LUA_TBOOLEAN);
		if( lua_isboolean(L, arg) )
			ok = true;
	}

	if( acceptableTypes & LT_TABLE )
	{
		acceptableTypeIDs.push_back(LUA_TTABLE);
		if( lua_istable(L, arg) )
			ok = true;
	}

	if( acceptableTypes & LT_FUNCTION )
	{
		acceptableTypeIDs.push_back(LUA_TFUNCTION);
		if( lua_isfunction(L, arg) )
			ok = true;
	}

	if( acceptableTypes & LT_THREAD )
	{
		acceptableTypeIDs.push_back(LUA_TTHREAD);
		if( lua_isthread(L, arg) )
			ok = true;
	}

	if( acceptableTypes & LT_USERDATA )
	{
		acceptableTypeIDs.push_back(LUA_TUSERDATA);
		if( lua_isuserdata(L, arg) )
			ok = true;
	}


	if( !ok )
	{
		char expected[256];
		strlcpy((char*)&expected, "unknown", 8);
		unsigned int buff_left = sizeof(expected)-1; // What's left to use (below)

		for(unsigned int i = 0; i < acceptableTypeIDs.size(); i++)
		{
			if( i == 0 )
			{
				unsigned int buff_used = 0;
				buff_used = strlcpy((char*)&expected, lua_typename(L, acceptableTypeIDs.at(i)), buff_left);
				buff_left = sizeof(expected) - 1 - buff_used;
			}
			else if( i == acceptableTypeIDs.size() - 1 )
			{
				const char *or_txt = " or ";
				strlcat((char*)&expected, or_txt, buff_left);
				buff_left -= strlen(or_txt);
				strlcat((char*)&expected, lua_typename(L, acceptableTypeIDs.at(i)), buff_left);
				buff_left = sizeof(expected) - 1 - strlen((char*)&expected);
			}
			else
			{
				const char *comma_txt = ", ";
				strlcat((char*)&expected, comma_txt, buff_left);
				buff_left -= strlen(comma_txt);
				strlcat((char*)&expected, lua_typename(L, acceptableTypeIDs.at(i)), buff_left);
				buff_left -= sizeof(expected) - 1 - strlen((char*)&expected);
			}
		}

		luaL_typerror(L, arg, expected);
	}

	return 0;
}

int luaL_typerror(lua_State *L, int narg, const char *tname)
{
	const char *msg = lua_pushfstring(L, "%s expected, got %s", tname,
		luaL_typename(L, narg));

	return luaL_argerror(L, narg, msg);
}

void pushLuaErrorEvent(lua_State *L, const char *fmt, ...)
{
	// Get Lua state info
	lua_Debug ar;
	lua_getstack(L, 1, &ar);
	lua_getinfo(L, "nSl", &ar);

	// Prep a string that tells us where the error originated
	char scriptinfo[256];
	slprintf(scriptinfo, sizeof(scriptinfo)-1, " %s:%d", ar.short_src, ar.currentline);

	// Actually format the error we were given
	char buffer[2048];
	va_list va_alist;
	va_start(va_alist, fmt);
	_vsnprintf(buffer, sizeof(buffer), fmt, va_alist);
	va_end(va_alist);

	// Queue it
	MicroMacro::Event e;
	e.type = MicroMacro::EVENT_ERROR;
	e.msg = buffer;
	e.msg += scriptinfo;
	//Macro::instance()->getEventQueue()->push(e);
	Macro::instance()->pushEvent(e);
}
