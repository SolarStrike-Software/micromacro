/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef KEY_LUA_H
#define KEY_LUA_H

	#define KEY_MODULE_NAME			"key"

	typedef struct lua_State lua_State;

	class Key_lua
	{
		protected:

		public:
			static int regmod(lua_State *L);
	};

#endif
