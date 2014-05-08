/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef INT64_LUA_H
#define INT64_LUA_H

	typedef union _LARGE_INTEGER LARGE_INTEGER;
	typedef struct lua_State lua_State;

	namespace LuaType
	{
		extern const char *int64_highpart_name;
		extern const char *int64_lowpart_name;
	}

	class Int64_lua
	{
		protected:
			static int tostring(lua_State *);
			static int add(lua_State *);
			static int sub(lua_State *);
			static int mul(lua_State *);
			static int div(lua_State *);
			static int eq(lua_State *);
			static int lt(lua_State *);
			static int gt(lua_State *);

		public:
			static int regmod(lua_State *);
	};

	bool lua_isint64(lua_State *, int);
	LARGE_INTEGER lua_toint64(lua_State *, int);
	void lua_pushint64(lua_State *, LARGE_INTEGER);

#endif
