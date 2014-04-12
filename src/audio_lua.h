/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef AUDIO_LUA_H
#define AUDIO_LUA_H

	#define AUDIO_MODULE_NAME			"audio"

	typedef struct lua_State lua_State;

	class Audio_lua
	{
		protected:
			static int load(lua_State *);
			static int unload(lua_State *);
			static int play(lua_State *);
			static int stop(lua_State *);
			static int pause(lua_State *);
			static int getState(lua_State *);
			static int setLooping(lua_State *);
			static int setVolume(lua_State *);

		public:
			static int regmod(lua_State *);
			static int cleanup(lua_State *);
	};
#endif
