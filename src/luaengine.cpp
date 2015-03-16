/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "luaengine.h"
#include "luatypes.h"
#include "macro.h"
#include "logger.h"

#include "ncurses_lua.h"
#include "time_lua.h"
#include "keyboard_lua.h"
#include "mouse_lua.h"
#include "key_lua.h"
#include "gamepad_lua.h"
#include "system_lua.h"
#include "filesystem_lua.h"
#include "process_lua.h"
#include "window_lua.h"
#include "class_lua.h"
#include "log_lua.h"
#include "hash_lua.h"
#include "cli_lua.h"
#include "memorychunk_lua.h"

#ifdef NETWORKING_ENABLED
	#include "network_lua.h"
	#include "socket_lua.h"
#endif

#ifdef AUDIO_ENABLED
	#include "audio_lua.h"
#endif

#include "global_addon.h"
#include "string_addon.h"
#include "math_addon.h"
#include "table_addon.h"

#include "vector3d_lua.h"

#include "strl.h"
#include "event.h"
#include "version.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}


LuaEngine::~LuaEngine()
{
	if( lstate )
	{
		lua_close(lstate);
		lstate = NULL;
	}
}

/*	Call this after something bad happened, but before popping
	anything off the Lua stack.
	It will store the formatted error message into lastErrorMsg
	and pop the error message from the stack.
*/
void LuaEngine::stdError()
{
	lastErrorMsg = lua_tostring(lstate, lua_gettop(lstate));
	lua_pop(lstate, 1);
}

// Placeholder; this gets replaced by user script
int LuaEngine::_macrotab_init(lua_State *)
{
	return 0;
}

// Placeholder; this gets replaced by user script
int LuaEngine::_macrotab_main(lua_State *L)
{
	lua_pushboolean(L, false);
	return 1;
}

// Placeholder; this gets replaced by user script
int LuaEngine::_macrotab_event(lua_State *)
{
	return 0;
}

// Returns the version as a MAJOR.MINOR.BUILD string format (ie 2.1.4)
int LuaEngine::_macrotab_getVersion(lua_State *L)
{
	char buffer[32];
	slprintf(buffer, sizeof(buffer)-1, "%d.%d.%d", AutoVersion::MAJOR, AutoVersion::MINOR, AutoVersion::BUILD);
	lua_pushstring(L, buffer);
	return 1;
}

/*	Push this into the stack and use it as the message handler
	for lua_pcall(). This appends the stacktrace to any error
	messages we might catch.
*/
int LuaEngine::err_msgh(lua_State *L)
{
	const char *origstr = lua_tostring(L, 1);
	lua_pop(L, 1); // Not entirely sure if we need to pop the "old" error

	luaL_traceback(L, L, origstr, 1);
	return 1;
}

/*	macro.is64bit()
	Returns:	boolean

	Returns whether or not the application is 64-bit
*/
int LuaEngine::is64bit(lua_State *L)
{
	#ifdef _WIN64
		lua_pushboolean(L, true);
	#else
		lua_pushboolean(L, false);
	#endif
	return 1;
}


/*	macro.is32bit()
	Returns:	boolean

	Returns whether or not the application is 32-bit
*/
int LuaEngine::is32bit(lua_State *L)
{
	#ifdef _WIN64
		lua_pushboolean(L, false);
	#else
		lua_pushboolean(L, true);
	#endif
	return 1;
}

