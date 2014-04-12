#ifndef LUATYPES_H
#define LUATYPES_H

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

	typedef union _LARGE_INTEGER LARGE_INTEGER;
	//typedef struct lua_State lua_State;
	//typedef struct luaL_Reg luaL_Reg;

	namespace LuaType
	{
		extern const char *metatable_int64;
		extern const char *metatable_ncursesWindow;
		extern const char *metatable_handle;
		//extern const char *metatable_windowDC;
		extern const char *metatable_audioResource;
		extern const char *metatable_vector2d;

		extern const char *highpart_name;
		extern const char *lowpart_name;

		int int64_add(lua_State *);
		int int64_subtract(lua_State *);
		int int64_multiply(lua_State *);
		int int64_divide(lua_State *);
		int int64_eq(lua_State *);
		int int64_lt(lua_State *);
		int int64_gt(lua_State *);
		int int64_tostring(lua_State *);

		int ncursesWindow_gc(lua_State *);
		int ncursesWindow_tostring(lua_State *);

		int handle_gc(lua_State *);
		int handle_tostring(lua_State *);

		/*int windowDC_gc(lua_State *);
		int windowDC_tostring(lua_State *);*/

		int audioResource_gc(lua_State *);
		int audioResource_tostring(lua_State *);

		int vector2d_tostring(lua_State *);
		int vector2d_add(lua_State *);
		int vector2d_sub(lua_State *);
		int vector2d_mul(lua_State *);
		int vector2d_div(lua_State *);
		int vector2d_set(lua_State *);
		int vector2d_length(lua_State *);

		//extern const luaL_Reg vector2d_methods[];
		const luaL_Reg vector2d_methods[] = {
			{"set", vector2d_set},
			{"length", vector2d_length},
			{NULL, NULL}
		};
	}

	int registerLuaTypes(lua_State *);
	LARGE_INTEGER lua_toint64(lua_State *, int);
	bool lua_isint64(lua_State *, int);
#endif
