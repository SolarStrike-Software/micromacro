/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef PROCESS_LUA_H
#define PROCESS_LUA_H

	#include <string>
	#include <vector>
	#include "types.h"
	#include "wininclude.h"

	#define PROCESS_MODULE_NAME			"process"
	#define MEMORY_READ_FAIL			0x00000001 // cannot read memory
	#define MEMORY_WRITE_FAIL			0x00000010 // cannot write memory

	typedef struct lua_State lua_State;

	class Process_lua
	{
		protected:
			// Error strings
			static const char *szInvalidHandleError;
			static const char *szInvalidDataType;

			// Necessary data
			static std::vector<DWORD> attachedThreadIds;

			// Helper functions
			static std::string narrowString(std::wstring);
			static std::string readString(HANDLE, unsigned long, int &, unsigned int);
			static std::wstring readUString(HANDLE, unsigned long, int &, unsigned int);
			static void writeString(HANDLE, unsigned long, char *, int &, unsigned int);

			template <class T>
			static T readMemory(HANDLE process, unsigned long address, int &err)
			{
				T buffer;
				SIZE_T bytesread = 0;
				err = 0;
				int success = 0;

				success = ReadProcessMemory(process, (LPCVOID)address,
				(void *)&buffer, sizeof(T), &bytesread);

				if( success == 0 )
					err = MEMORY_READ_FAIL;

				return buffer;
			}

			template <class T>
			static void writeMemory(HANDLE process, unsigned long address, T data, int &err)
			{
				SIZE_T byteswritten = 0;
				err = 0;
				int success = 0;
				DWORD old;

				VirtualProtectEx(process, (void *)address, sizeof(T), PAGE_READWRITE, &old);
				success = WriteProcessMemory(process, (void *)address,
				(void*)&data, sizeof(T), &byteswritten);
				VirtualProtectEx(process, (void *)address, sizeof(T), old, &old);

				if( success == 0 )
					err = MEMORY_WRITE_FAIL;
			}

			static unsigned int readBatch_parsefmt(const char *, std::vector<BatchJob> &);
			static bool procDataCompare(const char *, const char *, const char *);

			// Actual Lua functions
			static int open(lua_State *);
			static int close(lua_State *);
			static int read(lua_State *);
			static int readPtr(lua_State *);
			static int readBatch(lua_State *);
			static int readChunk(lua_State *);
			static int write(lua_State *);
			static int writePtr(lua_State *);
			static int findPattern(lua_State *);
			static int findByWindow(lua_State *);
			static int findByExe(lua_State *);
			static int getModuleAddress(lua_State *);
			static int attachInput(lua_State *);
			static int detachInput(lua_State *);

		public:
			static int regmod(lua_State *);
			static int cleanup(lua_State *);
	};
#endif
