/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef MEMORYCHUNK_LUA_H
#define MEMORYCHUNK_LUA_H

	typedef struct lua_State lua_State;

	namespace LuaType
	{
		extern const char *metatable_memorychunk;
	}

	class MemoryChunk_lua
	{
		protected:
			static int gc(lua_State *);
			static int tostring(lua_State *);
			static int getSize(lua_State *);
			static int getAddress(lua_State *);
			static int getData(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
