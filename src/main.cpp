/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "wininclude.h"		// Be sure to include this *before* ncurses!
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <queue>
#include <time.h>
#include <iostream>
#include <conio.h>
#include <Shlwapi.h>

#include "macro.h"
#include "ncurses_lua.h"
#include "network_lua.h"
#include "filesystem.h"
#include "logger.h"
#include "timer.h"
#include "event.h"
#include "strl.h"
#include "rng.h"
#include "version.h"
#include "os.h"

#include "encstring.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include "debugmessages.h"

const int GAMEPAD_REPOLL_SECONDS = 10;
char baseDirectory[MAX_PATH+1];
std::string previousScript;

std::string scriptGUIDialog(std::string);
std::string promptForScript();
void splitArgs(std::string cmd, std::vector<std::string> &);
std::string autoAdjustScriptFilename(std::string);
double getConfigFloat(lua_State *, const char *, double);
int getConfigInt(lua_State *, const char *, int);
std::string getConfigString(lua_State *, const char *, std::string);
int loadConfig(const char *);
void printStdHead();
void openLog();
void deleteOldLogs(const char *, unsigned int);
void clearCliScreen();
static BOOL WINAPI consoleControlCallback(DWORD);

#ifdef NETWORKING_ENABLED
WSADATA wsadata;
#endif


