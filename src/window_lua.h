/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef WINDOW_LUA_H
#define WINDOW_LUA_H

	#include <string>
	#include "wininclude.h"

	#define WINDOW_MODULE_NAME			"window"
	typedef struct lua_State lua_State;

	extern const char *windowThumbnailClassName;

	class Window_lua
	{
		protected:
			static BOOL CALLBACK _findProc(HWND, LPARAM);
			static BOOL CALLBACK _findListProc(HWND, LPARAM);

			static int find(lua_State *);
			static int findList(lua_State *);
			static int getParent(lua_State *);
			static int getTitle(lua_State *);
			static int setTitle(lua_State *);
			static int getClassName(lua_State *);
			static int valid(lua_State *);
			static int getRect(lua_State *);
			static int setRect(lua_State *);
			static int getClientRect(lua_State *);
			static int setClientRect(lua_State *);
			static int show(lua_State *);
			static int flash(lua_State *);
			//static int openDC(lua_State *); // No functions require a DC yet
			//static int closeDC(lua_State *);
			static int getPixel(lua_State *);
			static int pixelSearch(lua_State *);
			static int saveScreenshot(lua_State *);

		public:
			static int regmod(lua_State *);

	};
#endif
