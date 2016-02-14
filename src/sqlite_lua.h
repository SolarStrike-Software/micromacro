/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef SQLITE_LUA_H
#define SQLITE_LUA_H

	#define SQLITE_MODULE_NAME		"sqlite"
	typedef struct lua_State lua_State;

	class Sqlite_lua
	{
		protected:
			static int callback(void *, int, char **, char **); // Used internally

			static int open(lua_State *);
			static int close(lua_State *);
			static int execute(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
