/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifdef NETWORKING_ENABLED

#include "socket_lua.h"
#include "error.h"
#include "macro.h"
#include "types.h"
#include "strl.h"
#include "logger.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <ws2tcpip.h>
#include "macro.h"
#include "settings.h"

const char *LuaType::metatable_socket = "socket";

HANDLE Socket_lua::socketListLock = NULL;
std::vector<Socket *> Socket_lua::socketList;

DWORD WINAPI Socket_lua::socketThread(SOCKET socket)
{
	char readBuff[Macro::instance()->getSettings()->getInt(CONFVAR_NETWORK_BUFFER_SIZE) + 1];

	while(true)
	{
		int result = recv(socket, readBuff, sizeof(readBuff)-1, 0);

		if( result > 0 )
		{ // Data received
			// Copy the received data into an event; push it.
			readBuff[result] = 0; // Enforce NULL-terminator
			Event e;
			e.idata1 = (int)socket;
			e.type = EVENT_SOCKETRECEIVED;
			e.msg = std::string(readBuff, result);
			Macro::instance()->pushEvent(e);
		}
		else if( result == 0 )
		{ // Connection closed (probably by remote)
			Event e;
			e.idata1 = (int)socket;
			e.type = EVENT_SOCKETDISCONNECTED;
			Macro::instance()->pushEvent(e);
			break;
		}
		else
		{ // Error occurred
			int errCode = WSAGetLastError();
			switch(errCode)
			{
				case 10038: // "Not a socket"; we closed the socket in the main thread, so this signals we should shut down this thread
				case 10054: // CONNRESET; remote side closed the connection forcibly
				{
					Event e;
					e.idata1 = (int)socket;
					e.type = EVENT_SOCKETDISCONNECTED;
					Macro::instance()->pushEvent(e);
				}
				break;

				default:
				#ifdef DISPLAY_DEBUG_MESSAGES
					printf("Socket error occurred. Code: %d, socket: 0x%X\n", errCode, socket);
				#endif
				{
					Event e;
					e.idata1 = (int)socket;
					e.idata2 = errCode;
					e.type = EVENT_SOCKETERROR;
					Macro::instance()->pushEvent(e);
				}
				break;
			}
			break;
		}
	}

	// Remove it from socket list.
	DWORD dwWaitResult = WaitForSingleObject(socketListLock, INFINITE);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
			for(unsigned int i = 0; i < socketList.size(); i++)
			{
				Socket *pSocket = socketList.at(i);
				if( pSocket->socket == socket )
				{
					socketList.erase(socketList.begin()+i);
					memset(pSocket, 0, sizeof(Socket));
					break;
				}
			}

			if( !ReleaseMutex(socketListLock) )
			{ // Uh oh... That's not good.
				char errBuff[1024];
				slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
					"Socket_lua", __FUNCTION__);
				fprintf(stderr, errBuff);
				Logger::instance()->add(errBuff);
			}
		break;

		case WAIT_ABANDONED: // TODO: What should we do here? Error?
		break;
	}

	return 1;
}

