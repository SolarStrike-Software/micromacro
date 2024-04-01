/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include "sqlite_lua.h"
#include "error.h"
#include "macro.h"
#include "luatypes.h"
#include "strl.h"

using MicroMacro::SQLResult;
using MicroMacro::SQLField;

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <sqlite3.h>

using MicroMacro::SQLiteDb;

int Sqlite_lua::regmod(lua_State *L)
{
    static const luaL_Reg _funcs[] = {
        {"open", Sqlite_lua::open},
        {"close", Sqlite_lua::close},
        {"execute", Sqlite_lua::execute},
        {NULL, NULL}
    };

    luaL_newlib(L, _funcs);
    lua_setglobal(L, SQLITE_MODULE_NAME);

    return MicroMacro::ERR_OK;
}

// Used internally only - Don't register
int Sqlite_lua::callback(void *outvec, int argc, char **argv, char **colName)
{
    std::vector<SQLResult> *results = (std::vector<SQLResult>*)(outvec);
    SQLResult   result;
    for(int i = 0; i < argc; i++)
    {
        SQLField    field;
        field.name      =   colName[i];
        field.value     =   (argv[i] ? argv[i] : "NULL");
        result.fields.push_back(field);
    }
    results->push_back(result);

    return 0;
}


/*  sqlite.open(string filename)
    Returns:    boolean

    Opens an SQLite DB. Returns a SQLite handle(class) on success.
    Returns nil on failure.
*/
int Sqlite_lua::open(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_STRING, 1);

    std::string filename = lua_tostring(L, 1);

    // Create a new, empty sqlite3 DB struct on the Lua stack
    SQLiteDb *pDb = static_cast<SQLiteDb *>(lua_newuserdata(L, sizeof(SQLiteDb)));
    luaL_getmetatable(L, LuaType::metatable_sqlitedb);
    lua_setmetatable(L, -2);
    pDb->opened = false;

    // Non-zero = error
    int rc = sqlite3_open(filename.c_str(), &pDb->db);
    if( rc )
    {
        std::string errmsg = sqlite3_errmsg(pDb->db);
        const char *fmt = "SQLite error occurred opening database: %s\n";
        #ifdef DISPLAY_DEBUG_MESSAGES
        fprintf(stderr, fmt, errmsg.c_str());
        #endif

        lua_pop(L, 1); // Pop our resource (pDb) off the stack.
        pushLuaErrorEvent(L, fmt, errmsg.c_str());
        return 0;
    }

    pDb->opened = true;
    return 1;
}

/*  sqlite.close(class sqlitedb)
    Returns:    nil

    Closes an opened SQLite DB.
*/
int Sqlite_lua::close(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_USERDATA, 1);

    SQLiteDb *pDb = static_cast<SQLiteDb *>(lua_touserdata(L, 1));
    if( pDb->opened )
        sqlite3_close(pDb->db);

    pDb->opened = false;
    return 0;
}

/*  sqlite.execute(sqlitedb, string)
    Returns:    table of results (on success)
    Returns:    nil + error message (on fail)

    Runs an SQL query on a SQLite DB.
*/
int Sqlite_lua::execute(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_USERDATA, 1);
    checkType(L, LT_STRING, 2);

    SQLiteDb *pDb = static_cast<SQLiteDb *>(lua_touserdata(L, 1));
    if( !pDb->opened )
    {
        lua_pushboolean(L, false);
        return 1;
    }

    // Prep a holding vector for results
    std::vector<SQLResult> results;

    char *errMsg;
    std::string query = lua_tostring(L, 2);
    int rc = sqlite3_exec(pDb->db, query.c_str(), callback, &results, &errMsg);

    if( rc != SQLITE_OK )
    {   // If an error occurs return false + errmsg
        printf("SQLite err: %s\nn", errMsg);
        lua_pushnil(L);
        lua_pushstring(L, errMsg);
        sqlite3_free(errMsg);

        return 2;
    }

    // Dump results as a table of tables
    lua_newtable(L);
    for(unsigned int i = 0; i < results.size(); i++)
    {
        lua_pushinteger(L, i + 1); // Push key
        lua_newtable(L); // Push value
        for(unsigned int j = 0; j < results.at(i).fields.size(); j++)
        {
            lua_pushstring(L, results.at(i).fields.at(j).name.c_str()); // Push key
            lua_pushstring(L, results.at(i).fields.at(j).value.c_str()); // Push value
            lua_settable(L, -3); // Compile into inner table ("row")
        }
        lua_settable(L, -3); // Compile into outer table
    }

    return 1;
}
