/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef CLASS_LUA_H
#define CLASS_LUA_H

	#define CLASS_MODULE_NAME		"class"

	//typedef struct lua_State lua_State;

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

	class Class_lua
	{
		protected:
			static int _new(lua_State *);

			static int __call(lua_State *);
			static int __tostring(lua_State *);

			static int vector3d(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