// Basic initialization stuff
int LuaEngine::init()
{
	if( lstate )
		return MicroMacro::ERR_DOUBLE_INIT;

	// Create the Lua state
	lstate = luaL_newstate();
	if( !lstate )
		return MicroMacro::ERR_INIT_FAIL;

	// Load Lua libraries
	luaL_openlibs(lstate);

	// Load our own library
	static const luaL_Reg _macrotab[] = {
		{MACRO_INIT_NAME, LuaEngine::_macrotab_init},
		{MACRO_MAIN_NAME, LuaEngine::_macrotab_main},
		{MACRO_EVENT_NAME, LuaEngine::_macrotab_event},
		{"getVersion", LuaEngine::_macrotab_getVersion},
		{"is64bit", LuaEngine::is64bit},
		{"is32bit", LuaEngine::is32bit},
		{NULL, NULL}
	};

	luaL_newlib(lstate, _macrotab);
	lua_setglobal(lstate, MACRO_TABLE_NAME);

	/* Register types (metatables) */
	registerLuaTypes(lstate);

	/* Register modules & addons */
	const lua_CFunction regModFuncs[] = {
		/* Modules */
		Ncurses_lua::regmod,
		Time_lua::regmod,
		Keyboard_lua::regmod,
		Mouse_lua::regmod,
		Key_lua::regmod,
		Gamepad_lua::regmod,
		System_lua::regmod,
		Filesystem_lua::regmod,
		Process_lua::regmod,
		Window_lua::regmod,
		MemoryChunk_lua::regmod,	// Is this needed?
#ifdef NETWORKING_ENABLED
		Network_lua::regmod,
		Socket_lua::regmod,
#endif

		Class_lua::regmod,
		Log_lua::regmod,
		Hash_lua::regmod,
		Cli_lua::regmod,
		/* Addons */
		Global_addon::regmod,
		String_addon::regmod,
		Math_addon::regmod,
		Table_addon::regmod,
		/* Any other classes */
		Vector3d_lua::regmod,
		0 // NULL terminator
	};

	unsigned int i = 0;
	while( regModFuncs[i] != 0 )
	{
		int regSuccess = regModFuncs[i](lstate);
		if( regSuccess != MicroMacro::ERR_OK )
		{ // Error occurred while loading module
			const char *err = "One or more modules failed to load.\n";
			fprintf(stderr, err);
			Logger::instance()->add(err);
			return regSuccess;
		}
		++i; // Next module
	}

	#ifdef AUDIO_ENABLED
	{
		int regSuccess = Audio_lua::regmod(lstate);
		if( regSuccess != MicroMacro::ERR_OK )
		{ // Error occurred while loading module
			const char *err = "Failed to load audio module; disabling sound and moving on.\n";
			fprintf(stderr, err);
			Logger::instance()->add(err);
			Macro::instance()->getSettings()->setInt(CONFVAR_AUDIO_ENABLED, 0);
		}
	}
	#endif

	/* Set path */
	lua_getglobal(lstate, "package");
	lua_getfield(lstate, -1, "path");
	std::string curPath = lua_tostring(lstate, -1);		// Grab path
	lua_pop(lstate, 1);									// Remove the old
	curPath.append(";");								// Add the new
	curPath.append(basePath);
	curPath.append("/lib/?;");
	curPath.append(basePath);
	curPath.append("/lib/?.lua");
	lua_pushstring(lstate, curPath.c_str());
	lua_setfield(lstate, -2, "path");
	lua_pop(lstate, 1);									// Reset stack

	lastTimestamp.QuadPart = 0;
	fDeltaTime = 0.0;

	return MicroMacro::ERR_OK;
}

// Destroy and rebuild
int LuaEngine::reinit()
{
	int success;
	success = cleanup();
	if( success != MicroMacro::ERR_OK )
		return success;

	success = init();
	return success;
}

// Trash the Lua state; you don't need to call this before reinit()
int LuaEngine::cleanup()
{
	if( !lstate )
		return MicroMacro::ERR_CLEANUP_FAIL;

	#ifdef AUDIO_ENABLED
	Audio_lua::cleanup(lstate);
	#endif

	if( Ncurses_lua::is_initialized() )
		Ncurses_lua::cleanup(lstate);

	#ifdef NETWORKING_ENABLED
	Network_lua::cleanup();
	#endif

	Process_lua::cleanup(lstate);

	lua_close(lstate);
	lstate = NULL;

	lastErrorMsg = ""; // No point holding onto this anymore.
	return MicroMacro::ERR_OK;
}

// Same as loadFile, except we're giving it a string
int LuaEngine::loadString(const char *str)
{
	if( !lstate )
		return MicroMacro::ERR_NO_STATE;

	// Load it
	int failstate = luaL_loadstring(lstate, str);

	if( !failstate )
	{	// If no errors yet, try running the file.
		failstate = lua_pcall(lstate, 0, 0, 0);
	}

	if( failstate )
	{
		stdError();
		switch(failstate) {
			case LUA_ERRRUN:		return MicroMacro::ERR_RUN;		break;
			case LUA_ERRMEM:		return MicroMacro::ERR_MEM;		break;
			case LUA_ERRSYNTAX:		return MicroMacro::ERR_SYNTAX;	break;
			case LUA_ERRFILE:		return MicroMacro::ERR_FILE;	break;
			case LUA_ERRERR:		return MicroMacro::ERR_ERR;		break;
			default:				return MicroMacro::ERR_UNKNOWN;	break;
		}
	}

	return MicroMacro::ERR_OK;
}

