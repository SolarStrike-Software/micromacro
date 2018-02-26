/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef SERIAL_COM_LUA_H
#define SERIAL_COM_LUA_H

	#define SERIAL_BUFFER_SIZE		10240

	namespace LuaType
	{
		extern const char *metatable_serial_port;
	}

	typedef struct lua_State lua_State;

	class Serial_port_lua
	{
		protected:
			static int isConnected(lua_State *);
			static int close(lua_State *);
			static int gc(lua_State *);
			static int tostring(lua_State *);
			static int read(lua_State *);
			static int write(lua_State *);

		public:
			static int regmod(lua_State *);
			static int cleanup(lua_State *);
	};


#endif
