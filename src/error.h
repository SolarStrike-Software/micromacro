/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef ERROR_H
#define ERROR_H

	#include <string>
	#include <exception>

	typedef struct lua_State lua_State;

	#define wrongArgs(state)			wrongArgsReal(state, __FUNCTION__)
	#ifdef DISPLAY_DEBUG_MESSAGES
		#define badAllocation()			badAllocationReal(__FILE__, __FUNCTION__, __LINE__)
	#else
		#define badAllocation()			badAllocationReal("", __FUNCTION__, __LINE__)
	#endif

	namespace MicroMacro
	{
		enum ErrCode {
			ERR_OK = 0,
			ERR_UNKNOWN,
			ERR_CLOSE,
			ERR_DOUBLE_INIT,
			ERR_INIT_FAIL,
			ERR_CLEANUP_FAIL,
			ERR_NO_STATE, // Can also indicate errors loading/finding a file
			ERR_RUN,
			ERR_MEM,
			ERR_SYNTAX,
			ERR_FILE,
			ERR_ERR,
			ERR_NOFUNCTION
		};
	}

	enum LuaTypeCheck {
		LT_NIL = 0x1,
		LT_NUMBER = 0x2,
		LT_STRING = 0x4,
		LT_BOOLEAN = 0x8,
		LT_TABLE = 0x10,
		LT_FUNCTION = 0x20,
		LT_THREAD = 0x40,
		LT_USERDATA = 0x80
	};

	void wrongArgsReal(lua_State *, const char *);
	void badAllocationReal(const char *, const char *, int);
	int checkType(lua_State *, int, int);
	int luaL_typerror (lua_State *, int, const char *); // For compatibility with Lua <= 5.1; yes, the spelling is correct
	const char *getErrorString(int);
	std::string getWindowsErrorString(int);

	void pushLuaErrorEvent(lua_State *, const char *, ...);

#endif