// Essentially lua_dofile(), but with some extra stuff
int LuaEngine::loadFile(const char *filename)
{
	if( !lstate )
		return MicroMacro::ERR_NO_STATE;

	// Load it
	int failstate = luaL_loadfile(lstate, filename);

	if( !failstate )
	{	// If no errors yet, try running the file.
		failstate = lua_pcall(lstate, 0, 0, 0);
	}

	if( failstate )
	{
		stdError();
		switch(failstate) {
			case LUA_ERRRUN:		return MicroMacro::ERR_RUN;		break;
			case LUA_ERRMEM:		return MicroMacro::ERR_MEM;		break;
			case LUA_ERRSYNTAX:		return MicroMacro::ERR_SYNTAX;	break;
			case LUA_ERRFILE:		return MicroMacro::ERR_FILE;	break;
			case LUA_ERRERR:		return MicroMacro::ERR_ERR;		break;
			default:				return MicroMacro::ERR_UNKNOWN;	break;
		}
	}

	return MicroMacro::ERR_OK;
}

/*	Run the script's macro.init() function
	opt_args may be NULL if no args should be passed
*/
int LuaEngine::runInit(std::vector<std::string> *opt_args)
{
	// Push our message handler before arguments
	int stackbase = lua_gettop(lstate);
	lua_pushcfunction(lstate, LuaEngine::err_msgh);

	lua_getglobal(lstate, MACRO_TABLE_NAME);
	if( lua_type(lstate, -1) != LUA_TTABLE )
	{
		lua_pop(lstate, 1);
		return MicroMacro::ERR_NOFUNCTION;
	}

	lua_getfield(lstate, -1, MACRO_INIT_NAME);
	if( lua_type(lstate, -1) != LUA_TFUNCTION )
	{
		lua_pop(lstate, 2);
		return MicroMacro::ERR_NOFUNCTION;
	}

	// If we have been given some arguments, push those
	// as strings
	unsigned int nargs = 0;
	if( opt_args != NULL )
	{
		nargs = opt_args->size();
		for(unsigned int i = 0; i < nargs; i++)
		{
			lua_pushstring(lstate, opt_args->at(i).c_str());
		}
	}

	int failstate = lua_pcall(lstate, nargs, 0, stackbase+1);
	int retval = MicroMacro::ERR_OK;
	if( failstate )
	{
		stdError();
		switch(failstate) {
			case LUA_ERRRUN:		retval = MicroMacro::ERR_RUN;		break;
			case LUA_ERRMEM:		retval = MicroMacro::ERR_MEM;		break;
			case LUA_ERRSYNTAX:		retval = MicroMacro::ERR_SYNTAX;	break;
			case LUA_ERRFILE:		retval = MicroMacro::ERR_FILE;		break;
			case LUA_ERRERR:		retval = MicroMacro::ERR_ERR;		break;
			default:				retval = MicroMacro::ERR_UNKNOWN;	break;
		}
	}

	lua_pop(lstate, 2); // Pop traceback, macro table

	// Set our first timestamp so we can calculate deltaTime in main
	lastTimestamp = getNow();

	return retval;
}

