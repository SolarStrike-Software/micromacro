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
#include "debugmessages.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <ws2tcpip.h>
#include "macro.h"
#include "settings.h"

#define DEFAULT_LOCK_TIMEOUT		1000

const char *LuaType::metatable_socket = "socket";

Mutex Socket_lua::socketListLock;
std::vector<Socket *> Socket_lua::socketList;

DWORD WINAPI Socket_lua::socketThread(Socket *pSocket)
{
	int buffSize = Macro::instance()->getSettings()->getInt(CONFVAR_NETWORK_BUFFER_SIZE);
	char *readBuff = new char[buffSize+1];

	int maxRecvQueueSize = Macro::instance()->getSettings()->getInt(CONFVAR_RECV_QUEUE_SIZE);
	while(true)
	{
		int result = ::recv(pSocket->socket, readBuff, buffSize, 0);

		if( result > 0 )
		{ // Data received
			// Copy the received data into an event; push it.
			std::string msg = std::string(readBuff, result);
			readBuff[result] = 0; // Enforce NULL-terminator
			Event e;
			e.idata1 = (int)pSocket->socket;
			e.type = EVENT_SOCKETRECEIVED;
			e.msg = msg;

			if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
			{
				while( pSocket->recvQueue.size() > (maxRecvQueueSize + 1) )
					pSocket->recvQueue.pop();

				pSocket->eventQueue.push(e);
				pSocket->recvQueue.push(msg);
				pSocket->mutex.unlock();
			}
		}
		else if( result == 0 )
		{ // Connection closed (probably by remote)
			Event e;
			e.idata1 = (int)pSocket->socket;
			e.type = EVENT_SOCKETDISCONNECTED;

			if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
			{
				pSocket->eventQueue.push(e);
				pSocket->mutex.unlock();
			}
			break;
		}
		else
		{ // Error occurred
			int errCode = WSAGetLastError();
			pSocket->connected = false;

			switch(errCode)
			{
				case 10038: // "Not a socket"; we closed the socket in the main thread, so this signals we should shut down this thread
				case 10054: // CONNRESET; remote side closed the connection forcibly
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.type = EVENT_SOCKETDISCONNECTED;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
					{
						pSocket->eventQueue.push(e);
						pSocket->mutex.unlock();
					}
				}
				break;

				case 10053:	// Software caused connection abort. (your machine); we should close the socket
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.idata2 = errCode;
					e.type = EVENT_SOCKETERROR;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
					{
						pSocket->eventQueue.push(e);
						closesocket(pSocket->socket);
						pSocket->connected = true;
						pSocket->mutex.unlock();
					}
				}
				break;

				default:
				#ifdef DISPLAY_DEBUG_MESSAGES
					fprintf(stderr, "Socket error occurred. Code: %d, socket: 0x%X\n", errCode, pSocket->socket);
				#endif
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.idata2 = errCode;
					e.type = EVENT_SOCKETERROR;
					//Macro::instance()->pushEvent(e);
					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
					{
						pSocket->eventQueue.push(e);
						pSocket->mutex.unlock();
					}
				}
				break;
			}
			break;
		}
	}

	delete []readBuff;

	return 1;
}

