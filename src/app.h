#ifndef APP_H
#define APP_H

	#include "wininclude.h"
	#include <string>
	#include <vector>
	#include "argv.h"

	typedef struct lua_State lua_State;

	namespace MicroMacro
	{
		class App
		{
			protected:
				static const int GAMEPAD_REPOLL_SECONDS = 10;
				static const int WM_SYSICON = (WM_USER + 1);

				HINSTANCE hinstance;
				HWND messageReceiveHwnd;
				char baseDirectory[MAX_PATH+1];
				std::string previousScript;
				bool running;
				int argc;
				Argv argv;
				#ifdef NETWORKING_ENABLED
				WSADATA wsadata;
				#endif
				bool ansiTerm = false;


				HWND createMessageReceiveWindow(HINSTANCE);
				static LRESULT CALLBACK windowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
				void openLog();
				void deleteOldLogs(const char *path, unsigned int daysToDelete);
				void clearCliScreen();
				void printStdHead();
				int loadConfig(const char *filename);
				double getConfigFloat(lua_State *L, const char *key, double defaultValue);
				int getConfigInt(lua_State *L, const char *key, int defaultValue);
				std::string getConfigString(lua_State *L, const char *key, std::string defaultValue);
				std::string autoAdjustScriptFilename(std::string filename);
				void splitArgs(std::string cmd, std::vector<std::string> &args);
				std::string promptForScript();
				std::string scriptGUIDialog(std::string defaultFilename);
				int enableVirtualTerminal();
				void renderErrorMessage(int errCode, const char *lastErrorMessage, const char *description);

				void resetFontRendering();
				void flashWindow(int count);
				void printBuildInfo();
				void execRepl();
				void execString(std::string command);
				int runCommandFromFolder(const char *cmdFilePath, std::vector<std::string> args);
				void runScript(std::vector<std::string> args);

			public:
				App(HINSTANCE hinstance, LPSTR cmdLine);
				~App();

				int run();
				HWND getHwnd();
		};
	}

#endif
