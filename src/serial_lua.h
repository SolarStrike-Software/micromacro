/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef SERIAL_LUA_H
#define SERIAL_LUA_H

	#define SERIAL_MODULE_NAME			"serial"

	typedef struct lua_State lua_State;

	class Serial_lua
	{
		protected:
			static int open(lua_State *);

		public:
			static int regmod(lua_State *);
			static int cleanup(lua_State *);
	};

#endif
