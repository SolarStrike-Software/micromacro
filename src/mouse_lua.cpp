#include "mouse_lua.h"
#include "error.h"
#include "macro.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <cmath>

int Mouse_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"pressed", Mouse_lua::pressed},
		{"released", Mouse_lua::released},
		{"isDown", Mouse_lua::isDown},
		{"press", Mouse_lua::press},
		{"hold", Mouse_lua::hold},
		{"release", Mouse_lua::release},
		{"move", Mouse_lua::move},
		{"setPosition", Mouse_lua::setPosition},
		{"getPosition", Mouse_lua::getPosition},
		{"wheelMove", Mouse_lua::wheelMove},
		{"virtualPress", Mouse_lua::virtualPress},
		{"virtualHold", Mouse_lua::virtualHold},
		{"virtualRelease", Mouse_lua::virtualRelease},
		{"virtualMove", Mouse_lua::virtualMove},
		{"virtualWheelMove", Mouse_lua::virtualWheelMove},
		{"setVirtualPosition", Mouse_lua::setVirtualPosition},
		{"getVirtualPosition", Mouse_lua::getVirtualPosition},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, MOUSE_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

int Mouse_lua::pressed(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = (int)lua_tonumber(L, 1);
	if( vk <= VK_XBUTTON2 && vk != 0)
		lua_pushboolean(L, Macro::instance()->getHid()->pressed(vk));
	else
		lua_pushboolean(L, false);
	return 1;
}

int Mouse_lua::released(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = (int)lua_tonumber(L, 1);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		lua_pushboolean(L, Macro::instance()->getHid()->released(vk));
	else
		lua_pushboolean(L, false);
	return 1;
}

int Mouse_lua::isDown(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = (int)lua_tonumber(L, 1);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		lua_pushboolean(L, Macro::instance()->getHid()->isDown(vk));
	else
		lua_pushboolean(L, false);
	return 1;
}

int Mouse_lua::press(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	if( top == 2 )
		checkType(L, LT_BOOLEAN, 2);

	int vk = (int)lua_tonumber(L, 1);
	bool async = true;
	if( top == 2 )
		async = lua_toboolean(L, 2);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->press(vk, async);
	return 0;
}

int Mouse_lua::hold(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = (int)lua_tonumber(L, 1);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->hold(vk);
	return 0;
}


int Mouse_lua::release(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int vk = (int)lua_tonumber(L, 1);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->release(vk);
	return 0;
}

int Mouse_lua::move(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int x = (int)lua_tonumber(L, 1);
	int y = (int)lua_tonumber(L, 2);

	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.dx = x;
	inp.mi.dy = y;
	inp.mi.dwFlags = MOUSEEVENTF_MOVE;
	inp.mi.time = 0;
	SendInput(1, &inp, sizeof(INPUT));

	return 0;
}

int Mouse_lua::wheelMove(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	int delta = (int)lua_tonumber(L, 1);

	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.dwFlags = MOUSEEVENTF_WHEEL;
	inp.mi.mouseData = (DWORD)(delta);
	SendInput(1, &inp, sizeof(INPUT));
	return 0;
}

int Mouse_lua::setPosition(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	double fScreenWidth = ::GetSystemMetrics(SM_CXSCREEN)-1;
	double fScreenHeight = ::GetSystemMetrics(SM_CYSCREEN)-1;
	int x = (int)lua_tointeger(L, 1);
	int y = (int)lua_tointeger(L, 2);

	// Normalize coords to expected value
	x = round(x * (65535/fScreenWidth));
	y = round(y * (65535/fScreenHeight));

	INPUT inp;
	inp.type = INPUT_MOUSE;
	inp.mi.dx = (DWORD)x;
	inp.mi.dy = (DWORD)y;
	inp.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
	inp.mi.time = 0;
	SendInput(1, &inp, sizeof(INPUT));

	return 0;
}

int Mouse_lua::getPosition(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	POINT p;
	GetCursorPos(&p);
	lua_pushnumber(L, p.x);
	lua_pushnumber(L, p.y);
	return 2;
}

int Mouse_lua::virtualPress(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 && top != 3 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	if( top == 3 )
		checkType(L, LT_BOOLEAN, 3);

	HWND hwnd = (HWND)(int)lua_tonumber(L, 1);
	int vk = (int)lua_tonumber(L, 2);
	bool async = true;
	if( top == 3 )
		async = lua_toboolean(L, 3);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->virtualPress(hwnd, vk, async);
	return 0;
}

int Mouse_lua::virtualHold(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	HWND hwnd = (HWND)(int)lua_tonumber(L, 1);
	int vk = (int)lua_tonumber(L, 2);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->virtualHold(hwnd, vk);
	return 0;
}

int Mouse_lua::virtualRelease(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	HWND hwnd = (HWND)(int)lua_tonumber(L, 1);
	int vk = (int)lua_tonumber(L, 2);
	if( vk <= VK_XBUTTON2 && vk != 0 )
		Macro::instance()->getHid()->virtualRelease(hwnd, vk);
	return 0;
}

int Mouse_lua::virtualMove(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int dx, dy;
	int cx, cy;

	dx = (int)lua_tonumber(L, 1);
	dy = (int)lua_tonumber(L, 2);
	Macro::instance()->getHid()->getVirtualMousePos(cx, cy);
	Macro::instance()->getHid()->setVirtualMousePos(cx+dx, cy+dy);

	return 0;
}

int Mouse_lua::virtualWheelMove(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	int mx, my;
	Macro::instance()->getHid()->getVirtualMousePos(mx, my);

	HWND hwnd = (HWND)(int)lua_tonumber(L, 1);
	int delta = (int)lua_tonumber(L, 2);
	WPARAM wparam = MAKEWPARAM(0, delta);
	LPARAM lparam = MAKELPARAM(mx, my);
	PostMessage(hwnd, WM_MOUSEWHEEL, wparam, lparam);

	return 0;
}

int Mouse_lua::setVirtualPosition(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	unsigned int dx, dy;
	dx = (int)lua_tonumber(L, 1);
	dy = (int)lua_tonumber(L, 2);
	Macro::instance()->getHid()->setVirtualMousePos(dx, dy);

	return 0;
}

int Mouse_lua::getVirtualPosition(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	int cx, cy;
	Macro::instance()->getHid()->getVirtualMousePos(cx, cy);
	lua_pushnumber(L, cx);
	lua_pushnumber(L, cy);
	return 2;
}