INT WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int nShow)
//int main(int argc, char **argv)			// See notes below
{
	/* We emulate the standard C/C++ main() function in this way.
		This code exists to keep compatibility simple.

		If you simply comment these lines out and restore main()
		declaration to "normal," you're good to go.
	*/
	int argc = 1;
	char *argv[2];

	char filename[MAX_PATH];
	GetModuleFileName( NULL, filename, MAX_PATH );
	argv[0] = filename;
	argv[1] = cmdLine;
	/* END: main() compatibility */


	bool running;
	SetConsoleCtrlHandler(consoleControlCallback, true);
	printStdHead(); // Intro text output

	// Extract MicroMacro's base path from argv[0]
	GetFullPathName(argv[0], MAX_PATH, baseDirectory, NULL);
	strlcpy(baseDirectory,
		fixSlashes(getFilePath(baseDirectory, true), SLASHES_TO_WINDOWS).c_str(), MAX_PATH);

	/* Copy the base path into the Lua engine; Do this *before* initializing! */
	Macro::instance()->getEngine()->setBasePath(baseDirectory);
	{	/* Run configs */
		std::string configFilename = appendToPath(baseDirectory, CONFIG_FILENAME);
		configFilename = fixSlashes(configFilename, SLASHES_TO_STANDARD);

		if( !fileExists(configFilename.c_str()) )
		{ // Lets try to copy it from default.config.lua, if it exists.
			std::string defaultConfigFilename = appendToPath(baseDirectory, CONFIG_DEFAULT_FILENAME);

			if( fileExists(defaultConfigFilename.c_str()) )
                copyFile(defaultConfigFilename.c_str(), configFilename.c_str());
			/* NOTE: If we want to do regex and replace some config stuff here,
				we should replace copyFile() with a line-by-line read/replace/write function.
				copyFile() is a straight binary copy.
			*/
		}


		int success = loadConfig(configFilename.c_str());
		if( success != MicroMacro::ERR_OK )
		{
			fprintf(stderr, "Failed loading config file. Err code: %d (%s)\n",
				success, getErrorString(success));
			system("pause");
			return success;
		}
	}

	{	/* Initiate our macro singleton */
		int success;
		success = Macro::instance()->init();
		if( success != MicroMacro::ERR_OK )
		{
			Logger::instance()->add("Failed to initiate Macro singleton; Error code: %d (%s)\n",
				success, getErrorString(success));
			return success;
		}
	}

	/* Set debug privileges on self */
	if( !OS::modifyPermission(GetCurrentProcess(), "SeDebugPrivilege", true) )
		Logger::instance()->add("Warning: Failed to enable SeDebugPrivilege.");

	// Ensure we seed the RNG
	random(0, 1);

	// Open log file. Also creates log directory (if needed)
	openLog();

	{ // Ensure scripts directory exists
		const char *scriptsDir = Macro::instance()->getSettings()->getString(
			CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY).c_str();
		std::string scriptsFullPath = "";
		if( PathIsRelative(scriptsDir) )
		{
			scriptsFullPath = appendToPath(baseDirectory, scriptsDir);
			scriptsFullPath = fixSlashes(fixFileRelatives(scriptsFullPath), SLASHES_TO_WINDOWS);
		}
		else
			scriptsFullPath = scriptsDir;

		if( !directoryExists(scriptsFullPath.c_str()) )
		{
			SECURITY_ATTRIBUTES attribs;
			attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
			attribs.bInheritHandle = false;
			attribs.lpSecurityDescriptor = NULL;
			CreateDirectory(scriptsFullPath.c_str(), &attribs);
		}
	}

	#ifdef NETWORKING_ENABLED
		if( WSAStartup(MAKEWORD(2,2), &wsadata) != 0 )
		{
			fprintf(stderr, "Failed. Error code: %d\n", WSAGetLastError());
			return MicroMacro::ERR_ERR;
		}
	#endif

	/* If the script was passed in as a command-line argument... autoload it */
	bool autoloadScript = false;
	if( argc > 1 )
		autoloadScript = true;

	/* Begin main loop */
	running = true;
	bool yieldTimeSlice = Macro::instance()->getSettings()->getInt(CONFVAR_YIELD_TIME_SLICE, CONFDEFAULT_YIELD_TIME_SLICE);
	while(running)
	{
		// Reset CWD
		SetCurrentDirectory((LPCTSTR)&baseDirectory);

		#ifdef DEBUG_LSTATE_STACK
		// Warn to make sure we're not screwing up the stack
		if( lua_gettop(Macro::instance()->getEngine()->getLuaState()) != 0 )
			fprintf(stderr, "[WARN]: lua_gettop() is not 0 (zero).\n");
		#endif

		{ /* Reset window title */
			char title[1024];
			char basicTitle[64];
			EncString::reveal(basicTitle, sizeof(basicTitle), EncString::basicTitle);
			slprintf(title, sizeof(title)-1, basicTitle, AutoVersion::MAJOR, AutoVersion::MINOR, AutoVersion::BUILD);
			SendMessage(Macro::instance()->getAppHwnd(), WM_SETTEXT, (WPARAM)0, (LPARAM)title);

			// Once set, we forcefully flush it from the buffers
			securezero(basicTitle, sizeof(basicTitle));
			securezero(title, sizeof(title));
		}

		/* Prompt for script, if needed */
		std::string command;
		std::vector<std::string> args;
		if( autoloadScript )
		{
			command = argv[1];

			/* Fetch argv[] passed-in arguments, push into args array */
			args.push_back(argv[1]);
			for(int i = 2; i < argc; i++)
				args.push_back(argv[i]);

			/* Now clear out autoloadScript so we don't reuse it next iteration */
			autoloadScript = false;
		}
		else
		{
			command = promptForScript();
			splitArgs(command, args);
		}

		if( args[0] == "" )
		{ // We need something to run, duh!
			fprintf(stderr, "Error: You didn\'t even give me a script to run, silly!\n\n");
			continue;
		}


		/* Check for commands */
		if( command == "exit" )
		{ // Days over, let's go home.
			running = false;
			break;
		} else if( command == "clear" )
		{
			clearCliScreen();
			continue;
		} else if( command == "buildinfo" )
		{
			#ifdef _WIN64
				const char *bits = "x64";
			#else
				const char *bits = "x86";
			#endif
			printf("Version %ld.%ld.%ld%s revision %ld %s, built on %s-%s-%s\n%s\n\n",
				AutoVersion::MAJOR, AutoVersion::MINOR, AutoVersion::BUILD,
				AutoVersion::STATUS_SHORT, AutoVersion::REVISION, bits,
				AutoVersion::YEAR, AutoVersion::MONTH, AutoVersion::DATE,
				LUA_VERSION);
			continue;
		} else if( args[0] == "exec" )
		{
			int argc = args.size();
			if( argc <= 1 ) // If they didn't actually give us a command...
			{
				printf("\nEntering interactive mode. Enter 'exit' to quit.\n");
				while(true)
				{
					// Prompt for command
					printf("> ");

					std::string fullcmd;
					std::cin.clear();
					getline(std::cin, fullcmd);
					std::cin.clear();

					if( fullcmd == "exit" ) // Leave interactive mode
					{
						printf("\n\n");
						break;
					}

					// Run the string
					LuaEngine *E = Macro::instance()->getEngine();
					int success = E->loadString(fullcmd.c_str());
					if( success != MicroMacro::ERR_OK )
					{
						char buffer[1024];
						slprintf(buffer, sizeof(buffer)-1, "String execution error code: %d (%s)\n%s\n",
							success, getErrorString(success), E->getLastErrorMessage().c_str());
						fprintf(stderr, "%s\n", buffer);
						Logger::instance()->add(buffer);
					}
				}
			}
			else
			{
				// Prep the string, then run it
				size_t fpos = command.find_first_of(" \t");
				if( fpos == std::string::npos )
					continue;

				std::string cmd = command.substr(fpos+1);
				printf("Execute string: %s\n\n", cmd.c_str());

				// Run the string
				LuaEngine *E = Macro::instance()->getEngine();
				int success = E->loadString(cmd.c_str());
				if( success != MicroMacro::ERR_OK )
				{
					char buffer[1024];
					slprintf(buffer, sizeof(buffer)-1, "String execution error code: %d (%s)\n%s\n",
						success, getErrorString(success), E->getLastErrorMessage().c_str());
					fprintf(stderr, "%s\n", buffer);
					Logger::instance()->add(buffer);
				}

				// Make sure we re-initialize our Lua state before we move on
				E->reinit();
			}
			continue;
		}

		/* Correct filename if needed */
		args[0] = autoAdjustScriptFilename(args[0]);

		/* Record the script as 'previous' so we can reference it next iteration */
		previousScript = fixSlashes(args[0], SLASHES_TO_WINDOWS);

		/* Change CWD to script's directory */
		SetCurrentDirectory(getFilePath(args[0], false).c_str());

		/* Flush any events we might have before running the new script */
		Macro::instance()->flushEvents();

		/* Force re-poll human interface devices */
		Macro::instance()->getHid()->repollGamepadMaxIndex();
		Macro::instance()->getHid()->poll();

		/* Run script */
		printf("Running \'%s\'\n\n", args[0].c_str()/*script.c_str()*/);
		int success = Macro::instance()->getEngine()->loadFile(getFileName(args[0]).c_str());
		if( success != MicroMacro::ERR_OK )
		{
			LuaEngine *E = Macro::instance()->getEngine();

			// Reset text color (just in case)
			SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());

			char buffer[1024];
			slprintf(buffer, sizeof(buffer)-1, "Load file error code: %d (%s)\n%s\n",
				success, getErrorString(success), E->getLastErrorMessage().c_str());
			fprintf(stderr, "%s\n", buffer);
			Logger::instance()->add(buffer);

			E->reinit();
			continue;
		}

		/* Run initialization callback */
		success = Macro::instance()->getEngine()->runInit(&args);
		if( success != MicroMacro::ERR_OK )
		{
			LuaEngine *E = Macro::instance()->getEngine();

			// Reset text color (just in case)
			SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());

			char buffer[1024];
			slprintf(buffer, sizeof(buffer)-1, "Failed to run init function, err code: %d (%s)\n%s\n",
				success, getErrorString(success), E->getLastErrorMessage().c_str());
			fprintf(stderr, "%s\n", buffer);
			Logger::instance()->add(buffer);

			E->reinit();
			continue;
		}

		/* Begin script main loop */
		Macro::instance()->getHid()->poll();
		Macro::instance()->pollForegroundWindow();

		TimeType lastRepollGamepadMaxIndex = getNow();
		int runState = success;
		while( runState == MicroMacro::ERR_OK )
		{
			#ifdef DEBUG_LSTATE_STACK
			// Warn to make sure we're not screwing up the stack
			if( lua_gettop(Macro::instance()->getEngine()->getLuaState()) != 0 )
				fprintf(stderr, "[WARN]: lua_gettop() is not 0 (zero).\n");
			#endif

			// Update window focus
			Macro::instance()->pollForegroundWindow();

			// Repoll gamepads if needed
			if( deltaTime(getNow(), lastRepollGamepadMaxIndex) > GAMEPAD_REPOLL_SECONDS )
			{
				//int prevCount = Macro::instance()->getHid()->getGamepadMaxIndex();
				Macro::instance()->getHid()->repollGamepadMaxIndex();
				//int newCount = Macro::instance()->getHid()->getGamepadMaxIndex();

				lastRepollGamepadMaxIndex = getNow();
			}

			// Handle keyboard held queue
			Macro::instance()->getHid()->handleKeyHeldQueue();

			// Handle keyboard input
			Macro::instance()->handleHidInput();

			// Check for console resize
			Macro::instance()->pollConsoleResize();

			// Handle hotkeys
			Hid *phid = Macro::instance()->getHid();
			if( (Macro::instance()->getForegroundWindow() == Macro::instance()->getAppHwnd() &&
				phid->pressed('L') && phid->isDown(VK_CONTROL)) // Local CTRL+L
				|| ((phid->pressed('L') && phid->isDown(VK_CONTROL) && phid->isDown(VK_SHIFT) &&
				phid->isDown(VK_MENU))) ) // Global CTRL SHIFT ALT L
			{
				// Reset text color (just in case)
				SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());

				printf("\nScript forcibly terminated.\n");
				break;
			}

			// Handle events
			success = Macro::instance()->handleEvents();
			if( success != MicroMacro::ERR_OK )
			{
				LuaEngine *E = Macro::instance()->getEngine();

				// Reset text color (just in case)
				SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());

				char buffer[1024];
				slprintf(buffer, sizeof(buffer)-1, "Failed to run event function, err code: %d (%s)\n%s\n",
					success, getErrorString(success), E->getLastErrorMessage().c_str());
				fprintf(stderr, "%s\n", buffer);
				Logger::instance()->add(buffer);

				// Pass to event function
				MicroMacro::Event e;
				e.type = MicroMacro::EVENT_ERROR;
				e.msg = E->getLastErrorMessage();
				E->runEvent(e);

				break;
			}

			// Dispatch messages before running macro.main() func
			runState = Macro::instance()->getEngine()->dispatchWindowsMessages();

			if( runState == MicroMacro::ERR_OK )
			{ // Run main callback
				runState = Macro::instance()->getEngine()->runMain();
			}

			if( runState == MicroMacro::ERR_CLOSE )
			{ // Script requested to end
				// Reset text color (just in case)
				SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());

				printf("\nScript requested termination.\n");
				break;
			}
			else if( runState != MicroMacro::ERR_OK )
			{ // An actual error occurred
				LuaEngine *pEngine = Macro::instance()->getEngine();

				// Reset text color (just in case)
				SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());

				char buffer[1024];
				slprintf(buffer, sizeof(buffer)-1, "Error in main loop. Error code %d (%s)\n%s\n",
					runState, getErrorString(runState), pEngine->getLastErrorMessage().c_str());
				fprintf(stderr, "%s\n", buffer);
				Logger::instance()->add(buffer);

				// Pass to event function
				MicroMacro::Event e;
				e.type = MicroMacro::EVENT_ERROR;
				e.msg = pEngine->getLastErrorMessage();
				pEngine->runEvent(e);

				break;
			}

			// Don't waste CPU cycles
			if( yieldTimeSlice )
				Sleep(1);
			else
				Sleep(0);
		}

		// Shut down Ncurses
		if( Ncurses_lua::is_initialized() )
			Ncurses_lua::cleanup(Macro::instance()->getEngine()->getLuaState());

		#ifdef NETWORKING_ENABLED
			// Make sure we cleanup any networking stuff
			Network_lua::cleanup();
		#endif

		// Grab the user's attention
		FLASHWINFO fwi;
		fwi.hwnd = Macro::instance()->getAppHwnd();
		fwi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		fwi.cbSize = sizeof(FLASHWINFO);
		fwi.uCount = 3;
		fwi.dwTimeout = 0;
		FlashWindowEx(&fwi);

		/* Do cleanup, reinit */
		Macro::instance()->getEngine()->reinit();
	}

	#ifdef NETWORKING_ENABLED
		// Make sure we cleanup any networking stuff
		Network_lua::cleanup();

		WSACleanup();
	#endif

	Logger::instance()->add("All done. Closing down.\n");

	printf("Shutting down; execution finished.\n");
    return 0;
}

