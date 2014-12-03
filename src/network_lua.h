/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef NETWORK_LUA_H
#define NETWORK_LUA_H
	#ifdef NETWORKING_ENABLED
	#define NETWORK_MODULE_NAME		"network"
	typedef struct lua_State lua_State;

	class Network_lua
	{
		protected:
			static int socket(lua_State *);

		public:
			static int regmod(lua_State *);
			static int cleanup();
	};
	#endif

#endif
