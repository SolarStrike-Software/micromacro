/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include "filesystem_lua.h"
#include "filesystem.h"
#include "error.h"
#include "wininclude.h"
#include "strl.h"
#include "macro.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <sys/stat.h>

int Filesystem_lua::regmod(lua_State *L)
{
    static const luaL_Reg _funcs[] = {
        {"getFileName", Filesystem_lua::getFileName},
        {"getFilePath", Filesystem_lua::getFilePath},
        {"directoryExists", Filesystem_lua::directoryExists},
        {"fileExists", Filesystem_lua::fileExists},
        {"getDirectory", Filesystem_lua::getDirectory},
        {"isDirectory", Filesystem_lua::isDirectory},
        {"createDirectory", Filesystem_lua::createDirectory},
        {"fixSlashes", Filesystem_lua::fixSlashes},
        {"getOpenFileName", Filesystem_lua::getOpenFileName},
        {"getSaveFileName", Filesystem_lua::getSaveFileName},
        {"getCWD", Filesystem_lua::getCWD},
        {"setCWD", Filesystem_lua::setCWD},
        {NULL, NULL}
    };

    luaL_newlib(L, _funcs);
    lua_setglobal(L, FILESYSTEM_MODULE_NAME);

    return MicroMacro::ERR_OK;
}

/*  filesystem.getFileName(string path)
    Returns:    string

    Returns just the filename section of a full file path.
*/
int Filesystem_lua::getFileName(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);

    const char *path = lua_tostring(L, 1);

    lua_pushstring(L, ::getFileName(path).c_str());
    return 1;
}

/*  filesystem.getFilePath(string path)
    Returns:    boolean

    Returns just the path section of a full file path.
*/
int Filesystem_lua::getFilePath(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);

    const char *path = lua_tostring(L, 1);

    lua_pushstring(L, ::getFilePath(path, false).c_str());
    return 1;
}

/*  filesystem.directoryExists(string path)
    Returns:    boolean

    If the given directory exists, returns true.
    Else, returns false.
*/
int Filesystem_lua::directoryExists(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);

    const char *path = lua_tostring(L, 1);
    bool exists = ::directoryExists(path);

    lua_pushboolean(L, exists);
    return 1;
}

/*  filesystem.fileExists(string path)
    Returns:    boolean

    If the given file exists, returns true.
    Else, returns false.
*/
int Filesystem_lua::fileExists(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);

    const char *path = lua_tostring(L, 1);
    bool exists = ::fileExists(path);

    lua_pushboolean(L, exists);
    return 1;
}

/*  filesystem.getDirectory(string path)
    Returns (on success):   table (of strings)
    Returns (on failure):   nil

    If the given directory does not exist, this function fails.
    Otherwise, returns a table of filenames & directories
    contained in this path. This does not include "." and ".."
*/
int Filesystem_lua::getDirectory(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);

    const char *path = lua_tostring(L, 1);
    std::vector<std::string> files = ::getDirectory(path);

    if( files.empty() )
        return 0;

    lua_newtable(L);
    for(unsigned int i = 0; i < files.size(); i++)
    {
        lua_pushnumber(L, i + 1); // Key
        lua_pushstring(L, files.at(i).c_str()); // Value
        lua_settable(L, -3);
    }
    return 1;
}

/*  filesystem.isDirectory(string path)
    Returns: boolean

    If the given path is a directory, returns true.
    Else, returns false.
*/
int Filesystem_lua::isDirectory(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    const char *path = lua_tostring(L, 1);

    struct stat dstat;
    stat(path, &dstat);
    bool isDir = S_ISDIR(dstat.st_mode);

    lua_pushboolean(L, isDir);
    return 1;
}

