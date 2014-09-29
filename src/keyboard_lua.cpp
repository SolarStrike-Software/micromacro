/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "keyboard_lua.h"
#include "error.h"
#include "macro.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}


int Keyboard_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"pressed", Keyboard_lua::pressed},
		{"released", Keyboard_lua::released},
		{"isDown", Keyboard_lua::isDown},
		{"getToggleState", Keyboard_lua::getToggleState},
		{"press", Keyboard_lua::press},
		{"hold", Keyboard_lua::hold},
		{"release", Keyboard_lua::release},
		{"virtualPress", Keyboard_lua::virtualPress},
		{"virtualHold", Keyboard_lua::virtualHold},
		{"virtualRelease", Keyboard_lua::virtualRelease},
		{"getKeyName", Keyboard_lua::getKeyName},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, KEYBOARD_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

/*	keyboard.pressed(number vk)
	Returns:	boolean

	If the given key identified by vk was pressed
	since last polling, returns true. Else, false.
*/
int Keyboard_lua::pressed(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = lua_tointeger(L, 1);
	if( vk > VK_XBUTTON2 && vk != 0 )
		lua_pushboolean(L, Macro::instance()->getHid()->pressed(vk));
	else
		lua_pushboolean(L, false);
	return 1;
}

/*	keyboard.released(number vk)
	Returns:	boolean

	If the given key identified by vk was released
	since last polling, returns true. Else, false.
*/
int Keyboard_lua::released(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = lua_tointeger(L, 1);
	if( vk > VK_XBUTTON2 && vk != 0 )
		lua_pushboolean(L, Macro::instance()->getHid()->released(vk));
	else
		lua_pushboolean(L, false);
	return 1;
}

/*	keyboard.isDown(number vk)
	Returns:	boolean

	If the given key identified by vk is currently
	held down (as of last polling), returns true. Else, false.
*/
int Keyboard_lua::isDown(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = lua_tointeger(L, 1);
	if( vk > VK_XBUTTON2 && vk != 0 )
		lua_pushboolean(L, Macro::instance()->getHid()->isDown(vk));
	else
		lua_pushboolean(L, false);
	return 1;
}

/*	keyboard.getToggleState(number vk)
	Returns:	boolean

	Returns the toggle state (true = on, false = off)
	for the given key identified by vk.
*/
int Keyboard_lua::getToggleState(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = lua_tointeger(L, 1);
	if( vk > VK_XBUTTON2 && vk != 0 )
		lua_pushboolean(L, Macro::instance()->getHid()->getToggleState(vk));
	else
		lua_pushboolean(L, false);
	return 1;
}

/*	keyboard.press(number vk [, boolean async])
	Returns:	nil

	Attempts to send a synthetic press for the given key.
	If async is true (default), it is queued for automatic
	release. Otherwise, execution is blocked while waiting for release.
*/
int Keyboard_lua::press(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	if( top == 2 )
		checkType(L, LT_BOOLEAN, 2);

	int vk = lua_tointeger(L, 1);
	bool async = true;
	if( top == 2 )
		async = lua_toboolean(L, 2);
	if( vk > VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->press(vk, async);
	return 0;
}

/*	keyboard.hold(number vk)
	Returns:	nil

	Attempts to send a synthetic hold for the given key.
*/
int Keyboard_lua::hold(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = lua_tointeger(L, 1);
	if( vk > VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->hold(vk);
	return 0;
}

/*	keyboard.release(number vk)
	Returns:	nil

	Attempts to send a synthetic release for the given key.
*/
int Keyboard_lua::release(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = lua_tointeger(L, 1);
	if( vk > VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->release(vk);
	return 0;
}

/*	keyboard.virtualPress(number hwnd, number vk [, boolean async])
	Returns:	nil

	Attempts to send a synthetic press for the given key, and sends
	that input directly to the given window.
	If async is true (default), it is queued for automatic
	release. Otherwise, execution is blocked while waiting for release.
*/
int Keyboard_lua::virtualPress(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 && top != 3 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	if( top == 3 )
		checkType(L, LT_BOOLEAN, 3);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	int vk = lua_tointeger(L, 2);
	bool async = true;
	if( top == 3 )
		async = lua_toboolean(L, 3);
	if( vk > VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->virtualPress(hwnd, vk, async);
	return 0;
}

/*	keyboard.virtualHold(number hwnd, number vk)
	Returns:	nil

	Attempts to send a synthetic hold for the given key, and sends
	that input directly to the given window.
*/
int Keyboard_lua::virtualHold(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	int vk = lua_tointeger(L, 2);
	if( vk > VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->virtualHold(hwnd, vk);
	return 0;
}

/*	keyboard.virtualRelease(number hwnd, number vk)
	Returns:	nil

	Attempts to send a synthetic release for the given key, and sends
	that input directly to the given window.
*/
int Keyboard_lua::virtualRelease(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	int vk = lua_tointeger(L, 2);
	if( vk > VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->virtualRelease(hwnd, vk);
	return 0;
}

/*	keyboard.getKeyName(number vk)
	Returns:	string (key name)

	Accepts a virtual key and returns the key's name as a string.
*/
int Keyboard_lua::getKeyName(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int key = lua_tointeger(L, 1);

	UINT scan = MapVirtualKey(key, 0);
	LPARAM lparam;
	if( Macro::instance()->getHid()->keyIsExtended(key) )
		lparam = (scan << 16) | POSTMESSAGE_EXTENDED;
	else
		lparam = (scan << 16);
	char buf[256];

	GetKeyNameText(lparam, buf, sizeof(buf)-1);
	lua_pushstring(L, buf);
	return 1;
}