/*	Run the script's macro.main() function
	Pushes delta time (in seconds) as first argument
	Expects one boolean in return: true(continue execution) or false(stop execution)
*/
int LuaEngine::runMain()
{
	// Calc delta time
	TimeType now = getNow();
	fDeltaTime = deltaTime(now, lastTimestamp);
	lastTimestamp = now;

	// Push our message handler before arguments
	int stackbase = lua_gettop(lstate);
	lua_pushcfunction(lstate, LuaEngine::err_msgh);

	// Push module
	lua_getglobal(lstate, MACRO_TABLE_NAME);
	if( lua_type(lstate, -1) != LUA_TTABLE )
	{
		lua_pop(lstate, 1);
		return MicroMacro::ERR_NOFUNCTION;
	}

	// Push function
	lua_getfield(lstate, -1, MACRO_MAIN_NAME);
	if( lua_type(lstate, -1) != LUA_TFUNCTION )
	{
		lua_pop(lstate, 2);
		return MicroMacro::ERR_NOFUNCTION;
	}

	lua_pushnumber(lstate, fDeltaTime);
	int failstate = lua_pcall(lstate, 1, 1, stackbase+1);
	int retval = MicroMacro::ERR_OK;
	if( failstate )
	{
		stdError();
		switch(failstate) {
			case LUA_ERRRUN:		retval = MicroMacro::ERR_RUN;		break;
			case LUA_ERRMEM:		retval = MicroMacro::ERR_MEM;		break;
			case LUA_ERRSYNTAX:		retval = MicroMacro::ERR_SYNTAX;	break;
			case LUA_ERRFILE:		retval = MicroMacro::ERR_FILE;		break;
			case LUA_ERRERR:		retval = MicroMacro::ERR_ERR;		break;
			default:				retval = MicroMacro::ERR_UNKNOWN;	break;
		}
	}

	// If available, get the return value (assume true)
	int continueMain = true;
	if( !lua_isnil(lstate, -1) )
		continueMain = (int)lua_toboolean(lstate, -1);

	lua_pop(lstate, 1); // Pop return value
	lua_pop(lstate, 2); // Pop stacktrace, macro table

	// Only change to ERR_CLOSE if we do NOT have an error
	// and we requested to close (return false from main)
	if( (retval == MicroMacro::ERR_OK) && (continueMain == false) )
		retval = MicroMacro::ERR_CLOSE; // Requested close

	return retval;
}

