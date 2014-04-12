#ifndef TIMER_LUA_H
#define TIMER_LUA_H

	#define TIMER_MODULE_NAME		"timer"
	typedef struct lua_State lua_State;

	#include "error.h"

	class Timer_lua
	{
		protected:
			static int getNow(lua_State *);
			static int deltaTime(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
