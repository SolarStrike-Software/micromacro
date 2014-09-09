/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef CLI_LUA_H
#define CLI_LUA_H

	#define CLI_MODULE_NAME		    "cli"

	typedef struct lua_State lua_State;


	class Cli_lua
	{
		protected:
			static int clear(lua_State *);
            static int resetColor(lua_State *);
            static int getColor(lua_State *);
            static int setColor(lua_State *);
			static int getAttributes(lua_State *);
			static int setAttributes(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
