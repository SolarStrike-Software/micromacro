/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef LUATYPES_H
#define LUATYPES_H

	typedef struct lua_State lua_State;
	typedef union _LARGE_INTEGER LARGE_INTEGER;

	namespace LuaType
	{
		extern const char *metatable_ncursesWindow;
		extern const char *metatable_handle;
		//extern const char *metatable_windowDC;
		extern const char *metatable_sqlitedb;

		int ncursesWindow_gc(lua_State *);
		int ncursesWindow_tostring(lua_State *);

		int handle_gc(lua_State *);
		int handle_tostring(lua_State *);

		/*int windowDC_gc(lua_State *);
		int windowDC_tostring(lua_State *);*/

		int sqlitedb_gc(lua_State *);
		int sqlitedb_tostring(lua_State *);
	}

	int registerLuaTypes(lua_State *);
#endif
