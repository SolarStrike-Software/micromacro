/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef NCURSES_LUA_H
#define NCURSES_LUA_H

	#define NCURSES_MODULE_NAME		"ncurses"

	#include <ncursesw/ncurses.h>


	typedef struct lua_State lua_State;

	class Ncurses_lua
	{
		protected:
			static bool initialized;
			static const char *stdscr_name;

			static void flush(WINDOW *);
			static void readline(WINDOW *, char *, size_t);

			static int print(lua_State *);
			static int refresh(lua_State *);
			static int scrollok(lua_State *);
			static int clear(lua_State *);
			static int move(lua_State *);
			static int createWindow(lua_State *);
			static int resizeWindow(lua_State *);
			static int moveWindow(lua_State *);
			static int getString(lua_State *);
			static int setPair(lua_State *);
			static int getPair(lua_State *);
			static int attributeOn(lua_State *);
			static int attributeOff(lua_State *);
			static int setAttribute(lua_State *);
			static int getAttribute(lua_State *);
			static int setBackground(lua_State *);
			static int getWindowSize(lua_State *);

		public:
			static int is_initialized();
			static int regmod(lua_State *);
			static int init(lua_State *);
			static int cleanup(lua_State *);
	};
#endif
