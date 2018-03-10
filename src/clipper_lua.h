/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef CLIPPER_LUA_H
#define CLIPPER_LUA_H

	#include "wininclude.h"

	#include "clipper/clipper.hpp"
	#define CLIPPER_MODULE_NAME		"clipper"
	typedef struct lua_State lua_State;

	class Clipper_lua
	{
		protected:
			static int getPointPrecision(lua_State *);
			static int setPointPrecision(lua_State *);
			static int offset(lua_State *);
			static int merge(lua_State *);
			static int pointInPoly(lua_State *);
			static int clean(lua_State *);
			static int simplify(lua_State *);

			static double pointPrecision;	// Used for scaling since Clipper uses ints to store points; should be power of two to make scaling lossless

			// Helpers to scale point coordinates for us
			static int scalePointToLong(double, double, long &, long &);
			static int scalePointToDouble(long, long, double &, double &);

			// Helpers to return solutions
			static int pushPathSolution(lua_State *, ClipperLib::Path &);
			static int pushPathsSolution(lua_State *, ClipperLib::Paths &);

		public:
			static int regmod(lua_State *);
			static int cleanup(lua_State *);

	};
#endif
