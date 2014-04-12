#ifndef GAMEPAD_H
#define GAMEPAD_H

	#define GAMEPAD_MODULE_NAME				"gamepad"

	typedef struct lua_State lua_State;

	class Gamepad_lua
	{
		protected:
			static int pressed(lua_State *);
			static int released(lua_State *);
			static int isDown(lua_State *);
			static int getPOV(lua_State *);
			static int getAxis(lua_State *);
			static int getCount(lua_State *);

			/*static int press(lua_State *);
			static int hold(lua_State *);
			static int release(lua_State *);*/

		public:
			void poll();
			static int regmod(lua_State *);
	};

#endif
