#include "luaengine.h"
#include "luatypes.h"
#include "macro.h"
#include "ncurses_lua.h"
#include "timer_lua.h"
#include "keyboard_lua.h"
#include "mouse_lua.h"
#include "key_lua.h"
#include "gamepad_lua.h"
#include "system_lua.h"
#include "filesystem_lua.h"
#include "process_lua.h"
#include "window_lua.h"
#include "audio_lua.h"
#include "class_lua.h"
#include "log_lua.h"

#include "global_addon.h"
#include "string_addon.h"
#include "math_addon.h"
#include "table_addon.h"

#include "event.h"

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

// Return the main application's HWND
int LuaEngine::_macrotab_appHwnd(lua_State *L)
{
	lua_pushnumber(L, (int)Macro::instance()->getAppHwnd());
	return 1;
}

// Return the main focused HWND
int LuaEngine::_macrotab_focusHwnd(lua_State *L)
{
	lua_pushnumber(L, (int)Macro::instance()->getForegroundWindow());
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
		{"getAppHwnd", LuaEngine::_macrotab_appHwnd},
		{"getFocusHwnd", LuaEngine::_macrotab_focusHwnd},
		{NULL, NULL}
	};

	luaL_newlib(lstate, _macrotab);
	lua_setglobal(lstate, MACRO_TABLE_NAME);

	/* Register types (metatables) */
	registerLuaTypes(lstate);

	/* Add modules */
	Ncurses_lua::regmod(lstate);
	Timer_lua::regmod(lstate);
	Keyboard_lua::regmod(lstate);
	Mouse_lua::regmod(lstate);
	Key_lua::regmod(lstate);
	Gamepad_lua::regmod(lstate);
	System_lua::regmod(lstate);
	Filesystem_lua::regmod(lstate);
	Process_lua::regmod(lstate);
	Window_lua::regmod(lstate);
	Audio_lua::regmod(lstate);
	Class_lua::regmod(lstate);
	Log_lua::regmod(lstate);

	/* Addons */
	Global_addon::regmod(lstate);
	String_addon::regmod(lstate);
	Math_addon::regmod(lstate);
	Table_addon::regmod(lstate);

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

	Audio_lua::cleanup(lstate);

	if( Ncurses_lua::is_initialized() )
		Ncurses_lua::cleanup(lstate);

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
	return retval;
}

/*	Run the script's macro.main() function
	Pushes delta time (in seconds) as first argument
	Expects one boolean in return: true(continue execution) or false(stop execution)
*/
int LuaEngine::runMain(double dt)
{
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

	lua_pushnumber(lstate, dt);
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

// Returns the last error on the Lua state as a string
std::string LuaEngine::getLastErrorMessage()
{
	return lastErrorMsg;
}

lua_State *LuaEngine::getLuaState()
{
	return lstate;
}