std::string scriptGUIDialog(std::string defaultFilename)
{
	const char *filter = "All Files\0*.*\0Lua files\0*.lua\0\0";

	// Make sure to use backslashes!
	defaultFilename = fixSlashes(defaultFilename, SLASHES_TO_WINDOWS);

	// We will use these buffers to store some data later.
	char cwdBuffer[MAX_PATH+1];
	char fileBuffer[MAX_PATH+1];
	char pathBuffer[MAX_PATH+1];

    /* NOTE: The dialog will modify the CWD, so we must restore it when done. */
    GetCurrentDirectory(MAX_PATH,(LPTSTR)&cwdBuffer);

	// Copy some default data into the buffers, prep OFN struct
	strlcpy(fileBuffer, getFileName(defaultFilename).c_str(), MAX_PATH);
	if( ::getFilePath(defaultFilename, false) == "" )
	{ // Assume scripts directory
		std::string buff = cwdBuffer;
		buff += "\\";
		buff += fixSlashes(
			Macro::instance()->getSettings()->getString(CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY),
			SLASHES_TO_WINDOWS);
		strlcpy(pathBuffer, buff.c_str(), MAX_PATH);
	}
	else
	{
		strlcpy(pathBuffer, fixSlashes(::getFilePath(defaultFilename, true), SLASHES_TO_WINDOWS).c_str(), MAX_PATH);
	}

    OPENFILENAME ofn;
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = Macro::instance()->getAppHwnd();
    ofn.hInstance = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = (LPSTR)&fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = pathBuffer;
    ofn.lpstrTitle = NULL;
    ofn.Flags = (DWORD)(OFN_ENABLESIZING);
    ofn.nFileOffset = (WORD)NULL;
    ofn.nFileExtension = (WORD)NULL;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = (WORD)NULL;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;
    ofn.pvReserved = NULL;
    ofn.dwReserved = (WORD)NULL;
    ofn.FlagsEx = (DWORD)0;

	// Show the dialog
	std::string retval = "";
    int success = GetOpenFileName(&ofn);
    if( success )
    { // User clicked OK
		retval = std::string("\"") + ofn.lpstrFile + std::string("\"");
    }

	// Reset CWD
    SetCurrentDirectory((LPCTSTR)&cwdBuffer);

	/* Attempt to un-fuck message queue
		A Windows bug causes GetOpenFileName to screw up the input queue
		which prevents GetKeyboardState() from functioning properly
	*/
	MSG msg;
	while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0 )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return retval;
}