DWORD WINAPI Socket_lua::listenThread(SOCKET socket)
{
	SOCKET new_socket;
	struct sockaddr_in client;

	while(true)
	{
		int addrlen = sizeof(struct sockaddr_in);
		new_socket = accept(socket, (struct sockaddr *)&client, &addrlen);

		if( new_socket == INVALID_SOCKET )
		{
			int errCode = WSAGetLastError();
			switch(errCode)
			{
				case 10038: // "Not a socket"; we closed the socket in the main thread, so this signals we should shut down this thread
				case 10053: // CONNABORTED; client side closed the connection forcibly
				case 10054: // CONNRESET; remote side closed the connection forcibly
				{
					Event e;
					e.idata1 = (int)socket;
					e.type = EVENT_SOCKETDISCONNECTED;
					Macro::instance()->pushEvent(e);
				}
				break;

				case 10004: // Interrupted blocking function call (probably termniated a listen thread on script end)
				break;

				default:
				#ifdef DISPLAY_DEBUG_MESSAGES
					printf("Socket error occurred. Code: %d, listen socket: 0x%X\n", errCode, socket);
				#endif
				{
					Event e;
					e.idata1 = (int)socket;
					e.idata2 = errCode;
					e.type = EVENT_SOCKETERROR;
					Macro::instance()->pushEvent(e);
				}
				break;
			}
			break;
		}
		else
		{ // Successfully accepted new client
			Event e;
			e.socket.socket = new_socket;
			e.socket.port = 0;				// TODO: We need to get this somehow
			e.socket.protocol = AF_INET;

			// Start a thread for this socket
			e.socket.hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socketThread, (PVOID)new_socket, 0, NULL);

			e.type = EVENT_SOCKETCONNECTED;
			Macro::instance()->pushEvent(e);
		}
	}

	// Remove it from socket list.
	DWORD dwWaitResult = WaitForSingleObject(socketListLock, INFINITE);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
			for(unsigned int i = 0; i < socketList.size(); i++)
			{
				Socket *pSocket = socketList.at(i);
				if( pSocket->socket == socket )
				{
					socketList.erase(socketList.begin()+i);
					memset(pSocket, 0, sizeof(Socket));
					break;
				}
			}

			if( !ReleaseMutex(socketListLock) )
			{ // Uh oh... That's not good.
				char errBuff[1024];
				slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
					"Socket_lua", __FUNCTION__);
				fprintf(stderr, errBuff);
				Logger::instance()->add(errBuff);
			}
		break;

		case WAIT_ABANDONED: // TODO: What should we do here? Error?
		break;
	}

	return 1;
}

int Socket_lua::regmod(lua_State *L)
{
	const luaL_Reg meta[] = {
		{"__gc", gc},
		{"__tostring", tostring},
		{NULL, NULL}
	};

	const luaL_Reg methods[] = {
		{"connect", connect},
		{"listen", listen},
		{"send", send},
		{"close", close},
		{"id", id},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LuaType::metatable_socket);
	luaL_setfuncs(L, meta, 0);
	luaL_newlib(L, methods);
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1); // Pop table


	socketListLock = CreateMutex(NULL, FALSE, NULL);
	if( !socketListLock )
	{
		Logger::instance()->add("CreateMutex() failed in Socket_lua::regmod(); err code: %d", GetLastError());
		return MicroMacro::ERR_ERR;
	}

	return MicroMacro::ERR_OK;
}

int Socket_lua::cleanup()
{
	// Close down all sockets & threads
	DWORD dwWaitResult = WaitForSingleObject(socketListLock, INFINITE);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
			for(unsigned int i = 0; i < socketList.size(); i++)
			{
				Socket *pSocket = socketList.at(i);

				// Close socket, let the thread take care of cleanup
				closesocket(pSocket->socket);
			}
			socketList.clear();

			if( !ReleaseMutex(socketListLock) )
			{ // Uh oh... That's not good.
				char errBuff[1024];
				slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
					"Socket_lua", __FUNCTION__);
				fprintf(stderr, errBuff);
				Logger::instance()->add(errBuff);
			}
		break;

		case WAIT_ABANDONED: // TODO: What should we do here? Error?
		break;
	}

	// Now we can close our mutex
	CloseHandle(socketListLock);
	socketListLock = NULL;

	return MicroMacro::ERR_OK;
}

// TODO: This is completely broken...
bool Socket_lua::isIP(const char *ip)
{
	struct addrinfo *result;
	int success = getaddrinfo(ip, NULL, NULL, &result);

	//printf("getaddrinfo success: %d\n", success);

	return (success != 0);
}

int Socket_lua::connect(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 3 )
		wrongArgs(L);
	checkType(L, LT_STRING, 2);
	checkType(L, LT_NUMBER, 3);

	Socket *pSocket = static_cast<Socket *>(lua_touserdata(L, 1));
	const char *host = lua_tostring(L, 2);
	int port = lua_tointeger(L, 3);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

