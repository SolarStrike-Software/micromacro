/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef TIME_LUA_H
#define TIME_LUA_H

	#define TIME_MODULE_NAME		"time"
	typedef struct lua_State lua_State;

	class Time_lua
	{
		protected:
			static int getNow(lua_State *);
			static int deltaTime(lua_State *);
			static int diff(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