std::string promptForScript()
{
	// Clear keyboard buffer
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	FlushConsoleInputBuffer(hStdin);

	printf("Please enter the script name to run.\n");
	printf("Type in \'exit\' (without quotes) to exit.\n");
	printf("Script> ");

	std::string fullcmd;
	std::cin.clear();
	getline(std::cin, fullcmd);
	std::cin.clear();

	if( fullcmd == "" )
		fullcmd = scriptGUIDialog(previousScript);

	//previousScript = fullcmd; // Remember this.
	return fullcmd;
}

void splitArgs(std::string cmd, std::vector<std::string> &args)
{
	const char *spaceReplace = "$_SPACE_$";
	const char *tabReplace = "$_TAB_$";
	size_t startpos = 0;
	size_t lastpos = 0;

	// Take quotes, replace sub-spaces
	startpos = cmd.find("\"");
	while( startpos != std::string::npos )
	{
		// Find the next quote
		size_t nextpos = cmd.find("\"", startpos+1);
		if( nextpos - startpos > 0 )
		{
			// Grab substring, remove quotes, substitute spaces
			std::string substr = cmd.substr(startpos+1, nextpos - startpos - 1);
			substr = strReplaceAll(substr, " ", spaceReplace);
			substr = strReplaceAll(substr, "\t", tabReplace);

			// Now pop it back into place.
			cmd.replace(startpos, nextpos - startpos + 1, substr);
			lastpos = nextpos+1;
			startpos = cmd.find("\"", lastpos);
		}
		else
			break;
	}

	// Push script as first arg, replace whitespace back to original
	startpos = cmd.find_first_of(" \t");
	std::string piece = strReplaceAll(cmd.substr(0, startpos), spaceReplace, " ");
	piece = strReplaceAll(piece, tabReplace, "\t");
	args.push_back( piece );

	// Now iterate over other pieces, do the same
	lastpos = startpos;
	while(startpos != std::string::npos)
	{
		startpos = cmd.substr(lastpos+1).find_first_of(" \t");
		std::string tmp = cmd.substr(lastpos+1, startpos);
		if( tmp != "" )
		{
			piece = strReplaceAll(tmp, spaceReplace, " ");
			piece = strReplaceAll(piece, tabReplace, "\t");
			args.push_back( piece );
		}
		lastpos += startpos +1;
	}
}

