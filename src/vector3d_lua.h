/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef VECTOR3D_LUA_H
#define VECTOR3D_LUA_H

	typedef struct lua_State lua_State;
	typedef struct Vector3d Vector3d;

	namespace LuaType
	{
		extern const char *metatable_vector3d;
	}

	class Vector3d_lua
	{
		protected:
			static int tostring(lua_State *);
			static int add(lua_State *);
			static int sub(lua_State *);
			static int mul(lua_State *);
			static int div(lua_State *);
			static int set(lua_State *);
			static int length(lua_State *);
			static int normal(lua_State *);
			static int rotateAboutX(lua_State *);
			static int rotateAboutY(lua_State *);
			static int rotateAboutZ(lua_State *);
			static int rotateAbout(lua_State *);

		public:
			static int regmod(lua_State *);
	};

	Vector3d lua_tovector3d(lua_State *, int );
	void lua_pushvector3d(lua_State *, Vector3d &);

#endif
