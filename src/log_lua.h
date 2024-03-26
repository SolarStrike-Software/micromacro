/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef LOG_LUA_H
#define LOG_LUA_H

	#define LOG_MODULE_NAME			"log"
	typedef struct lua_State lua_State;

	class Log_lua
	{
		protected:
			static int getFilename(lua_State *);
			static int add(lua_State *);
			static int addRaw(lua_State *);
			static int isOpen(lua_State *);

			static int setLevel(lua_State *);
			static int emergency(lua_State *);
			static int alert(lua_State *);
			static int critical(lua_State *);
			static int error(lua_State *);
			static int warning(lua_State *);
			static int notice(lua_State *);
			static int info(lua_State *);
			static int debug(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