/* If filename is not found, attempt to locate the intended target.
	If it cannot be found, it returns the original filename
*/
std::string autoAdjustScriptFilename(std::string filename)
{
	std::string path;
	std::string scriptsDir = Macro::instance()->getSettings()->getString(
		CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY);

	// Test for the given full path
	if( fileExists(filename.c_str()) )
		return filename;

	// If it is a directory, try appending main.lua
	if( directoryExists(filename.c_str()) )
	{
		path = appendToPath(filename, "main.lua");
		if( fileExists(path.c_str()) )
			return path;
	}

	// If scriptsDir + filename is a directory, try appending main.lua
	path = appendToPath(scriptsDir, filename);
	if( directoryExists(path.c_str()) )
	{
		path = appendToPath(path, "main.lua");

		if( fileExists(path.c_str()) )
			return path;
	}

	// Try appending the .lua extension
	path = filename + ".lua";
	if( fileExists(path.c_str()) )
		return path;

	if( PathIsRelative(filename.c_str()) )
	{
		// Try prepending scripts dir
		path = appendToPath(scriptsDir, filename);//scriptsDir + filename;
		if( fileExists(path.c_str()) )
			return path;

		// Try prepending scripts dir and appending .lua extension
		path += ".lua";
		if( fileExists(path.c_str()) )
			return path;
	}

	return filename;
}


