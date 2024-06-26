#include "app.h"

#include "wininclude.h"     // Be sure to include this *before* ncurses!
#include <stdio.h>
#include <string.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <queue>
#include <time.h>
#include <iostream>
#include <conio.h>
#include <Shlwapi.h>
#include <regex>
#include <unistd.h>
#include <stdbool.h>

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
#include "debugmessages.h"
#include "resource.h"
#include "argv.h"

#include "encstring.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

using MicroMacro::App;

App::App(HINSTANCE hinstance, LPSTR cmdLine)
{
    running         =   false;
    this->hinstance =   hinstance;

    /* We emulate the standard C/C++ main() function in this way.
        This code exists to keep compatibility simple.

        If you simply comment these lines out and restore main()
        declaration to "normal," you're good to go.
    */
    argc = 1;

    char filename[MAX_PATH];
    GetModuleFileName( NULL, filename, MAX_PATH );
    argv.add(filename);

    if( strlen(cmdLine) > 0 )
    {   // If we were passed a command-line, tokenize it and dump it into argv
        argv.parse(cmdLine);
        argc = argv.size();
    }
    /* END: main() compatibility */

    // Window initialization stuff here
    messageReceiveHwnd = createMessageReceiveWindow(hinstance);
}

App::~App()
{
}

int App::run()
{
    // Enable virtual terminal so that we can use ANSI colors
    this->enableVirtualTerminal();

    // Intro text output
    printStdHead();

    // Extract MicroMacro's base path from argv[0]
    GetFullPathName(argv[0], MAX_PATH, baseDirectory, NULL);
    strlcpy(baseDirectory,
            fixSlashes(getFilePath(baseDirectory, true), SLASHES_TO_WINDOWS).c_str(), MAX_PATH);

    /* Copy the base path into the Lua engine; Do this *before* initializing! */
    Macro::instance()->getEngine()->setBasePath(baseDirectory);
    {   /* Run configs */
        std::string configFilename = appendToPath(baseDirectory, CONFIG_FILENAME);
        configFilename = fixSlashes(configFilename, SLASHES_TO_STANDARD);

        if( !fileExists(configFilename.c_str()) )
        {   // Lets try to copy it from default.config.lua, if it exists.
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

    {   /* Initiate our macro singleton */
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

    {   // Ensure scripts directory exists
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
    if( WSAStartup(MAKEWORD(2, 2), &wsadata) != 0 )
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
    running         =   true;
    previousScript  =   "";
    while(running)
    {
        // Reset CWD
        SetCurrentDirectory((LPCTSTR)&baseDirectory);

        // Reset log level to configured default
        LogLevel logLevel = static_cast<LogLevel>(Macro::instance()->getSettings()->getInt(CONFVAR_LOG_LEVEL));
        Logger::instance()->setLevel(logLevel);

        #ifdef DEBUG_LSTATE_STACK
        // Warn to make sure we're not screwing up the stack
        if( lua_gettop(Macro::instance()->getEngine()->getLuaState()) != 0 )
            fprintf(stderr, "[WARN]: lua_gettop() is not 0 (zero).\n");
        #endif

        {   /* Reset window title */
            char title[1024];
            char basicTitle[64];
            EncString::reveal(basicTitle, sizeof(basicTitle), EncString::basicTitle);
            slprintf(title, sizeof(title) - 1, basicTitle, AutoVersion::MAJOR, AutoVersion::MINOR, AutoVersion::BUILD);
            SendMessage(Macro::instance()->getAppHwnd(), WM_SETTEXT, (WPARAM)0, (LPARAM)title);

            // Once set, we forcefully flush it from the buffers
            securezero(basicTitle, sizeof(basicTitle));
            securezero(title, sizeof(title));
        }

        resetFontRendering();

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
        {   // We need something to run, duh!
            fprintf(stderr, "Error: You didn\'t even give me a script to run, silly!\n\n");
            continue;
        }

        Macro::instance()->getEngine()->setCloseState(false);
        /* Check for commands */
        if( command == "exit" )
        {   // Days over, let's go home.
            running = false;
            break;
        } else if( command == "clear" )
        {
            clearCliScreen();
            continue;
        } else if( command == "buildinfo" )
        {
            printBuildInfo();
            continue;
        } else if( args[0] == "exec" )
        {
            if( args.size() <= 1 ) // If they didn't actually give us a command...
            {
                execRepl();
            }
            else
            {
                execString(command);
            }
            continue;
        }

        std::string cmdFilePath = std::string("commands/").append(args[0]).append(".lua");
        if( ::fileExists(cmdFilePath.c_str())) {
            args.erase(args.begin());
            this->runCommandFromFolder(cmdFilePath.c_str(), args);
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
        runScript(args);

        // Force garbage collection
        lua_gc(Macro::instance()->getEngine()->getLuaState(), LUA_GCCOLLECT, 0);

        // Shut down Ncurses
        if( Ncurses_lua::is_initialized() )
            Ncurses_lua::cleanup(Macro::instance()->getEngine()->getLuaState());

        /* Do cleanup, reinit */
        Macro::instance()->getEngine()->reinit();


        // Grab the user's attention
        flashWindow(3);
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

HWND App::getHwnd()
{
    return messageReceiveHwnd;
}

HWND App::createMessageReceiveWindow(HINSTANCE hinstance)
{
    const char *hiddenWindowClass = "catchmsg";
    // Create our window for receiving Windows messages
    WNDCLASSEX wincl;

    /* The Window structure */
    wincl.hInstance = hinstance;
    wincl.lpszClassName = hiddenWindowClass;
    wincl.lpfnWndProc = windowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon         =   LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
    wincl.hIconSm       =   LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON));
    wincl.hCursor       =   LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName  =   NULL; // No menu
    wincl.cbClsExtra    =   0;
    wincl.cbWndExtra    =   0;
    wincl.hbrBackground =   (HBRUSH)(CreateSolidBrush(RGB(0, 0, 0)));

    if( !RegisterClassEx(&wincl) )
    {
        fprintf(stderr, "Failed to register class\n");
        return 0;
    }

    HWND hwnd = CreateWindowEx(
                    0,
                    hiddenWindowClass,       // Classname
                    hiddenWindowClass,       // Title Text
                    WS_DISABLED | WS_ICONIC, // Style
                    CW_USEDEFAULT,           // Default position
                    CW_USEDEFAULT,           // Default position
                    0,                       // Width
                    0,                       // Height
                    HWND_DESKTOP,            // Parent (don't use main console window!)
                    NULL,                    // No menu
                    hinstance,
                    NULL                     // No Window Creation data
                );

    if( !hwnd )
    {
        fprintf(stderr, "Failed to create message catcher\n");
        return 0;
    }

    return hwnd;
}



LRESULT CALLBACK App::windowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_SYSCOMMAND:
            /*In WM_SYSCOMMAND messages, the four low-order bits of the wParam parameter
            are used internally by the system. To obtain the correct result when testing the value of wParam,
            an application must combine the value 0xFFF0 with the wParam value by using the bitwise AND operator.*/

            switch( wParam & 0xFFF0 )
            {
                case SC_MINIMIZE:
                case SC_CLOSE:
                    return 0;
                    break;
            }
            break;

        case WM_SYSICON:
            {
                if( lParam == WM_LBUTTONUP )
                {
                    ShowWindow(Macro::instance()->getAppHwnd(), SW_RESTORE);
                }
            }
            break;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

// Just opens the log file, dumps some basic info
void App::openLog()
{
    Settings *psettings = Macro::instance()->getSettings();
    std::string logDir = psettings->getString(CONFVAR_LOG_DIRECTORY);
    unsigned int logRemovalDays = (unsigned int)psettings->getInt(CONFVAR_LOG_REMOVAL_DAYS);

    std::string logFullPath;
    if( PathIsRelative(logDir.c_str()) )
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
        strftime(szTime, sizeof(szTime) - 1, "%Y-%m-%d", timeinfo);
        slprintf(logfileName, sizeof(logfileName) - 1,
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
        slprintf((char*)&szProcessorName, sizeof(szProcessorName) - 1, "%s", szProcessorName);

        // Speed
        buffSize = sizeof(mhz);
        RegQueryValueEx(hKey, "~MHz", NULL, NULL, (BYTE *)&mhz, &buffSize);
        slprintf((char*)&szProcessorSpeed, sizeof(szProcessorSpeed) - 1, "@%dMHz", (int)mhz);
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
    Logger::instance()->add("%s %s, %s", szProcessorName, szProcessorSpeed, OS::getOsName().c_str());
    Logger::instance()->add("User privilege: %s", userGroupName.c_str());
    Logger::instance()->add_raw((char *)&splitLine80);
    Logger::instance()->add_raw("\n");

    // Flush it
    securezero(logVersionFmt, sizeof(logVersionFmt));
}

/*
    Just like you imagine, it deletes log files older than
    daysToDelete in days that exist in the given path
*/
void App::deleteOldLogs(const char *path, unsigned int daysToDelete)
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
        slprintf(fname, sizeof(fname) - 1, "%s/%s", path, files.at(i).c_str());
        success = GetFileAttributesEx(fname, GetFileExInfoStandard, &fad);

        // If it fails for some reason, just move on to the next
        if( !success )
            continue;

        /* Note: Do *NOT* mix TimeTypes from getNow() with FILETIMEs
            or you're going to have a bad time.
        */
        unsigned int seconds = filetimeDelta(&ft_now, &fad.ftLastAccessTime);
        unsigned int days = seconds / (24 * 60 * 60);

        if( days >= daysToDelete )
            DeleteFile(fname);
    }
}

// Clear the screen, much like the cls command
void App::clearCliScreen()
{
    HANDLE stdOut = Macro::instance()->getAppHandle()/*GetStdHandle(STD_OUTPUT_HANDLE)*/;
    COORD coord = {0, 0};

    DWORD count;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(stdOut, &csbi);

    if( ansiTerm )
    {
        // ANSI clear -- prevents bugginess with ANSI output in the buffer
        PCWSTR sequence = L"\x1b[2J\x1b[3J";
        DWORD written = 0;
        WriteConsoleW(stdOut, sequence, (DWORD)wcslen(sequence), &written, NULL);
    }

    FillConsoleOutputAttribute(stdOut, csbi.wAttributes, csbi.dwMaximumWindowSize.X * csbi.dwMaximumWindowSize.Y, coord, &count);
    FillConsoleOutputCharacter(stdOut, ' ', csbi.dwMaximumWindowSize.X * csbi.dwMaximumWindowSize.Y, coord, &count);
    SetConsoleCursorPosition(stdOut, coord);
}

// Just dump the intro text
void App::printStdHead()
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

// Load a config file, copy its contents to the settings manager
int App::loadConfig(const char *filename)
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
        retval = mapLuaError(failstate);

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

    ival = getConfigInt(lstate, CONFVAR_LOG_LEVEL, CONFDEFAULT_LOG_LEVEL);
    psettings->setInt(CONFVAR_LOG_LEVEL, ival);

    szval = getConfigString(lstate, CONFVAR_LOG_DIRECTORY, CONFDEFAULT_LOG_DIRECTORY);
    psettings->setString(CONFVAR_LOG_DIRECTORY, szval);

    szval = getConfigString(lstate, CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY);
    psettings->setString(CONFVAR_SCRIPT_DIRECTORY, szval);

    #ifdef NETWORKING_ENABLED
    ival = getConfigInt(lstate, CONFVAR_NETWORK_ENABLED, CONFDEFAULT_NETWORK_ENABLED);
    psettings->setInt(CONFVAR_NETWORK_ENABLED, ival);

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

    ival = getConfigInt(lstate, CONFVAR_STYLE_ERRORS, CONFDEFAULT_STYLE_ERRORS);
    psettings->setInt(CONFVAR_STYLE_ERRORS, ival);

    szval = getConfigString(lstate, CONFVAR_FILE_STYLE, CONFDEFAULT_FILE_STYLE);
    psettings->setString(CONFVAR_FILE_STYLE, szval);

    szval = getConfigString(lstate, CONFVAR_LINE_NUMBER_STYLE, CONFDEFAULT_LINE_NUMBER_STYLE);
    psettings->setString(CONFVAR_LINE_NUMBER_STYLE, szval);

    szval = getConfigString(lstate, CONFVAR_MESSAGE_STYLE, CONFDEFAULT_LINE_NUMBER_STYLE);
    psettings->setString(CONFVAR_MESSAGE_STYLE, szval);

    lua_close(lstate);
    return retval;
}

double App::getConfigFloat(lua_State *L, const char *key, double defaultValue)
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

int App::getConfigInt(lua_State *L, const char *key, int defaultValue)
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

std::string App::getConfigString(lua_State *L, const char *key, std::string defaultValue)
{
    lua_getglobal(L, key);

    if( lua_isstring(L, -1) )
        return lua_tostring(L, -1);
    else
        return defaultValue;
}

/* If filename is not found, attempt to locate the intended target.
    If it cannot be found, it returns the original filename
*/
std::string App::autoAdjustScriptFilename(std::string filename)
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

void App::splitArgs(std::string cmd, std::vector<std::string> &args)
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
        size_t nextpos = cmd.find("\"", startpos + 1);
        if( nextpos - startpos > 0 )
        {
            // Grab substring, remove quotes, substitute spaces
            std::string substr = cmd.substr(startpos + 1, nextpos - startpos - 1);
            substr = strReplaceAll(substr, " ", spaceReplace);
            substr = strReplaceAll(substr, "\t", tabReplace);

            // Now pop it back into place.
            cmd.replace(startpos, nextpos - startpos + 1, substr);
            lastpos = nextpos + 1;
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
        startpos = cmd.substr(lastpos + 1).find_first_of(" \t");
        std::string tmp = cmd.substr(lastpos + 1, startpos);
        if( tmp != "" )
        {
            piece = strReplaceAll(tmp, spaceReplace, " ");
            piece = strReplaceAll(piece, tabReplace, "\t");
            args.push_back( piece );
        }
        lastpos += startpos + 1;
    }
}

std::string App::promptForScript()
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

std::string App::scriptGUIDialog(std::string defaultFilename)
{
    const char *filter = "All Files\0*.*\0Lua files\0*.lua\0\0";

    // Make sure to use backslashes!
    defaultFilename = fixSlashes(defaultFilename, SLASHES_TO_WINDOWS);

    // We will use these buffers to store some data later.
    char cwdBuffer[MAX_PATH + 1];
    char fileBuffer[MAX_PATH + 1];
    char pathBuffer[MAX_PATH + 1];

    /* NOTE: The dialog will modify the CWD, so we must restore it when done. */
    GetCurrentDirectory(MAX_PATH, (LPTSTR)&cwdBuffer);

    // Copy some default data into the buffers, prep OFN struct
    strlcpy(fileBuffer, getFileName(defaultFilename).c_str(), MAX_PATH);
    if( ::getFilePath(defaultFilename, false) == "" )
    {   // Assume scripts directory
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
    {   // User clicked OK
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

int App::enableVirtualTerminal()
{
    // https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
    HANDLE handle = Macro::instance()->getAppHandle();

    if (handle == INVALID_HANDLE_VALUE)
    {
        return GetLastError();
    }

    DWORD dwMode = 0;
    if (!GetConsoleMode(handle, &dwMode))
    {
        return GetLastError();
    }

    // Enable virtual terminal if not already set
    ansiTerm = isAnsiSupported();
    if( !(dwMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) )
    {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        if (!SetConsoleMode(handle, dwMode))
            return GetLastError();
        ansiTerm = isAnsiSupported();
    }

    return 0;
}

bool App::isAnsiSupported() {
    char *term = getenv("TERM");
    if (term != NULL && strstr(term, "xterm") != NULL) {
        return true;
    }

    if( isatty(STDOUT_FILENO) ) {
        return true;
    }

    return false;
}

void App::renderErrorMessage(int errCode, const char *lastErrorMessage, const char *description)
{
    char buffer[4096];
    Settings *psettings = Macro::instance()->getSettings();
    bool styleErrors = psettings->getInt(CONFVAR_STYLE_ERRORS);
    std::string renderedError = lastErrorMessage;

    std::string fileStyle = psettings->getString(CONFVAR_FILE_STYLE);
    std::string lineNumberStyle = psettings->getString(CONFVAR_LINE_NUMBER_STYLE);
    std::string messageStyle = psettings->getString(CONFVAR_MESSAGE_STYLE);

    if( ansiTerm && styleErrors ) {
        std::regex baseRegex("^([^:]+):(\\d+): ((.|\r|\n)*?)\n?(stack traceback:\n((.|\r|\n)*))?$");
        std::cmatch baseMatch;
        std::string stacktrace = "";

        if (std::regex_match(lastErrorMessage, baseMatch, baseRegex))
        {
            renderedError = fileStyle + baseMatch[1].str() + "\x1b[0m:";
            renderedError += lineNumberStyle + baseMatch[2].str() + "\x1b[0m\n";
            renderedError += messageStyle + baseMatch[3].str() + "\x1b[0m\n";

            if( baseMatch[5] != "" ) {
                renderedError += "\n";
            }
            stacktrace = baseMatch[5].str();
        }
        else
        {
            baseRegex = std::regex("^((.|\r|\n)*)\nstack traceback:\n((.|\r|\n)*)$");
            if (std::regex_match(lastErrorMessage, baseMatch, baseRegex))
            {
                renderedError = messageStyle + baseMatch[1].str() + "\x1b[0m\n\nstack trace:\n";
                stacktrace = baseMatch[3].str();
            }
        }

        std::istringstream in(stacktrace);
        std::string line;
        std::string renderedStacktrace;
        std::smatch stacktraceMatch;
        const std::regex stacktraceRegex("^([^:]+):(\\d+)((.|\r|\n)*)");

        while(std::getline(in, line)) {
            if (std::regex_match(line, stacktraceMatch, stacktraceRegex))
            {
                renderedStacktrace += fileStyle + stacktraceMatch[1].str() + "\x1b[0m:";
                renderedStacktrace += lineNumberStyle + stacktraceMatch[2].str() + "\x1b[0m";
                renderedStacktrace += stacktraceMatch[3].str() + "\n";
            } else
            {
                renderedStacktrace += line.c_str();
                renderedStacktrace += "\n";
            }
        }
        renderedError += renderedStacktrace;
    }

    slprintf(buffer, sizeof(buffer) - 1, "%s -- err code: %d (%s)\n%s\n",
             description, errCode, getErrorString(errCode), renderedError.c_str());

    fprintf(stderr, "%s\n", buffer);
}

void App::resetFontRendering()
{
    // (B = Switch to ASCII character set
    // [0m = reset to default fonts
    if( ansiTerm )
        printf("\x1b(B\x1b[0m");

    SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());
}

void App::flashWindow(int count)
{
    FLASHWINFO fwi;
    fwi.hwnd = Macro::instance()->getAppHwnd();
    fwi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
    fwi.cbSize = sizeof(FLASHWINFO);
    fwi.uCount = count;
    fwi.dwTimeout = 0;
    FlashWindowEx(&fwi);
}

void App::printBuildInfo() {
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
}

/*
    Read, Evaluate, Print, Loop
*/
void App::execRepl()
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

        if( fullcmd == "exit" || fullcmd == "quit" ) // Leave interactive mode
        {
            printf("\n\n");
            return;
        }

        Macro::instance()->getHid()->poll();

        // Run the string
        LuaEngine *E = Macro::instance()->getEngine();
        int success = E->loadString(fullcmd.c_str());
        if( success != MicroMacro::ERR_OK )
        {
            std::string lastErrorMessage = E->getLastErrorMessage();
            this->renderErrorMessage(success, lastErrorMessage.c_str(), "String execution error");
            Logger::instance()->add("%s", lastErrorMessage.c_str());
        }
    }
}

void App::execString(std::string command)
{
    // Prep the string, then run it
    size_t fpos = command.find_first_of(" \t");
    if( fpos == std::string::npos )
        return;

    std::string cmd = command.substr(fpos + 1);
    printf("Execute string: %s\n\n", cmd.c_str());
    Logger::instance()->add("Execute string: \'%s\'\n", cmd.c_str());

    // Run the string
    LuaEngine *E = Macro::instance()->getEngine();
    int success = E->loadString(cmd.c_str());
    if( success != MicroMacro::ERR_OK )
    {
        std::string lastErrorMessage = E->getLastErrorMessage();
        this->renderErrorMessage(success, lastErrorMessage.c_str(), "String execution error");
        Logger::instance()->add("%s", lastErrorMessage.c_str());
    }

    // Make sure we re-initialize our Lua state before we move on
    E->reinit();
}

int App::runCommandFromFolder(const char *cmdFilePath, std::vector<std::string> args)
{
    const char *msgFmt = "Running command file %s\n";
    Logger::instance()->add(msgFmt, cmdFilePath);
    LuaEngine *E = Macro::instance()->getEngine();

    // Push args
    lua_newtable(E->getLuaState());
    for(int i = 0; i < args.size(); i++) {
        lua_pushinteger(E->getLuaState(), i + 1);
        lua_pushstring(E->getLuaState(), args.at(i).c_str());
        lua_settable(E->getLuaState(), -3);
    }
    lua_setglobal(E->getLuaState(), "args");

    // Run command script
    printf("\n");
    int success = mapLuaError(luaL_dofile(E->getLuaState(), cmdFilePath));
    if( success != MicroMacro::ERR_OK )
    {
        E->stdError();
        std::string lastErrorMessage = E->getLastErrorMessage();
        this->renderErrorMessage(success, lastErrorMessage.c_str(), "Error while executing command file");
        Logger::instance()->add("%s", lastErrorMessage.c_str());
    }

    E->reinit();
    return success;
}

void App::runScript(std::vector<std::string> args)
{
    bool yieldTimeSlice = Macro::instance()->getSettings()->getInt(CONFVAR_YIELD_TIME_SLICE, CONFDEFAULT_YIELD_TIME_SLICE);

    if(args.size() == 0) {
        throw std::out_of_range("Missing arguments");
    }

    printf("Running \'%s\'\n\n", args[0].c_str()/*script.c_str()*/);
    Logger::instance()->add("Running \'%s\'\n", args[0].c_str());
    int success = Macro::instance()->getEngine()->loadFile(getFileName(args[0]).c_str());
    if( success != MicroMacro::ERR_OK )
    {
        LuaEngine *E = Macro::instance()->getEngine();

        resetFontRendering();
        char buffer[1024];
        slprintf(buffer, sizeof(buffer) - 1, "Load file error code: %d (%s)\n%s\n",
                 success, getErrorString(success), E->getLastErrorMessage().c_str());
        fprintf(stderr, "%s\n", buffer);
        Logger::instance()->add("%s", buffer);

        E->reinit();
        return;
    }

    /* Run initialization callback */
    success = Macro::instance()->getEngine()->runInit(&args);
    if( success != MicroMacro::ERR_OK )
    {
        LuaEngine *E = Macro::instance()->getEngine();

        resetFontRendering();
        std::string lastErrorMessage = E->getLastErrorMessage();
        this->renderErrorMessage(success, lastErrorMessage.c_str(), "Failed to run init function");
        Logger::instance()->add("%s", lastErrorMessage.c_str());

        E->reinit();
        return;
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
            resetFontRendering();

            printf("\nScript forcibly terminated.\n");
            return;
        }

        // Handle events
        success = Macro::instance()->handleEvents();
        if( success != MicroMacro::ERR_OK )
        {
            LuaEngine *E = Macro::instance()->getEngine();

            resetFontRendering();
            std::string lastErrorMessage = E->getLastErrorMessage();
            this->renderErrorMessage(success, lastErrorMessage.c_str(), "Failed to run event function");
            Logger::instance()->add("%s", lastErrorMessage.c_str());

            // Pass to event function
            MicroMacro::Event *pe = new MicroMacro::Event;
            pe->type = MicroMacro::EVENT_ERROR;
            MicroMacro::EventData ced;
            ced.setValue(E->getLastErrorMessage());
            pe->data.push_back(ced);
            E->runEvent(pe);

            return;
        }

        // Dispatch messages before running macro.main() func
        runState = Macro::instance()->getEngine()->dispatchWindowsMessages();

        if( runState == MicroMacro::ERR_OK )
        {   // Run main callback
            runState = Macro::instance()->getEngine()->runMain();
        }

        if( runState == MicroMacro::ERR_CLOSE )
        {   // Script requested to end
            // Reset text color (just in case)
            SetConsoleTextAttribute(Macro::instance()->getAppHandle(), Macro::instance()->getConsoleDefaultAttributes());

            printf("\nScript requested termination.\n");
            return;
        }
        else if( runState != MicroMacro::ERR_OK )
        {   // An actual error occurred
            LuaEngine *pEngine = Macro::instance()->getEngine();

            resetFontRendering();
            std::string lastErrorMessage = pEngine->getLastErrorMessage();
            this->renderErrorMessage(runState, lastErrorMessage.c_str(), "Error in main loop");
            Logger::instance()->add("%s", lastErrorMessage.c_str());

            // Pass to event function
            MicroMacro::Event *pe = new MicroMacro::Event;
            pe->type = MicroMacro::EVENT_ERROR;
            MicroMacro::EventData ced;
            ced.setValue(pEngine->getLastErrorMessage());
            pe->data.push_back(ced);
            pEngine->runEvent(pe);

            return;
        }

        // Don't waste CPU cycles
        if( yieldTimeSlice )
            Sleep(1);
        else
            Sleep(0);
    }
}
