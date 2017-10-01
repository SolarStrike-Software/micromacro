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


int Keyboard_lua::luaHookCallbackRef = 0;
HHOOK Keyboard_lua::hKeyboardHook = NULL;

int Keyboard_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"pressed", Keyboard_lua::pressed},
		{"released", Keyboard_lua::released},
		{"isDown", Keyboard_lua::isDown},
		{"getToggleState", Keyboard_lua::getToggleState},
		{"setToggleState", Keyboard_lua::setToggleState},
		{"press", Keyboard_lua::press},
		{"hold", Keyboard_lua::hold},
		{"release", Keyboard_lua::release},
		{"virtualPress", Keyboard_lua::virtualPress},
		{"virtualHold", Keyboard_lua::virtualHold},
		{"virtualRelease", Keyboard_lua::virtualRelease},
		{"virtualType", Keyboard_lua::virtualType},
		{"getKeyName", Keyboard_lua::getKeyName},
		{"setHookCallback", Keyboard_lua::setHookCallback},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, KEYBOARD_MODULE_NAME);

	luaHookCallbackRef = 0;

	return MicroMacro::ERR_OK;
}

int Keyboard_lua::cleanup(lua_State *)
{
	removeHook();
	return 0;
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

/*	keyboard.settToggleState(number vk, boolean status)
	Returns:	nil

	Sets the given keys toggle status.
*/
int Keyboard_lua::setToggleState(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_BOOLEAN, 2);

	int vk = lua_tointeger(L, 1);
	bool status = lua_toboolean(L, 2);
	if( vk > VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->setToggleState(vk, status);

	return 0;
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

/*	keyboard.virtualType(number hwnd, string msg)
	Returns:	nil

	Attempts to send a synthetic message to a window, as if the user typed it.
*/
int Keyboard_lua::virtualType(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_STRING, 2);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	std::string msg = lua_tostring(L, 2);

	for(unsigned int i = 0; i < msg.size(); i++)
	{
		char chr = msg.at(i);
		LPARAM lparam = 0;
		PostMessage(hwnd, WM_CHAR, chr, lparam);
	}

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


/*
	Used internally; when a keyboard hook is installed, this function will be used as
	the callback, and pass the torch on to the user-defined callback.
*/
LRESULT CALLBACK Keyboard_lua::lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if( nCode < 0 || nCode != HC_ACTION ) // do not process message
		return CallNextHookEx( hKeyboardHook, nCode, wParam, lParam);

	bool ignoreKey = false;
    KBDLLHOOKSTRUCT &event = *(PKBDLLHOOKSTRUCT)lParam;

    switch (wParam)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
        {
			/* Try the Lua callback, see what it returns for us */
			lua_State *lstate = Macro::instance()->getEngine()->getLuaState();

			int stackbase = lua_gettop(lstate);

			/* Look up the function and check it before attempting to use it */
			lua_rawgeti(lstate, LUA_REGISTRYINDEX, luaHookCallbackRef);
			if( lua_type(lstate, 1) != LUA_TFUNCTION )
				break;

			lua_pushinteger(lstate, event.vkCode);	// Push the key code

			// Push the event type (up/down)
			if( wParam == WM_KEYUP || wParam == WM_SYSKEYUP )
				lua_pushstring(lstate, "up");
			else
				lua_pushstring(lstate, "down");

			int failstate = lua_pcall(lstate, 2, 1, 0);
			if( failstate == LUA_OK )
			{
				if( !lua_isnil(lstate, -1) )
				{
					ignoreKey = lua_toboolean(lstate, -1);
					lua_pop(lstate, 1); // Pop return value;
				}
			} else
			{ /* Throw an error */
				//stdError();
				const char *lastErrorMsg = lua_tostring(lstate, lua_gettop(lstate));
				Macro::instance()->getEngine()->setLastErrorMessage(lastErrorMsg);

				int errState = 0;
				switch(failstate) {
					case LUA_ERRRUN:		errState = MicroMacro::ERR_RUN;		break;
					case LUA_ERRMEM:		errState = MicroMacro::ERR_MEM;		break;
					case LUA_ERRSYNTAX:		errState = MicroMacro::ERR_SYNTAX;	break;
					case LUA_ERRFILE:		errState = MicroMacro::ERR_FILE;	break;
					case LUA_ERRERR:		errState = MicroMacro::ERR_ERR;		break;
					default:				errState = MicroMacro::ERR_UNKNOWN;	break;
				}
				Macro::instance()->getEngine()->setKeyHookErrorState(errState);
				lua_pop(lstate, 1);
			}


            break;
        }
    }

    if( ignoreKey )
        return 1;
    else
        return CallNextHookEx( hKeyboardHook, nCode, wParam, lParam );
}

/*	keyboard.setHookCallback(nil|function callback)
	Returns:	boolean

	Attempt to install a keyboard hook. The callback should be a function
	that accepts a number: the virtual key code. Some events may push
	additional data.
	If the callback returns true, the key will be dropped.
	If the callback returns false or nil, the input be left in the queue.

	If you pass nil as the callback, the keyboard hook will instead
	be removed.

	Please note that your callback function should return almost immediately
	so you aren't clogging up the input system.
	Your main function must also execute quickly (so don't use any rests!).
	Failure to execute quickly may lead to dropped or lagged user input.
*/
int Keyboard_lua::setHookCallback(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_FUNCTION | LT_NIL, 1);

	// Lets remove the hook
	if( lua_isnil(L, 1) )	//User requested removal
	{
		removeHook();
		lua_pushboolean(L, true);
		return 1;
	}

	if( hKeyboardHook )
	{
		removeHook();	// We're installing a new hook, so remove the old one.
	}

	// Lets install the hook
	hKeyboardHook = SetWindowsHookEx( WH_KEYBOARD_LL,  lowLevelKeyboardProc, GetModuleHandle(NULL), 0 );
	if( !hKeyboardHook )
	{
		lua_pushboolean(L, false);
		return 1;
	}

	luaHookCallbackRef = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushboolean(L, true);
	return 1;
}

/*
	Removes the keyboard hook. Nothing fancy here.
*/
int Keyboard_lua::removeHook()
{
	if( hKeyboardHook )
	{
		UnhookWindowsHookEx( hKeyboardHook );
		hKeyboardHook = 0;
	}
	luaHookCallbackRef = 0;

	return 0;
}