/*	Run the script's macro.event() function.
	Arguments passed depend on event type
*/
int LuaEngine::runEvent(Event &e)
{
	// Push our message handler before arguments
	int stackbase = lua_gettop(lstate);
	lua_pushcfunction(lstate, LuaEngine::err_msgh);

	lua_getglobal(lstate, MACRO_TABLE_NAME);
	if( lua_type(lstate, -1) != LUA_TTABLE )
	{
		lua_pop(lstate, 1);
		return MicroMacro::ERR_NOFUNCTION;
	}

	lua_getfield(lstate, -1, MACRO_EVENT_NAME);
	if( lua_type(lstate, -1) != LUA_TFUNCTION )
	{
		lua_pop(lstate, 2);
		return MicroMacro::ERR_NOFUNCTION;
	}

	int nargs = 0; // The number of arguments we're pushing onto the stack
	switch( e.type )
	{
		case EVENT_FOCUSCHANGED:
			lua_pushstring(lstate, "focuschanged");
			lua_pushinteger(lstate, e.idata1);
			nargs = 2;
		break;

		case EVENT_KEYPRESSED:
			lua_pushstring(lstate, "keypressed");
			lua_pushinteger(lstate, e.idata1);
			lua_pushboolean(lstate, e.idata2);
			nargs = 3;
		break;

		case EVENT_KEYRELEASED:
			lua_pushstring(lstate, "keyreleased");
			lua_pushinteger(lstate, e.idata1);
			lua_pushboolean(lstate, e.idata2);
			nargs = 3;
		break;

		case EVENT_MOUSEPRESSED:
			lua_pushstring(lstate, "mousepressed");
			lua_pushinteger(lstate, e.idata1);
			lua_pushboolean(lstate, e.idata2);
			nargs = 3;
		break;

		case EVENT_MOUSERELEASED:
			lua_pushstring(lstate, "mousereleased");
			lua_pushinteger(lstate, e.idata1);
			lua_pushboolean(lstate, e.idata2);
			nargs = 3;
		break;

		case EVENT_GAMEPADPRESSED:
			lua_pushstring(lstate, "gamepadpressed");
			lua_pushinteger(lstate, e.idata1);
			lua_pushinteger(lstate, e.idata2);
			nargs = 3;
		break;

		case EVENT_GAMEPADRELEASED:
			lua_pushstring(lstate, "gamepadreleased");
			lua_pushinteger(lstate, e.idata1);
			lua_pushinteger(lstate, e.idata2);
			nargs = 3;
		break;

		case EVENT_GAMEPADPOVCHANGED:
			lua_pushstring(lstate, "gamepadpovchanged");
			lua_pushinteger(lstate, e.idata1);
			lua_pushnumber(lstate, e.fdata2);
			nargs = 3;
		break;

		case EVENT_GAMEPADAXISCHANGED:
			lua_pushstring(lstate, "gamepadaxischanged");
			lua_pushinteger(lstate, e.idata1);
			lua_pushinteger(lstate, e.idata2);
			lua_pushnumber(lstate, e.fdata3);
			nargs = 4;
		break;

		case EVENT_ERROR:
			lua_pushstring(lstate, "error");
			lua_pushstring(lstate, e.msg.c_str());
			nargs = 2;
		break;

		case EVENT_WARNING:
			lua_pushstring(lstate, "warning");
			lua_pushstring(lstate, e.msg.c_str());
			nargs = 2;
		break;

		case EVENT_CONSOLERESIZED:
			lua_pushstring(lstate, "consoleresized");
			nargs = 1;
		break;

		case EVENT_SOCKETCONNECTED:
		{
			lua_pushstring(lstate, "socketconnected");
			/*Socket *pSocket;
			Socket **ppSocket = static_cast<Socket **>(lua_newuserdata(lstate, sizeof(struct Socket)));
			*ppSocket = pSocket;*/
			Socket **ppSocket = static_cast<Socket **>(lua_newuserdata(lstate, sizeof(struct Socket)));
			*ppSocket = e.pSocket;
			printf("socketconnected event.\n");
/*			pSocket->socket		= e.pSocket->socket;
			pSocket->port		= e.pSocket->port;
			pSocket->protocol	= e.pSocket->protocol;
			pSocket->hThread	= e.pSocket->hThread;
*/

			// Give it a metatable
			luaL_getmetatable(lstate, LuaType::metatable_socket);
			lua_setmetatable(lstate, -2);

			nargs = 2;
		}
		break;

		case EVENT_SOCKETRECEIVED:
			lua_pushstring(lstate, "socketreceived");
			lua_pushinteger(lstate, e.idata1);
			lua_pushlstring(lstate, e.msg.c_str(), e.msg.size());
			nargs = 3;
		break;

		case EVENT_SOCKETDISCONNECTED:
			lua_pushstring(lstate, "socketdisconnected");
			lua_pushinteger(lstate, e.idata1);
			nargs = 2;
		break;

		case EVENT_SOCKETERROR:
			lua_pushstring(lstate, "socketerror");
			lua_pushinteger(lstate, e.idata1);
			lua_pushinteger(lstate, e.idata2);
			nargs = 3;
		break;

		case EVENT_QUIT:
			lua_pushstring(lstate, "quit");
			nargs = 1;
		break;

		case EVENT_UNKNOWN:
		default:
			lua_pushstring(lstate, "unknown");
			nargs = 1;
		break;
	}

	int failstate = lua_pcall(lstate, nargs, 0, stackbase+1);
	int retval = MicroMacro::ERR_OK;
	if( failstate )
	{
		stdError();
		switch(failstate) {
			case LUA_ERRRUN:		retval = MicroMacro::ERR_RUN;		break;
			case LUA_ERRMEM:		retval = MicroMacro::ERR_MEM;		break;
			case LUA_ERRSYNTAX:		retval = MicroMacro::ERR_SYNTAX;	break;
			case LUA_ERRFILE:		retval = MicroMacro::ERR_FILE;		break;
			case LUA_ERRERR:		retval = MicroMacro::ERR_ERR;		break;
			default:				retval = MicroMacro::ERR_UNKNOWN;	break;
		}
	}

	lua_pop(lstate, 2); // Pop stacktrace, macro table
	return retval;
}

float LuaEngine::getDeltaTime()
{
	return fDeltaTime;
}

// Returns the last error on the Lua state as a string
std::string LuaEngine::getLastErrorMessage()
{
	return lastErrorMsg;
}

lua_State *LuaEngine::getLuaState()
{
	return lstate;
}

std::string LuaEngine::getBasePath()
{
	return basePath;
}

void LuaEngine::setBasePath(std::string np)
{
	basePath = np;
}
