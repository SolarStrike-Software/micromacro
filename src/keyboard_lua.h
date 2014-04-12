#ifndef KEYBOARD_LUA_H
#define KEYBOARD_LUA_H

	#define KEYBOARD_MODULE_NAME		"keyboard"
	typedef struct lua_State lua_State;

	class Keyboard_lua
	{
		protected:
			static int pressed(lua_State *);
			static int released(lua_State *);
			static int isDown(lua_State *);
			static int getToggleState(lua_State *);

			static int press(lua_State *);
			static int hold(lua_State *);
			static int release(lua_State *);

			static int virtualPress(lua_State *);
			static int virtualHold(lua_State *);
			static int virtualRelease(lua_State *);

		public:
			static int regmod(lua_State *);
	};
#endif