/*  filesystem.createDirectory(string path)
    Returns:    boolean

    Creates a directory at 'path'. Returns true on success, else false.
*/
int Filesystem_lua::createDirectory(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);
    const char *path = lua_tostring(L, 1);
    bool success;

    std::string tempPath = ::fixSlashes(path, SLASHES_TO_WINDOWS);
    size_t startPos = 0;
    size_t nextPos = 0;

    /* If given a full path, then start after the drive name */
    if( tempPath.substr(1, 2).compare(":\\") == 0 )
        nextPos = 2;

    while( true )
    {
        nextPos = tempPath.find("\\", nextPos + 1);

        /* Extract the name of the next directory in line */
        std::string directoryName;
        if( nextPos == std::string::npos )
            directoryName = tempPath.substr(startPos);
        else
            directoryName = tempPath.substr(startPos, nextPos - startPos);

        std::string currentPath = tempPath.substr(0, startPos) + directoryName;
        startPos = nextPos + 1;


        if( !::directoryExists(currentPath.c_str()) )
        {
            SECURITY_ATTRIBUTES attribs;
            attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
            attribs.bInheritHandle = false;
            attribs.lpSecurityDescriptor = NULL;
            success = CreateDirectory(currentPath.c_str(), &attribs);
            if( !success )
                break;
        }

        if( nextPos == std::string::npos || !success )
            break;
    }

    lua_pushboolean(L, success);
    return 1;
}

/*  filesystem.fixSlashes(string path [, boolean posix])
    Returns:    string

    Convert slashes within 'path' to / or \.
    If posix is true (default), converts backslashes(\)
    to forward slashes(/). Else, converts forward slashes
    to backslashes.
*/
int Filesystem_lua::fixSlashes(lua_State *L)
{
    int top = lua_gettop(L);
    if( top != 1 && top != 2 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);
    std::string path = (char *)lua_tostring(L, 1);
    bool posix = true;

    if( top == 2 )
    {
        checkType(L, LT_BOOLEAN, 2);
        posix = lua_toboolean(L, 2);
    }

    int slashes = SLASHES_TO_STANDARD;
    if( !posix )
        slashes = SLASHES_TO_WINDOWS;

    lua_pushstring(L, ::fixSlashes(path, slashes).c_str());
    return 1;
}

