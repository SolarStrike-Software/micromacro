#ifndef SYSTEM_LUA_H
#define SYSTEM_LUA_H

	#define SYSTEM_MODULE_NAME		"system"

	typedef struct lua_State lua_State;

	class System_lua
	{
		protected:
			static int rest(lua_State *);
			static int exec(lua_State *);
			static int getClipboard(lua_State *);
			static int setClipboard(lua_State *);
			static int getActiveCodePage(lua_State *);
			static int getConsoleCodePage(lua_State *);
			static int setPriority(lua_State *);
			static int getConsoleAttributes(lua_State *);
			static int setConsoleAttributes(lua_State *);

		public:
			static int regmod(lua_State *);
	};
#endif
