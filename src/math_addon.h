/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef MATH_ADDON_H
#define MATH_ADDON_H

	#define MATH_MODULE_NAME			"math"
	typedef struct lua_State lua_State;

	class Math_addon
	{
		protected:
			static int distance(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
