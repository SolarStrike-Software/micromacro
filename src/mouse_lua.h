/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef MOUSE_LUA_H
#define MOUSE_LUA_H

	#define MOUSE_MODULE_NAME		"mouse"
	typedef struct lua_State lua_State;

	class Mouse_lua
	{
		protected:
			static int pressed(lua_State *);
			static int released(lua_State *);
			static int isDown(lua_State *);

			static int press(lua_State *);
			static int hold(lua_State *);
			static int release(lua_State *);
			static int move(lua_State *);
			static int wheelMove(lua_State *);
			static int setPosition(lua_State *);
			static int getPosition(lua_State *);
			static int getConsolePosition(lua_State *);

			static int virtualPress(lua_State *);
			static int virtualHold(lua_State *);
			static int virtualRelease(lua_State *);
			static int virtualMove(lua_State *);
			static int virtualWheelMove(lua_State *);
			static int setVirtualPosition(lua_State *);
			static int getVirtualPosition(lua_State *);

		public:
			static int regmod(lua_State *);
	};

#endif
