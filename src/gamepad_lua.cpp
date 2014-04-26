/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "gamepad_lua.h"
#include "event.h"
#include "error.h"
#include "macro.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

int Gamepad_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"pressed", Gamepad_lua::pressed},
		{"released", Gamepad_lua::released},
		{"isDown", Gamepad_lua::isDown},
		{"getPOV", Gamepad_lua::getPOV},
		{"getAxis", Gamepad_lua::getAxis},
		{"getCount", Gamepad_lua::getCount},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, GAMEPAD_MODULE_NAME);

	lua_pushnumber(L, JOY_POVBACKWARD/100);
	lua_setglobal(L, "JOY_POVBACKWARD");

	lua_pushnumber(L, JOY_POVFORWARD/100);
	lua_setglobal(L, "JOY_POVFORWARD");

	lua_pushnumber(L, JOY_POVCENTERED/100);
	lua_setglobal(L, "JOY_POVCENTERED");

	lua_pushnumber(L, JOY_POVLEFT/100);
	lua_setglobal(L, "JOY_POVLEFT");

	lua_pushnumber(L, JOY_POVRIGHT/100);
	lua_setglobal(L, "JOY_POVRIGHT");

	return MicroMacro::ERR_OK;
}

/*	gamepad.pressed(number gamepadId, number btn)
	Returns:	boolean

	If the given gamepad has had its button pressed
	since last polling, returns true. Else, false.
*/
int Gamepad_lua::pressed(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int joyId = lua_tointeger(L, 1);
	int button = lua_tointeger(L, 2) - 1;

	lua_pushboolean(L, Macro::instance()->getHid()->joyPressed(joyId, button) == 1);
	return 1;
}

/*	gamepad.released(number gamepadId, number btn)
	Returns:	boolean

	If the given gamepad has had its button released
	since last polling, returns true. Else, false.
*/
int Gamepad_lua::released(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int joyId = lua_tointeger(L, 1);
	int button = lua_tointeger(L, 2) - 1;

	lua_pushboolean(L, Macro::instance()->getHid()->joyReleased(joyId, button) == 1);
	return 1;
}

/*	gamepad.isDown(number gamepadId, number btn)
	Returns:	boolean

	If the given gamepad currently has a given
	button held down (as of last polling), returns true.
	Else, returns false.
*/
int Gamepad_lua::isDown(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int joyId = lua_tointeger(L, 1);
	int button = lua_tointeger(L, 2) - 1;

	lua_pushboolean(L, Macro::instance()->getHid()->joyIsDown(joyId, button) == 1);
	return 1;
}

/*	gamepad.getPOV(number gamepadId)
	Returns:	number

	Returns the angle of the POV hat (D-pad).
	If centered, returns JOY_POVCENTERED
*/
int Gamepad_lua::getPOV(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int joyId = lua_tointeger(L, 1);
	lua_pushnumber(L, Macro::instance()->getHid()->joyPOV(joyId)/100);
	return 1;
}

/*	gamepad.getAxis(number gamepadId, number axisId)
	Returns:	number

	Returns the axis value for a given gamepad.
	The value will be between 0 (minimum) and 100(maximum).
	If centered, the value will be ~50.

	NOTE: This will not always be exact! Due to calibration,
	floating-point inaccuracies, and physical defects,
	you may receive a value that is a fraction off
	the intended value. I.e. ~49-51 while centered.
*/
int Gamepad_lua::getAxis(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int joyId = lua_tointeger(L, 1);
	int axisId = lua_tointeger(L, 2);
	lua_pushnumber(L, Macro::instance()->getHid()->joyAxis(joyId, axisId)/65536.0f*100);
	return 1;
}

/*	NOTE: Commented out until I can find a way
	to make this work.

int Gamepad_lua::press(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 && top != 3 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	if( top == 3 )
		checkType(L, LT_NIL | LT_NUMBER, 3);

	bool async = true;
	int gamepad = (int)lua_tonumber(L, 1);
	int button = (int)lua_tonumber(L, 2);
	if( top == 3 )
		async = (bool)lua_toboolean(L, 3);

	Macro::instance()->getHid()->joyPress(gamepad, button, async);

	return 0;
}

int Gamepad_lua::hold(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int gamepad = (int)lua_tonumber(L, 1);
	int button = (int)lua_tonumber(L, 2);

	Macro::instance()->getHid()->joyHold(gamepad, button);

	return 0;
}

int Gamepad_lua::release(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int gamepad = (int)lua_tonumber(L, 1);
	int button = (int)lua_tonumber(L, 2);

	Macro::instance()->getHid()->joyRelease(gamepad, button);

	return 0;
}*/

/*	gamepad.getCount()
	Returns:	number

	Returns the number of connected gamepads.
*/
int Gamepad_lua::getCount(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);
	lua_pushnumber(L, Macro::instance()->getHid()->getGamepadCount());
	return 1;
}
