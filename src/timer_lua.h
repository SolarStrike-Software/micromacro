/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef TIMER_LUA_H
#define TIMER_LUA_H

	#define TIMER_MODULE_NAME		"timer"
	typedef struct lua_State lua_State;

	class Timer_lua
	{
		protected:
			static int getNow(lua_State *);
			static int deltaTime(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