DWORD WINAPI Socket_lua::listenThread(Socket *pSocket)
{
	// Set the socket to listen mode
	::listen(pSocket->socket, LISTEN_BUFFER);

	SOCKET new_socket;
	struct sockaddr_in client;

	while(true)
	{
		int addrlen = sizeof(struct sockaddr_in);
		new_socket = accept(pSocket->socket, (struct sockaddr *)&client, &addrlen);

		if( new_socket == INVALID_SOCKET )
		{
			int errCode = WSAGetLastError();
			pSocket->connected = false;
			switch(errCode)
			{
				case 10038: // "Not a socket"; we closed the socket in the main thread, so this signals we should shut down this thread
				case 10054: // CONNRESET; remote side closed the connection forcibly
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.type = EVENT_SOCKETDISCONNECTED;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
					{
						pSocket->eventQueue.push(e);
						pSocket->mutex.unlock();
					}
				}
				break;

				case 10053:	// Software caused connection abort. (your machine); we should close the socket
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.idata2 = errCode;
					e.type = EVENT_SOCKETERROR;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
					{
						pSocket->eventQueue.push(e);
						closesocket(pSocket->socket);
						pSocket->connected = true;
						pSocket->mutex.unlock();
					}
				}
				break;

				case 10004: // Interrupted blocking function call (probably terminated a listen thread on script end)
				break;

				default:
				#ifdef DISPLAY_DEBUG_MESSAGES
					fprintf(stderr, "Socket error occurred. Code: %d, listen socket: 0x%X\n", errCode, pSocket->socket);
				#endif
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.idata2 = errCode;
					e.type = EVENT_SOCKETERROR;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
					{
						pSocket->eventQueue.push(e);
						pSocket->mutex.unlock();
					}
				}
				break;
			}
			break;
		}
		else
		{ // Successfully accepted new client
			Socket *pSocket = new Socket;

			// Push the event
			Event e;
			e.type = EVENT_SOCKETCONNECTED;
			e.pSocket = pSocket;
			pSocket->eventQueue.push(e);

			/* Now we can record some more info and start the new thread
				Note: We don't need to mutex this because it cannot be accessed
				until after we create the thread.
			*/
			pSocket->socket = new_socket;
			pSocket->protocol = AF_INET;
			pSocket->connected = true;
			pSocket->open = true;

			pSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socketThread, (PVOID)pSocket, 0, NULL);

			if( socketListLock.lock() )
			{
				socketList.push_back(pSocket);
				socketListLock.unlock();
			}
		}
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
		{"recv", recv},
		{"close", close},
		{"id", id},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LuaType::metatable_socket);
	luaL_setfuncs(L, meta, 0);
	luaL_newlib(L, methods);
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1); // Pop table

	return MicroMacro::ERR_OK;
}

int Socket_lua::cleanup()
{
	if( socketListLock.lock() )
	{
		for(SocketListIterator i = socketList.begin(); i != socketList.end(); ++i)
		{
			Socket *pSocket = *i;
			if( pSocket->mutex.lock() )
			{
				// Close the socket if needed
				if( pSocket->open )
					closesocket(pSocket->socket);

				pSocket->open = false;
				pSocket->connected = false;
				pSocket->mutex.unlock();
			}
		}
		socketList.clear();
		socketListLock.unlock();
	}

	return MicroMacro::ERR_OK;
}

// TODO: This is completely broken...
bool Socket_lua::isIP(const char *ip)
{
	struct addrinfo *result;
	int success = getaddrinfo(ip, NULL, NULL, &result);

	return (success != 0);
}