/*	if( isIP(host) )
	{
		printf("Use raw IP\n");
		server.sin_addr.s_addr = inet_addr(host);
	}
	else*/
	{
		//printf("Look up hostname\n");
		HOSTENT *pHostent;
		pHostent = gethostbyname(host);
		server.sin_addr = *((LPIN_ADDR)*pHostent->h_addr_list);
	}


	int success = (::connect(pSocket->socket, (struct sockaddr *)&server, sizeof(struct sockaddr)) >= 0);

	if( !success )
	{
		char errbuff[2048];
		slprintf(errbuff, sizeof(errbuff), "Connection failed. Err code %d\n", WSAGetLastError());
		lua_pushboolean(L, false);
		lua_pushstring(L, errbuff);
		return 2;
	}

	pSocket->port = port;

	// Start a thread for this socket
	pSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socketThread, (PVOID)pSocket->socket, 0, NULL);

	// Lets make a record of it in our list
	DWORD dwWaitResult = WaitForSingleObject(socketListLock, INFINITE);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
			socketList.push_back(pSocket);

			if( !ReleaseMutex(socketListLock) )
			{ // Uh oh... That's not good.
				char errBuff[1024];
				slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
					"Socket_lua", __FUNCTION__);
				fprintf(stderr, errBuff);
				Logger::instance()->add(errBuff);
			}
		break;

		case WAIT_ABANDONED: // TODO: What should we do here? Error?
		break;
	}

	lua_pushboolean(L, true);
	return 1;
}

int Socket_lua::listen(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 3 )
		wrongArgs(L);
	checkType(L, LT_STRING, 2);
	checkType(L, LT_NUMBER, 3);

	Socket *pSocket = static_cast<Socket *>(lua_touserdata(L, 1));
	const char *host = lua_tostring(L, 2);
	int port = lua_tointeger(L, 3);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	/*if( isIP(host) )
	{
		printf("Use raw IP\n");
		server.sin_addr.s_addr = inet_addr(host);
	}*/
	if( strlen(host) == 0 )
	{
		server.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		//printf("Look up hostname\n");
		HOSTENT *pHostent;
		pHostent = gethostbyname(host);
		server.sin_addr = *((LPIN_ADDR)*pHostent->h_addr_list);
	}


	int success = (::bind(pSocket->socket, (struct sockaddr *)&server, sizeof(struct sockaddr)) >= 0);

	if( !success )
	{
		char errbuff[2048];
		slprintf(errbuff, sizeof(errbuff), "Bind failed. Err code %d\n", WSAGetLastError());
		lua_pushboolean(L, false);
		lua_pushstring(L, errbuff);
		return 2;
	}

	::listen(pSocket->socket, LISTEN_BUFFER);

	pSocket->port = port;

	// Start a thread for this socket
	pSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)listenThread, (PVOID)pSocket->socket, 0, NULL);

	// Lets make a record of it in our list
	DWORD dwWaitResult = WaitForSingleObject(socketListLock, INFINITE);
	switch(dwWaitResult)
	{
		case WAIT_OBJECT_0:
			socketList.push_back(pSocket);

			if( !ReleaseMutex(socketListLock) )
			{ // Uh oh... That's not good.
				char errBuff[1024];
				slprintf(errBuff, sizeof(errBuff)-1, "Unable to ReleaseMutex() in %s:%s()\n",
					"Socket_lua", __FUNCTION__);
				fprintf(stderr, errBuff);
				Logger::instance()->add(errBuff);
			}
		break;

		case WAIT_ABANDONED: // TODO: What should we do here? Error?
		break;
	}

	lua_pushboolean(L, true);
	return 1;
}

int Socket_lua::send(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER | LT_STRING, 2);

	Socket *pSocket = static_cast<Socket *>(lua_touserdata(L, 1));
	size_t len;
	const char *msg = lua_tolstring(L, 2, &len);

	int success = ::send(pSocket->socket, msg, len, 0);
	if( success < 0 )
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	return 1;
}

int Socket_lua::close(lua_State *L)
{
	int top = lua_gettop(L);

	Socket *pSocket = static_cast<Socket *>(lua_touserdata(L, 1));
	if( pSocket->socket == 0 )
	{ // Nothing to do here. Move alone.
		return 0;
	}
	// Close the socket; let the thread take care of cleanup
	closesocket(pSocket->socket);

	return 0;
}

int Socket_lua::id(lua_State *L)
{
	Socket *pSocket = static_cast<Socket *>(lua_touserdata(L, 1));
	lua_pushinteger(L, pSocket->socket);
	return 1;
}

int Socket_lua::gc(lua_State *L)
{
	close(L);
	return 0;
}

int Socket_lua::tostring(lua_State *L)
{
	Socket *pSocket = static_cast<Socket *>(lua_touserdata(L, 1));
	char buffer[64];
	slprintf(buffer, sizeof(buffer), "Socket (0x%X)", pSocket->socket);

	lua_pushstring(L, buffer);
	return 1;
}

#endif
