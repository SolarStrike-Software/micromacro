/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include "wininclude.h"
#include "cli_lua.h"
#include "macro.h"
#include "error.h"

int Cli_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"clear", Cli_lua::clear},
		{"resetColor", Cli_lua::resetColor},
		{"getColor", Cli_lua::getColor},
		{"setColor", Cli_lua::setColor},
		{"getAttributes", Cli_lua::getAttributes},
		{"setAttributes", Cli_lua::setAttributes},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);

	lua_pushinteger(L, 0);
	lua_setfield(L, -2, "BLACK");
	lua_pushinteger(L, 1);
	lua_setfield(L, -2, "BLUE");
	lua_pushinteger(L, 2);
	lua_setfield(L, -2, "GREEN");
	lua_pushinteger(L, 3);
	lua_setfield(L, -2, "CYAN");
	lua_pushinteger(L, 4);
	lua_setfield(L, -2, "RED");
	lua_pushinteger(L, 5);
	lua_setfield(L, -2, "MAGENTA");
	lua_pushinteger(L, 6);
	lua_setfield(L, -2, "YELLOW");
	lua_pushinteger(L, 7);
	lua_setfield(L, -2, "WHITE");
	lua_pushinteger(L, 8);
	lua_setfield(L, -2, "GRAY");
	lua_pushinteger(L, 9);
	lua_setfield(L, -2, "LIGHT_BLUE");
	lua_pushinteger(L, 10);
	lua_setfield(L, -2, "LIGHT_GREEN");
	lua_pushinteger(L, 11);
	lua_setfield(L, -2, "LIGHT_CYAN");
	lua_pushinteger(L, 12);
	lua_setfield(L, -2, "LIGHT_RED");
	lua_pushinteger(L, 13);
	lua_setfield(L, -2, "LIGHT_MAGENA");
	lua_pushinteger(L, 14);
	lua_setfield(L, -2, "LIGHT_YELLOW");
	lua_pushinteger(L, 15);
	lua_setfield(L, -2, "LIGHT_WHITE");

	lua_setglobal(L, CLI_MODULE_NAME);


	return MicroMacro::ERR_OK;
}

/*	cli.clear()
	Returns:	nil

	Clears the console screen. Nothing fancy.
*/
int Cli_lua::clear(lua_State *L)
{
	HANDLE stdOut = Macro::instance()->getAppHandle();
	COORD coord = {0, 0};
	DWORD count;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(stdOut, &csbi);

	FillConsoleOutputCharacter(stdOut, ' ', csbi.dwSize.X * csbi.dwSize.Y,
		coord, &count);

	SetConsoleCursorPosition(stdOut, coord);
	return 0;
}

/*	cli.resetColor()
	Returns:	nil

	Resets the console's text attributes back to default
*/
int Cli_lua::resetColor(lua_State *L)
{
	HANDLE stdOut = Macro::instance()->getAppHandle();
	DWORD attributes = Macro::instance()->getConsoleDefaultAttributes();
	SetConsoleTextAttribute(stdOut, attributes);

	return 0;
}

/*	cli.getColor()
	Returns:	number

	Returns the console's current attribute mask detailing its text color
*/
int Cli_lua::getColor(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(Macro::instance()->getAppHandle(), &csbi);
	lua_pushinteger(L, csbi.wAttributes);

	return 1;
}

/*	cli.getColor(number color)
	Returns:	nil

	Change the console's text (and/or background) color
*/
int Cli_lua::setColor(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);


	DWORD foreground = lua_tointeger(L, 1);
	DWORD background = 0;
	if( top >= 2 )
	{
		checkType(L, LT_NUMBER, 2);
		background = lua_tointeger(L, 2);
	}

	DWORD attributes = foreground + (16*background);
	HANDLE stdOut = Macro::instance()->getAppHandle();
	SetConsoleTextAttribute(stdOut, attributes);
	return 0;
}


/*	cli.getAttributes()
	Returns (on success):	number windowWidth
							number windowHeight
							number bufferWidth
							number bufferHeight
							number cursorX
							number cursorY

	Returns (on failure): nil

	Returns a set of console attributes.
	Values are in characters, not pixels!
*/
int Cli_lua::getAttributes(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	CONSOLE_SCREEN_BUFFER_INFO lpcsbi;
	if( GetConsoleScreenBufferInfo(Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/, &lpcsbi) )
	{
		lua_pushnumber(L, lpcsbi.srWindow.Right + 1);
		lua_pushnumber(L, lpcsbi.srWindow.Bottom - lpcsbi.srWindow.Top + 1);
		lua_pushnumber(L, lpcsbi.dwSize.X);
		lua_pushnumber(L, lpcsbi.dwSize.Y);
		lua_pushnumber(L, lpcsbi.dwCursorPosition.X + 1);
		lua_pushnumber(L, lpcsbi.dwCursorPosition.Y + 1);
		return 6;
	}
	else
		return 0; // Error
}


/*	cli.getAttributes(number windowWidth, number windowHeight,
								number bufferWidth, number bufferHeight)
	Returns:	nil

	Modify the console's attributes.
	Values should be in characters, not pixels.
*/
int Cli_lua::setAttributes(lua_State *L)
{
	int args = lua_gettop(L);
	if( args != 2 && args != 4 )
		wrongArgs(L);

	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	if( args >= 4 )
	{
		checkType(L, LT_NUMBER, 3);
		checkType(L, LT_NUMBER, 4);
	}

	// Get original buffer size
	CONSOLE_SCREEN_BUFFER_INFO lpcsbi;
	GetConsoleScreenBufferInfo(Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/, &lpcsbi);

	// For window size
	SMALL_RECT rect;
	rect.Right = lua_tointeger(L, 1) - 1;
	rect.Bottom = lua_tointeger(L, 2) - 1;
	rect.Left = 0;
	rect.Top = 0;

	// For buffer size
	COORD coord;
	coord.X = lpcsbi.dwSize.X;
	coord.Y = lpcsbi.dwSize.Y;

	// Set buffer size
	if( args >= 4 )
	{
		coord.X = lua_tointeger(L, 3);
		coord.Y = lua_tointeger(L, 4);
	}

	// Ensure that the buffer will be of proper size
	COORD tmpCoord;
	tmpCoord.X = coord.X;
	tmpCoord.Y = coord.Y;
	if( lpcsbi.dwSize.X > tmpCoord.X )
		tmpCoord.X = lpcsbi.dwSize.X;
	if( lpcsbi.dwSize.Y > tmpCoord.Y )
		tmpCoord.Y = lpcsbi.dwSize.Y;

	// Note: We set the buffer both before and after because it is
	// easier than checking current vs. future screen size vs. buffer size
	SetConsoleScreenBufferSize(Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/, tmpCoord);
	SetConsoleWindowInfo(Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/, true, &rect);
	SetConsoleScreenBufferSize(Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/, coord);

	return 0;
}
