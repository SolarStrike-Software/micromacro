#include "serial_lua.h"
#include "serial_port_lua.h"
#include "types.h"
#include "error.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

using MicroMacro::SerialPort;


int Serial_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"open", Serial_lua::open},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, SERIAL_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

int Serial_lua::cleanup(lua_State *L)
{
	return MicroMacro::ERR_OK;
}

int Serial_lua::open(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	char *portName	=	(char *)lua_tostring(L, 1);
	int baud		=	9600;

	if( top >= 2 )
	{
		checkType(L, LT_NUMBER, 2);
		baud		=	(int)lua_tointeger(L, 2);
	}

	SerialPort *pSerialPort = new SerialPort;
	SerialPort **ppSerialPort = static_cast<SerialPort **>(lua_newuserdata(L, sizeof(SerialPort **)));
	*ppSerialPort = pSerialPort;

	pSerialPort->open(portName, baud);

	// It was created, so give it a metatable
	luaL_getmetatable(L, LuaType::metatable_serial_port);
	lua_setmetatable(L, -2);

	return 1;
}
