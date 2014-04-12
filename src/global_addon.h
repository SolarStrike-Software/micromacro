/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef GLOBAL_ADDON_H
#define GLOBAL_ADDON_H

	#include <vector>
	#include <string>

	typedef struct lua_State lua_State;
	typedef int (*lua_CFunction) (lua_State *L);

	class Global_addon
	{
		protected:
			static int printf(lua_State *);
			static lua_CFunction sprintf; // We just use Lua's string.format
			static int unpack2(lua_State *);
			static int include(lua_State *);

			static std::vector<std::string> includedList;

		public:
			static int regmod(lua_State *);
	};

#endif