/* ===========================================================================
	Config stuff
   =========================================================================*/

double getConfigFloat(lua_State *L, const char *key, double defaultValue)
{
	lua_getglobal(L, key);
	double num = 0;

	if( lua_isnumber(L, -1) )
		num = lua_tonumber(L, -1);
	else
		num = defaultValue;

	lua_pop(L, 1);
	return num;
}

int getConfigInt(lua_State *L, const char *key, int defaultValue)
{
	lua_getglobal(L, key);
	int num = 0;

	if( lua_isnumber(L, -1) )
		num = lua_tointeger(L, -1);
	else if( lua_isboolean(L, -1) )
		num = (int)lua_toboolean(L, -1);
	else
		num = defaultValue;

	lua_pop(L, 1);
	return num;
}

std::string getConfigString(lua_State *L, const char *key, std::string defaultValue)
{
	lua_getglobal(L, key);

	if( lua_isstring(L, -1) )
		return lua_tostring(L, -1);
	else
		return defaultValue;
}

// Load a config file, copy its contents to the settings manager
int loadConfig(const char *filename)
{
	lua_State *lstate = luaL_newstate();
	if( !lstate )
		return MicroMacro::ERR_INIT_FAIL;

	// Load it, run it, check it.
	int retval = MicroMacro::ERR_OK;
	int failstate = luaL_loadfile(lstate, filename);
	if( !failstate )
		failstate = lua_pcall(lstate, 0, 0, 0);

	if( failstate )
	{
		switch(failstate) {
			case LUA_ERRRUN:		retval = MicroMacro::ERR_RUN;		break;
			case LUA_ERRMEM:		retval = MicroMacro::ERR_MEM;		break;
			case LUA_ERRSYNTAX:		retval = MicroMacro::ERR_SYNTAX;	break;
			case LUA_ERRFILE:		retval = MicroMacro::ERR_FILE;		break;
			case LUA_ERRERR:		retval = MicroMacro::ERR_ERR;		break;
			default:				retval = MicroMacro::ERR_UNKNOWN;	break;
		}

		const char *err = lua_tostring(lstate, -1);
		fprintf(stderr, "%s\n", err);
		lua_close(lstate);
		return retval;
	}


	// Copy from the settings file into our settings manager
	double fval = 0.0;
	int ival = 0;
	std::string szval;
	Settings *psettings = Macro::instance()->getSettings();

	ival = getConfigInt(lstate, CONFVAR_MEMORY_STRING_BUFFER_SIZE, CONFDEFAULT_MEMORY_STRING_BUFFER_SIZE);
	psettings->setInt(CONFVAR_MEMORY_STRING_BUFFER_SIZE, ival);

	ival = getConfigInt(lstate, CONFVAR_LOG_REMOVAL_DAYS, CONFDEFAULT_LOG_REMOVAL_DAYS);
	psettings->setInt(CONFVAR_LOG_REMOVAL_DAYS, ival);

	szval = getConfigString(lstate, CONFVAR_LOG_DIRECTORY, CONFDEFAULT_LOG_DIRECTORY);
	psettings->setString(CONFVAR_LOG_DIRECTORY, szval);

	szval = getConfigString(lstate, CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY);
	psettings->setString(CONFVAR_SCRIPT_DIRECTORY, szval);

	#ifdef AUDIO_ENABLED
	ival = getConfigInt(lstate, CONFVAR_AUDIO_ENABLED, CONFDEFAULT_AUDIO_ENABLED);
	psettings->setInt(CONFVAR_AUDIO_ENABLED, ival);
	#else
	psettings->setInt(CONFVAR_AUDIO_ENABLED, 0);
	#endif

	#ifdef NETWORKING_ENABLED
	ival = getConfigInt(lstate, CONFVAR_NETWORK_BUFFER_SIZE, CONFDEFAULT_NETWORK_BUFFER_SIZE);
	if( ival < 16 ) // Make sure it is reasonable...
		ival = CONFDEFAULT_NETWORK_BUFFER_SIZE;
	if( ival > 65535 ) // Maximum packet size
		ival = 65535;
	psettings->setInt(CONFVAR_NETWORK_BUFFER_SIZE, ival);

	ival = getConfigInt(lstate, CONFVAR_RECV_QUEUE_SIZE, CONFDEFAULT_RECV_QUEUE_SIZE);
	if( ival < 4 ) // Make sure it is reasonable...
		ival = CONFDEFAULT_RECV_QUEUE_SIZE;
	if( ival > 10240 )
		ival = 10240;
	psettings->setInt(CONFVAR_RECV_QUEUE_SIZE, ival);
	#endif

	ival = getConfigInt(lstate, CONFVAR_YIELD_TIME_SLICE, CONFDEFAULT_YIELD_TIME_SLICE);
	psettings->setInt(CONFVAR_YIELD_TIME_SLICE, ival);

	lua_close(lstate);
	return retval;
}



