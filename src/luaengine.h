/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef LUAENGINE_H
#define LUAENGINE_H

	#include "error.h"
	#include "timer.h"
	#include <string>
	#include <vector>

	namespace MicroMacro
	{
		class Event;
	}

	typedef struct lua_State lua_State;

	#define MACRO_TABLE_NAME					"macro"
	#define MACRO_INIT_NAME						"init"
	#define MACRO_MAIN_NAME						"main"
	#define MACRO_EVENT_NAME					"event"
	#define	MAX_WINDOWS_MESSAGES_PER_CYCLE		100

	class LuaEngine
	{
		protected:
			lua_State *lstate;
			static int _macrotab_init(lua_State *);
			static int _macrotab_main(lua_State *);
			static int _macrotab_event(lua_State *);
			static int _macrotab_getVersion(lua_State *);
			static int err_msgh(lua_State *);

			static int is64bit(lua_State *);
			static int is32bit(lua_State *);

			void stdError();

			std::string basePath;
			std::string lastErrorMsg;
			TimeType lastTimestamp;			// Holds the timestamp so we can compute delta time
			float fDeltaTime;				// Holds the time elapsed between last cycle and current logic cycle
			int keyHookErrorState;

		public:
			LuaEngine() : lstate(NULL), lastErrorMsg(""), fDeltaTime(0), keyHookErrorState(MicroMacro::ERR_OK) { };
			~LuaEngine();

			int init();
			int reinit();
			int cleanup();
			int loadFile(const char *);
			int loadString(const char *);

			int runInit(std::vector<std::string> * = NULL);
			int runMain();
			int runEvent(MicroMacro::Event &);
			int dispatchWindowsMessages();

			float getDeltaTime();
			std::string getLastErrorMessage();
			void setLastErrorMessage(const char *);
			lua_State *getLuaState();

			std::string getBasePath();
			void setBasePath(std::string);

			int getKeyHookErrorState();
			void setKeyHookErrorState(int);
	};


#endif
