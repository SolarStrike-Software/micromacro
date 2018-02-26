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

const char *LuaType::metatable_serial_port = "serial_port";

int Serial_port_lua::regmod(lua_State *L)
{
	const luaL_Reg meta[] = {
		{"__gc", Serial_port_lua::gc},
		{"__tostring", Serial_port_lua::tostring},
		{NULL, NULL}
	};

	const luaL_Reg methods[] = {
		{"isConnected", Serial_port_lua::isConnected},
		{"close", Serial_port_lua::close},
		{"read", Serial_port_lua::read},
		{"write", Serial_port_lua::write},
		{NULL, NULL}
	};


	luaL_newmetatable(L, LuaType::metatable_serial_port);
	luaL_setfuncs(L, meta, 0);
	luaL_newlib(L, methods);
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1); // Pop table

	return MicroMacro::ERR_OK;
}

int Serial_port_lua::gc(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	SerialPort *pSerialPort = *static_cast<SerialPort **>(lua_touserdata(L, 1));

	pSerialPort->close();
	return 0;
}

int Serial_port_lua::tostring(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	SerialPort *pSerialPort = *static_cast<SerialPort **>(lua_touserdata(L, 1));

	std::string portName;
	if( pSerialPort->connected )
	{
		portName = "Serial Port ";
		portName += pSerialPort->portName;
	}
	else
	{
		portName = "Disconnected Serial Port";
	}

	lua_pushstring(L, portName.c_str());
	return 1;
}

int Serial_port_lua::cleanup(lua_State *L)
{
	return MicroMacro::ERR_OK;
}

int Serial_port_lua::isConnected(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	SerialPort *pSerialPort = *static_cast<SerialPort **>(lua_touserdata(L, 1));

	lua_pushboolean(L, pSerialPort->connected);
	return 1;
}

int Serial_port_lua::close(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	SerialPort *pSerialPort = *static_cast<SerialPort **>(lua_touserdata(L, 1));

	pSerialPort->close();
	return 0;
}

int Serial_port_lua::read(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	SerialPort *pSerialPort = *static_cast<SerialPort **>(lua_touserdata(L, 1));

	DWORD maxRead = SERIAL_BUFFER_SIZE;
	char buffer[SERIAL_BUFFER_SIZE];
	unsigned int toRead;
	DWORD bytesRead;
	DWORD errors;
	ClearCommError(pSerialPort->handle, &errors, &pSerialPort->status);
	bool moreQueued	=	false;


	// If the user requested to read a specific amount of data, take care of that here
	if( top >= 2 )
	{
		checkType(L, LT_NUMBER, 2);
		maxRead = lua_tointeger(L, 2);
		if( maxRead > SERIAL_BUFFER_SIZE )
			maxRead = SERIAL_BUFFER_SIZE;
	}

	if( pSerialPort->status.cbInQue > 0 )
	{
		if( pSerialPort->status.cbInQue > maxRead )
		{
			toRead		=	maxRead;
			moreQueued	=	true;
		}
		else
			toRead		=	pSerialPort->status.cbInQue;
	}

	bool success = ReadFile(pSerialPort->handle, buffer, toRead, &bytesRead, NULL);

	if( !success )
	{
		return 0;
	}

	lua_pushlstring(L, buffer, bytesRead);
	lua_pushboolean(L, moreQueued);
	return 2;
}


int Serial_port_lua::write(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_STRING | LT_NUMBER, 2);

	SerialPort *pSerialPort = *static_cast<SerialPort **>(lua_touserdata(L, 1));

	size_t sendLen;
	DWORD bytesSent;
	DWORD errors;
	const char *toSend = lua_tolstring(L, 2, &sendLen);

	bool success = WriteFile(pSerialPort->handle, toSend, sendLen, &bytesSent, 0);

	if( !success )
	{
		ClearCommError(pSerialPort->handle, &errors, &pSerialPort->status);
		lua_pushboolean(L, false);
	}
	else
		lua_pushboolean(L, true);

	return 1;
}