/* ===========================================================================
	Output stuff
   =========================================================================*/

// Just dump the intro text
void printStdHead()
{
	WORD color = 3; // Green
	HANDLE handle = Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	// Get current text settings
	GetConsoleScreenBufferInfo(handle, &csbi);

	// Change settings, output
	SetConsoleTextAttribute(handle, color);

	char basicTitle[64];
	char website[256];
	EncString::reveal(basicTitle, sizeof(basicTitle), EncString::basicTitle);
	EncString::reveal(website, sizeof(website), EncString::website);

	printf(basicTitle, AutoVersion::MAJOR, AutoVersion::MINOR, AutoVersion::BUILD);
	printf("\n%s\n", website);

	// Now flush the buffers
	securezero(basicTitle, sizeof(basicTitle));
	securezero(website, sizeof(website));

	// Revert to original text settings
	color = csbi.wAttributes;
	SetConsoleTextAttribute(handle, color);
}

// Clear the screen, much like the cls command
void clearCliScreen()
{
	HANDLE stdOut = Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/;
	COORD coord = {0, 0};
	DWORD count;

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(stdOut, &csbi);

	FillConsoleOutputCharacter(stdOut, ' ', csbi.dwSize.X * csbi.dwSize.Y,
		coord, &count);

	SetConsoleCursorPosition(stdOut, coord);
}



/* ===========================================================================
	Logging stuff
   =========================================================================*/

/*
	Just like you imagine, it deletes log files older than
	daysToDelete in days that exist in the given path
*/
void deleteOldLogs(const char *path, unsigned int daysToDelete)
{
	std::vector<std::string> files = getDirectory(path, "txt");
	FILETIME ft_now;
	GetSystemTimeAsFileTime(&ft_now);

	for(unsigned int i = 0; i < files.size(); i++)
	{
		// Get timestamp from file
		bool success;
		char fname[MAX_PATH];
		WIN32_FILE_ATTRIBUTE_DATA fad;
		slprintf(fname, sizeof(fname)-1, "%s/%s", path, files.at(i).c_str());
		success = GetFileAttributesEx(fname, GetFileExInfoStandard, &fad);

		// If it fails for some reason, just move on to the next
		if( !success )
			continue;

		/* Note: Do *NOT* mix TimeTypes from getNow() with FILETIMEs
			or you're going to have a bad time.
		*/
		unsigned int seconds = filetimeDelta(&ft_now, &fad.ftLastAccessTime);
		unsigned int days = seconds/(24*60*60);

		if( days >= daysToDelete )
			DeleteFile(fname);
	}
}