/*  filesystem.getOpenFileName([string defaultfilename [, string filter]])
    Returns (on success):   string
    Returns (on failure):   nil

    Displays the standard open file dialog.
    'defaultfilename' can contain a full path to the default file.
    'filter' should be properly formatted: Split key/value pairs with NULL, terminate with double NULL

    If the user cancels the dialog, this function fails and returns nil.
*/
int Filesystem_lua::getOpenFileName(lua_State *L)
{
    int top = lua_gettop(L);
    if( top > 2 )
        wrongArgs(L);

    size_t defaultFilenameLen = 0;
    size_t filterLen = 0;
    char *defaultFilename = (char *)"";
    char *filter = NULL;

    if( top >= 1 )
        checkType(L, LT_STRING | LT_NIL, 1);
    if( top >= 2 )
        checkType(L, LT_STRING | LT_NIL, 2);

    if( top >= 1 && lua_isstring(L, 1) )
        defaultFilename = (char *)lua_tolstring(L, 1, &defaultFilenameLen);

    if( top >= 2 && lua_isstring(L, 2) )
        filter = (char *)lua_tolstring(L, 2, &filterLen);
    else
        filter = (char *)"All Files\0*.*\0Lua files\0*.lua\0\0";

    // Make sure to use backslashes!
    defaultFilename = (char *)::fixSlashes(defaultFilename, SLASHES_TO_WINDOWS).c_str();

    // We will use these buffers to store some data later.
    char cwdBuffer[MAX_PATH + 1];
    char fileBuffer[MAX_PATH + 1];
    char pathBuffer[MAX_PATH + 1];

    /* NOTE: The dialog will modify the CWD, so we must restore it when done. */
    GetCurrentDirectory(MAX_PATH, (LPTSTR)&cwdBuffer);

    // Copy some default data into the buffers, prep OFN struct
    strlcpy(fileBuffer, ::getFileName(defaultFilename).c_str(), MAX_PATH);
    if( ::getFilePath(defaultFilename, false) == "" )
    {   // Assume scripts directory
        std::string buff = cwdBuffer;
        buff += "/";
        buff += Macro::instance()->getSettings()->getString(CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY);
        strlcpy(pathBuffer, buff.c_str(), MAX_PATH);
    }
    else
        strlcpy(pathBuffer, ::fixSlashes(::getFilePath(defaultFilename, true), SLASHES_TO_WINDOWS).c_str(), MAX_PATH);

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

    // Show dialog
    int retCount = 0;
    int success = GetOpenFileName(&ofn);
    if( success )
    {   // User clicked OK
        lua_pushstring(L, ofn.lpstrFile);
        retCount = 1;
    }
    else
    {   // User clicked cancel
        SetCurrentDirectory((LPCTSTR)&cwdBuffer);
        retCount = 0;
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

    return retCount;
}

/*  filesystem.getSaveFileName([string defaultfilename [, string filter]])
    Returns (on success):   string
    Returns (on failure):   nil

    Same as getOpenFileName, except with a save dialog.
    See getOpenFileName for more information.
*/
int Filesystem_lua::getSaveFileName(lua_State *L)
{
    int top = lua_gettop(L);
    if( top > 2 )
        wrongArgs(L);

    size_t defaultFilenameLen = 0;
    size_t filterLen = 0;
    char *defaultFilename = (char *)"";
    char *filter = NULL;

    if( top >= 1 )
        checkType(L, LT_STRING | LT_NIL, 1);
    if( top >= 2 )
        checkType(L, LT_STRING | LT_NIL, 2);

    if( top >= 1 && lua_isstring(L, 1) )
        defaultFilename = (char *)lua_tolstring(L, 1, &defaultFilenameLen);

    if( top >= 2 && lua_isstring(L, 2) )
        filter = (char *)lua_tolstring(L, 2, &filterLen);
    else
        filter = (char *)"All Files\0*.*\0Lua files\0*.lua\0\0";

    // Make sure to use backslashes!
    defaultFilename = (char *)::fixSlashes(defaultFilename, SLASHES_TO_WINDOWS).c_str();

    // We will use these buffers to store some data later.
    char cwdBuffer[MAX_PATH + 1];
    char fileBuffer[MAX_PATH + 1];
    char pathBuffer[MAX_PATH + 1];

    /* NOTE: The dialog will modify the CWD, so we must restore it when done. */
    GetCurrentDirectory(MAX_PATH, (LPTSTR)&cwdBuffer);

    // Copy some default data into the buffers, prep OFN struct
    strlcpy(fileBuffer, ::getFileName(defaultFilename).c_str(), MAX_PATH);
    if( ::getFilePath(defaultFilename, false) == "" )
    {   // Assume scripts directory
        std::string buff = cwdBuffer;
        buff += "/";
        buff += Macro::instance()->getSettings()->getString(CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY);
        strlcpy(pathBuffer, buff.c_str(), MAX_PATH);
    }
    else
        strlcpy(pathBuffer, ::fixSlashes(::getFilePath(defaultFilename, true), SLASHES_TO_WINDOWS).c_str(), MAX_PATH);

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

    // Show dialog
    int retCount = 0;
    int success = GetSaveFileName(&ofn);
    if( success )
    {   // User clicked OK
        lua_pushstring(L, ofn.lpstrFile);
        retCount = 1;
    }
    else
    {   // User clicked cancel
        SetCurrentDirectory((LPCTSTR)&cwdBuffer);
        retCount = 0;
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

    return retCount;
}

/*  filesystem.getCWD()
    Returns:    string

    Returns the current working directory (CWD).
*/
int Filesystem_lua::getCWD(lua_State *L)
{
    if( lua_gettop(L) != 0 )
        wrongArgs(L);

    char cwdBuffer[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH, (LPTSTR)&cwdBuffer);

    std::string fixed = ::fixSlashes(cwdBuffer, SLASHES_TO_STANDARD);

    lua_pushstring(L, fixed.c_str());
    return 1;
}

/*  filesystem.setCWD(string path)
    Returns:    nil

    Modify the current working directory (CWD) to the given path.
*/
int Filesystem_lua::setCWD(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);
    std::string newCWD = lua_tostring(L, 1);

    newCWD = ::fixSlashes(newCWD, SLASHES_TO_WINDOWS);

    bool success = SetCurrentDirectory(newCWD.c_str());
    lua_pushboolean(L, success);
    return 1;
}
