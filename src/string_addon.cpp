/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include "string_addon.h"
#include "error.h"
#include "rng.h"

#include <string>
#include <sstream>
#include <algorithm>
#include <string.h>

#include "wininclude.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

int String_addon::regmod(lua_State *L)
{
    lua_getglobal(L, STRING_MODULE_NAME);

    lua_pushcfunction(L, String_addon::explode);
    lua_setfield(L, -2, "explode");

    lua_pushcfunction(L, String_addon::implode);
    lua_setfield(L, -2, "implode");

    lua_pushcfunction(L, String_addon::trim);
    lua_setfield(L, -2, "trim");

    lua_pushcfunction(L, String_addon::random);
    lua_setfield(L, -2, "random");

    lua_pushcfunction(L, String_addon::toUnicode);
    lua_setfield(L, -2, "toUnicode");

    lua_pushcfunction(L, String_addon::likeness);
    lua_setfield(L, -2, "likeness");

    lua_pop(L, 1); // Pop module off stack

    return MicroMacro::ERR_OK;
}

/*  string.explode(string str, string delim)
    Returns:    table

    Splits string 'str' by delimiter 'delim'.
    Returns results as a table.
*/
int String_addon::explode(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);
    checkType(L, LT_STRING, 2);
    std::string str = (char *)lua_tostring(L, 1);
    size_t delimLen;
    const char *delim = lua_tolstring(L, 2, &delimLen);

    lua_newtable(L);
    unsigned int key = 1;
    while(true)
    {
        std::size_t found = str.find(delim);

        if( str.size() > 0 )
        {
            // Push the string.
            lua_pushinteger(L, key); // Key
            lua_pushstring(L, str.substr(0, found).c_str()); // Value
            str = str.substr(found + delimLen); // Set it
            lua_settable(L, -3);
            ++key;
        }

        if( found == std::string::npos )
            break;
    }

    return 1;
}

/*  string.implode(table str, string glue)
    Returns:    table

    Does the opposite of explode; joins a table of values
    together by separating values with 'glue'
*/
int String_addon::implode(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_STRING, 2);

    const char *glue = lua_tostring(L, 2);
    std::string holder = "";
    lua_pushnil(L);
    bool first = true;
    while( lua_next(L, 1) )
    {
        if( first )
            first = false;
        else
            holder += glue;

        if( lua_isstring(L, -1) ) {
            const char *nextString = lua_tostring(L, -1);
            holder += nextString;
        }

        lua_pop(L, 1); // So we can next
    }

    lua_pushstring(L, holder.c_str());
    return 1;
}

/*  string.trim(string str)
    Returns:    string

    "Trim" whitespace off head and tail of string 'str', return result.
*/
int String_addon::trim(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);
    std::string str = (char *)lua_tostring(L, 1);

    // Strip spaces from both ends
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    str.erase(std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), str.end());

    lua_pushstring(L, str.c_str());
    return 1;
}

/*  string.random(string type, number length)
    Returns:    string

    Creates a random string based on the given type
    and with 'length' characters.
    'type' should be:
        "alnum"     Alpha-numeric (both upper and lower case) characters
        "letters"   Letters only (both upper and lower case)
        "numbers"   Numeric characters only
*/
// TODO: Add table option for type
int String_addon::random(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);
    checkType(L, LT_NUMBER, 2);

    static char alnum[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
                           'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a',
                           'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                           'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6',
                           '7', '8', '9', 0
                          };
    static char letters[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
                             'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a',
                             'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                             'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 0
                            };
    static char numbers[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 0};

    std::string type = (char *)lua_tostring(L, 1);
    unsigned int len = lua_tointeger(L, 2);
    char *validChars = NULL;

    if( type == "alnum" )
        validChars = alnum;
    else if( type == "letters" )
        validChars = letters;
    else if( type == "numbers" )
        validChars = numbers;
    else // Default alnum
        validChars = alnum;

    unsigned int validCharsLen = strlen(validChars);
    std::string buff = "";
    for(unsigned int i = 0; i < len; i++)
        buff += validChars[::random(0, validCharsLen)];

    lua_pushstring(L, buff.c_str());
    return 1;
}

/*  string.toUnicode(string str)
    Returns:    string

    Attempt to convert the input string 'str' to a
    wide string.
*/
int String_addon::toUnicode(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);

    size_t origStrLen;
    std::string origStr = (char *)lua_tolstring(L, 1, &origStrLen);
    std::wstring wStr = std::wstring(origStr.begin(), origStr.end());
    lua_pushlstring(L, (char*)wStr.c_str(), wStr.size()*sizeof(wchar_t));

    return 1;
}

/*  string.likeness(string str1, string str2)
    Returns:    number

    Returns the percentage detailing how alike two strings are.
    This implementation uses the Levenshtein distance.
*/
#ifndef MIN3
    #define MIN3(a, b, c) ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))
#endif
int String_addon::likeness(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);

    size_t len1;
    size_t len2;
    char *str1 = (char *)lua_tolstring(L, 1, &len1);
    char *str2 = (char *)lua_tolstring(L, 2, &len2);

    size_t largestLen = std::max(len1, len2);
    float percent;
    int differences;

    if( largestLen == 0 )
    {   // Both empty
        percent     =   100.0;
        differences =   0;
    }
    else
    {
        // Build out a matrix of differences
        // This is heavily based on the code found here:
        // https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C.2B.2B
        unsigned int column[len1 + 1];
        unsigned int x, y, lastdiag, olddiag;
        for(y = 1; y <= len1; y++)
            column[y] = y;
        for(x = 1; x <= len2; x++)
        {
            column[0] = x;
            for(y = 1, lastdiag = x - 1; y <= len1; y++)
            {
                olddiag     =   column[y];
                column[y]   =   MIN3(column[y] + 1, column[y - 1] + 1, lastdiag + (str1[y - 1] == str2[x - 1] ? 0 : 1));
                lastdiag    =   olddiag;
            }
        }

        // Now we can compute how different the strings are
        differences =   column[len1];
        percent     =   float(largestLen - differences) / largestLen * 100;
    }

    lua_pushnumber(L,   percent);
    lua_pushinteger(L,  differences);
    return 2;
}