int Socket_lua::connect(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 3 )
		wrongArgs(L);
	checkType(L, LT_STRING, 2);
	checkType(L, LT_NUMBER, 3);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
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

	if( !pSocket->mutex.lock() )
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, "Could not lock socket mutex.");
		return 2;
	}

	if( pSocket->connected || pSocket->open )
	{ // Socket already in use; cannot do this.
		pSocket->mutex.unlock();
		lua_pushboolean(L, false);
		lua_pushstring(L, "Socket already connected; cannot open a new connection. Use a new socket.\n");
		return 2;
	}


	int success = (::connect(pSocket->socket, (struct sockaddr *)&server, sizeof(struct sockaddr)) >= 0);

	if( !success )
	{
		pSocket->mutex.unlock();
		char errbuff[2048];
		slprintf(errbuff, sizeof(errbuff), "Connection failed. Err code %d\n", WSAGetLastError());
		lua_pushboolean(L, false);
		lua_pushstring(L, errbuff);
		return 2;
	}

	// Start a thread for this socket
	pSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socketThread, (PVOID)pSocket, 0, NULL);
	pSocket->connected = true;
	pSocket->open = true;

	pSocket->mutex.unlock();

	// Lets make a record of it in our list
	if( socketListLock.lock(DEFAULT_LOCK_TIMEOUT) )
	{
		socketList.push_back(pSocket);
		socketListLock.unlock();
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

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
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

	if( !pSocket->mutex.lock() )
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, "Could not lock socket mutex.");
		return 2;
	}

	if( pSocket->connected || pSocket->open )
	{ // Socket already in use; cannot do this.
		pSocket->mutex.unlock();
		lua_pushboolean(L, false);
		lua_pushstring(L, "Socket already connected; cannot set to listen. Use a new socket.\n");
		return 2;
	}

	int success = (::bind(pSocket->socket, (struct sockaddr *)&server, sizeof(struct sockaddr)) >= 0);

	if( !success )
	{
		pSocket->mutex.unlock();
		char errbuff[2048];
		slprintf(errbuff, sizeof(errbuff), "Bind failed. Err code %d\n", WSAGetLastError());
		lua_pushboolean(L, false);
		lua_pushstring(L, errbuff);
		return 2;
	}

	// Start a thread for this socket
	pSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)listenThread, (PVOID)pSocket, 0, NULL);
	pSocket->connected = true;
	pSocket->open = true;

	pSocket->mutex.unlock();

	// Lets make a record of it in our list
	if( socketListLock.lock(DEFAULT_LOCK_TIMEOUT) )
	{
		socketList.push_back(pSocket);
		socketListLock.unlock();
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

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	size_t len;
	const char *msg = lua_tolstring(L, 2, &len);

	if( !pSocket->mutex.lock() )
	{
		lua_pushboolean(L, false);
		return 1;
	}

	if( !pSocket->connected || !pSocket->open )
	{ // Cannot send on a closed socket.
		pSocket->mutex.unlock();
		lua_pushboolean(L, false);
		return 1;
	}

	int success = ::send(pSocket->socket, msg, len, 0);
	pSocket->mutex.unlock();
	if( success < 0 )
	{
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	return 1;
}

int Socket_lua::recv(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	int retVal = 0;
	if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT) )
	{
		if( !pSocket->recvQueue.empty() )
		{
			lua_pushlstring(L, pSocket->recvQueue.front().c_str(), pSocket->recvQueue.front().size());
			pSocket->recvQueue.pop();
			retVal = 1;
		}

		pSocket->mutex.unlock();
	}

	return retVal;
}

int Socket_lua::id(lua_State *L)
{
	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	lua_pushinteger(L, pSocket->socket);
	return 1;
}

int Socket_lua::close(lua_State *L)
{
	int top = lua_gettop(L);
	if( top < 1 )
		wrongArgs(L);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	if( pSocket->mutex.lock() )
	{
		if( pSocket->open )
			closesocket(pSocket->socket);

		pSocket->connected = false;
		pSocket->open = false;
		pSocket->mutex.unlock();
	}


	return 0;
}

int Socket_lua::gc(lua_State *L)
{
	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	close(L);

	/* Iterate through the list and erase the socket */
	if( socketListLock.lock() )
	{
		for(SocketListIterator i = socketList.begin(); i != socketList.end(); ++i)
		{
			if( *i == pSocket )
			{
				if( pSocket->mutex.lock() )
				{
					pSocket->connected = false;
					pSocket->open = false;
					socketList.erase(i);
					pSocket->mutex.unlock();
				}

				break;
			}
		}
		socketListLock.unlock();
	}
	return 0;
}

int Socket_lua::tostring(lua_State *L)
{
	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	char buffer[64];
	slprintf(buffer, sizeof(buffer), "Socket (0x%X)", pSocket->socket);

	lua_pushstring(L, buffer);
	return 1;
}

#endif