// Just opens the log file, dumps some basic info
void openLog()
{
	Settings *psettings = Macro::instance()->getSettings();
	std::string logDir = psettings->getString(CONFVAR_LOG_DIRECTORY);
	unsigned int logRemovalDays = (unsigned int)psettings->getInt(CONFVAR_LOG_REMOVAL_DAYS);

	std::string logFullPath;
	if(	PathIsRelative(logDir.c_str()) )
	{
		logFullPath = appendToPath(baseDirectory, logDir);

		// Append '\\' if needed
		char lastChar = *logFullPath.rbegin();
		if( lastChar != '/' && lastChar != '\\' )
			logFullPath += "\\";

		logFullPath = fixSlashes(fixFileRelatives(logFullPath), SLASHES_TO_WINDOWS);
	}
	else
		logFullPath = logDir;

	// Ensure log directory exists
	if( !directoryExists(logFullPath.c_str()) )
	{
		SECURITY_ATTRIBUTES attribs;
		attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
		attribs.bInheritHandle = false;
		attribs.lpSecurityDescriptor = NULL;
		CreateDirectory(logFullPath.c_str(), &attribs);
	}

	// Remove old logs, open a new log file
	if( logRemovalDays > 0 )
		deleteOldLogs(logFullPath.c_str(), logRemovalDays); // Remove old stuff before creating a new log

	char logfileName[MAX_PATH];
	char szTime[256];
	time_t rawtime;
	struct tm * timeinfo;
	time( &rawtime );
	timeinfo = localtime(&rawtime);

	// Iterate through, increasing ID, to find what we will name our log
	bool nameFound = false;
	unsigned int fileCount = 1;
	while( !nameFound )
	{
		strftime(szTime, sizeof(szTime)-1, "%Y-%m-%d", timeinfo);
		slprintf(logfileName, sizeof(logfileName)-1,
			"%s%s-%02u.txt", logFullPath.c_str(), szTime, fileCount);
		nameFound = !fileExists(logfileName);
		++fileCount;
	}

	Logger::instance()->open(logfileName);
	if( Logger::instance()->is_open() )
		fprintf(stdout, "Logging to %s\n\n", logfileName);
	else
		fprintf(stdout, "Failed to open file for logging.\n\n");



	// Get CPU info
	HKEY hKey;
	char szProcessorSpeed[32];
	char szProcessorName[256];
	int rError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0, KEY_READ, &hKey);
	if( rError == ERROR_SUCCESS )
	{
		DWORD mhz = _MAX_PATH;
		DWORD buffSize;

		// Name
		buffSize = sizeof(szProcessorName) - 1;
		RegQueryValueEx(hKey, "ProcessorNameString", NULL, NULL, (BYTE *)&szProcessorName, &buffSize);
		slprintf((char*)&szProcessorName, sizeof(szProcessorName)-1, "%s", szProcessorName);

		// Speed
		buffSize = sizeof(mhz);
		RegQueryValueEx(hKey, "~MHz", NULL, NULL, (BYTE *)&mhz, &buffSize);
		slprintf((char*)&szProcessorSpeed, sizeof(szProcessorSpeed)-1, "@%dMHz", (int)mhz);
	}

	// Get Windows & user info
	DWORD userGroup = OS::getUserPriv();
	std::string userGroupName = OS::getPrivName(userGroup);

	// Now print out basic info to it
	char splitLine80[81];
	memset((char*)&splitLine80, '-', 80);
	splitLine80[79] = '\n';
	splitLine80[80] = 0;

	#ifdef _WIN64
		const char *bits = "x64";
	#else
		const char *bits = "x86";
	#endif

	char logVersionFmt[64];
	EncString::reveal(logVersionFmt, sizeof(logVersionFmt), EncString::logVersionFmt);

	Logger::instance()->add(logVersionFmt, AutoVersion::FULLVERSION_STRING, AutoVersion::STATUS, bits);
	Logger::instance()->add("%s %s, %s\n", szProcessorName, szProcessorSpeed, OS::getOsName().c_str());
	Logger::instance()->add("User privilege: %s\n", userGroupName.c_str());
	Logger::instance()->add_raw((char *)&splitLine80);
	Logger::instance()->add_raw("\n\n");

	// Flush it
	securezero(logVersionFmt, sizeof(logVersionFmt));
}

// Capture important events, such as force shutdown
static BOOL WINAPI consoleControlCallback(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
		case CTRL_C_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			/* We pretty much have to call exit(0) here.
				Would be nice to be able to just set mainRunning = false
				and let it exit graciously, but, that's WIN32 for you.
			*/

			// Close. Down. EVERYTHING.
			/*	NOTE: We don't want to do this while a script could be running...
				That may cause a SEGFAULT
				TODO: Gracefully terminate the Lua thread, somehow.
				Macro::instance()->cleanup();
			*/
			Logger::instance()->add("Process forcefully terminated (Win32 callback)\n");
			exit(EXIT_SUCCESS);
			return true;
		break;
	}
	return false;
}
