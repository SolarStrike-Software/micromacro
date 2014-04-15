/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef FILESYSTEM_LUA_H
#define FILESYSTEM_LUA_H

	#define FILESYSTEM_MODULE_NAME			"filesystem"

	typedef struct lua_State lua_State;

	class Filesystem_lua
	{
		protected:
			static int directoryExists(lua_State *);
			static int fileExists(lua_State *);
			static int getDirectory(lua_State *);
			static int isDirectory(lua_State *);
			static int makeDirectory(lua_State *);
			static int fixSlashes(lua_State *);
			static int getOpenFileName(lua_State *);
			static int getSaveFileName(lua_State *);
			static int getCWD(lua_State *);
			static int setCWD(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
