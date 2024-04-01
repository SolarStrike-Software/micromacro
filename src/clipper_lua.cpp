/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include <limits>
#include "clipper/clipper.hpp"
#include "clipper_lua.h"
#include "error.h"
#include "macro.h"

using ClipperLib::Path;
using ClipperLib::Paths;
using ClipperLib::Clipper;
using ClipperLib::ClipperOffset;
using ClipperLib::IntPoint;
using ClipperLib::etClosedPolygon;
using ClipperLib::JoinType;
using ClipperLib::jtMiter;
using ClipperLib::jtRound;
using ClipperLib::jtSquare;
using ClipperLib::pftNonZero;
using ClipperLib::ClipType;
using ClipperLib::ctUnion;
using ClipperLib::ctIntersection;
using ClipperLib::ctDifference;
using ClipperLib::ctXor;
using ClipperLib::ptSubject;

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

double Clipper_lua::pointPrecision  =   16384.0;    // Seems like a good default value

int Clipper_lua::regmod(lua_State *L)
{
    static const luaL_Reg _funcs[] = {
        {"getPointPrecision", Clipper_lua::getPointPrecision},
        {"setPointPrecision", Clipper_lua::setPointPrecision},
        {"offset", Clipper_lua::offset},
        {"merge", Clipper_lua::merge},
        {"pointInPoly", Clipper_lua::pointInPoly},
        {"clean", Clipper_lua::clean},
        {"simplify", Clipper_lua::simplify},
        {NULL, NULL}
    };

    luaL_newlib(L, _funcs);
    lua_setglobal(L, CLIPPER_MODULE_NAME);

    return MicroMacro::ERR_OK;
}

int Clipper_lua::cleanup(lua_State *L)
{
    return 0;
}

// Scale point from double to long, based on our pointPrecision.
// Return false on error (out of numeric limit) or true on success
int Clipper_lua::scalePointToLong(double srcX, double srcY, long &destX, long &destY)
{
    double const min_value = std::numeric_limits<long>::min() / pointPrecision;
    double const max_value = std::numeric_limits<long>::max() / pointPrecision;

    if( srcX < 0 )
    {
        if( srcX < min_value )
            return false;
        destX = static_cast<long>(srcX * pointPrecision - 0.5);
    }
    else
    {
        if( srcX > max_value )
            return false;
        destX = static_cast<long>(srcX * pointPrecision + 0.5);
    }

    if( srcY < 0 )
    {
        if( srcY < min_value )
            return false;
        destY = static_cast<long>(srcY * pointPrecision - 0.5);
    }
    else
    {
        if( srcY > max_value )
            return false;
        destY = static_cast<long>(srcY * pointPrecision + 0.5);
    }

    return true;
}

// Scale point from long to double, based on our pointPrecision.
// Return false on error (out of numeric limit) or true on success
int Clipper_lua::scalePointToDouble(long srcX, long srcY, double &destX, double &destY)
{
    destX = static_cast<double>(srcX / pointPrecision);
    destY = static_cast<double>(srcY / pointPrecision);

    return true;
}

void Clipper_lua::pushPathSolution(lua_State *L, ClipperLib::Path &path)
{
    lua_newtable(L);
    for(unsigned int i = 0; i < path.size(); i++)
    {
        lua_pushinteger(L, i + 1);
        lua_newtable(L);
        // Set fields within this table
        double dx = 0;
        double dy = 0;
        if( !Clipper_lua::scalePointToDouble(path[i].X, path[i].Y, dx, dy) )
        {
            lua_pushliteral(L, "Number out of range");
            lua_error(L);
        }

        // Set X value
        lua_pushstring(L, "x");
        lua_pushnumber(L, dx);
        lua_settable(L, -3);

        // Set Y value
        lua_pushstring(L, "y");
        lua_pushnumber(L, dy);
        lua_settable(L, -3);

        // X/Y are set so now set the point table into the poly table
        lua_settable(L, -3);
    }
}

void Clipper_lua::pushPathsSolution(lua_State *L, ClipperLib::Paths &paths)
{
    lua_newtable(L);
    for(unsigned int i = 0; i < paths.size(); i++)
    {   // for each polygon in this solution
        lua_pushinteger(L, i + 1);
        lua_newtable(L);
        for(unsigned int j = 0; j < paths[i].size(); j ++)
        {   // For each point in this poly
            lua_pushinteger(L, j + 1);
            lua_newtable(L);
            // Set fields within this table
            double dx = 0;
            double dy = 0;
            if( !Clipper_lua::scalePointToDouble(paths[i][j].X, paths[i][j].Y, dx, dy) )
            {
                lua_pushliteral(L, "Number out of range");
                lua_error(L);
            }

            // Set X value
            lua_pushstring(L, "x");
            lua_pushnumber(L, dx);
            lua_settable(L, -3);

            // Set Y value
            lua_pushstring(L, "y");
            lua_pushnumber(L, dy);
            lua_settable(L, -3);

            // X/Y are set so now set the point table into the poly table
            lua_settable(L, -3);
        }
        lua_settable(L, -3);
    }
}




int Clipper_lua::getPointPrecision(lua_State *L)
{
    if( lua_gettop(L) > 0 )
        wrongArgs(L);

    lua_pushnumber(L, Clipper_lua::pointPrecision);
    return 1;
}

int Clipper_lua::setPointPrecision(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_NUMBER, 1);

    double newPrecision =   lua_tonumber(L, 1);

    if( newPrecision < 1.0 )
    {
        lua_pushliteral(L, "Precision must be at least 1.0");
        lua_error(L);
    }

    pointPrecision  =   newPrecision;
    return 0;
}

int Clipper_lua::offset(lua_State *L)
{
    int top = lua_gettop(L);
    if( top != 2 && top != 3 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_NUMBER, 2);

    Path subject;
    Paths solution;
    JoinType joinType = jtSquare;
    double offsetAmount = lua_tonumber(L, 2) * pointPrecision;

    if( top >= 3 )
    {
        checkType(L, LT_STRING, 3);
        std::string szJoinType = lua_tostring(L, 3);
        if( szJoinType.compare("square") == 0 )
            joinType = jtSquare;
        else if( szJoinType.compare("round") == 0 )
            joinType = jtRound;
        else if( szJoinType.compare("miter") == 0 )
            joinType = jtMiter;
        else
        {
            lua_pushliteral(L, "Invalid JoinType given for clipper.offset()");
            lua_error(L);
        }
    }

    // Argument 1 should be a table of points
    // Each point should be a table with an 'x' and 'y' value
    // Iterate over the table
    lua_pushnil(L); // Index = nil for first iteration
    while( lua_next(L, 1) )
    {
        if( lua_istable(L, -1) )
        {
            // Pull our x,y from the table
            lua_getfield(L, -1, "x");
            double x = lua_tonumber(L, -1);
            lua_getfield(L, -2, "y");
            double y = lua_tonumber(L, -1);
            lua_pop(L, 2); // Pop off our x,y

            long sx = 0;    // We need to scale the points to fit some ints
            long sy = 0;
            if( !Clipper_lua::scalePointToLong(x, y, sx, sy) )
            {
                lua_pushliteral(L, "Number out of range");
                lua_error(L);
            }
            subject << IntPoint(sx, sy);
        }
        lua_pop(L, 1); // Pop old value; our index needs to be on top of the stack (so we can lua_next)
    }

    ClipperOffset clipperOffset;
    clipperOffset.AddPath(subject, joinType, etClosedPolygon);  // jtMiter works great for this operation
    clipperOffset.Execute(solution, offsetAmount);

    pushPathsSolution(L, solution);
    return 1;
}

int Clipper_lua::merge(lua_State *L)
{
    int top = lua_gettop(L);
    if( top != 1 && top != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);

    Paths subjects;
    Paths solution;
    ClipType clipType = ctUnion;

    if( top >= 2 )
    {
        checkType(L, LT_STRING, 2);
        std::string clipType = lua_tostring(L, 2);
        if( clipType.compare("union") == 0 )
            clipType = ctUnion;
        else if( clipType.compare("intersection") )
            clipType = ctIntersection;
        else if( clipType.compare("difference") )
            clipType = ctDifference;
        else if ( clipType.compare("xor") )
            clipType = ctXor;
        else
        {
            lua_pushliteral(L, "Invalid clipType passed to clipper.merge()");
            lua_error(L);
        }
    }

    // Argument 1 should be a table of tables of points
    lua_pushnil(L); // Index = nil for first iteration
    while( lua_next(L, 1) )
    {
        if( lua_istable(L, -1) )
        {
            Path subject;
            lua_pushnil(L);
            while( lua_next(L, -2) ) // Table is now -2 because we pushed nil
            {
                if( lua_istable(L, -1) )
                {
                    // Pull our x,y from the table
                    lua_getfield(L, -1, "x");
                    double x = lua_tonumber(L, -1);
                    lua_getfield(L, -2, "y");
                    double y = lua_tonumber(L, -1);
                    lua_pop(L, 2); // Pop off our x,y

                    long sx = 0;    // We need to scale the points to fit some ints
                    long sy = 0;
                    if( !Clipper_lua::scalePointToLong(x, y, sx, sy) )
                    {
                        lua_error(L);
                    }
                    subject << IntPoint(sx, sy);
                }
                lua_pop(L, 1);
            }
            subjects.push_back(subject);
        }
        lua_pop(L, 1);
    }

    Clipper clipper;
    clipper.AddPaths(subjects, ptSubject, true);
    clipper.Execute(clipType, solution, pftNonZero, pftNonZero);

    pushPathsSolution(L, solution);
    return 1;
}

int Clipper_lua::pointInPoly(lua_State *L)
{
    int top = lua_gettop(L);
    if( top != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);

    Path subject;
    IntPoint point;

    // Get X/Y
    lua_getfield(L, 1, "x");
    double x = lua_tonumber(L, -1);
    lua_getfield(L, 1, "y");
    double y = lua_tonumber(L, -1);
    lua_pop(L, 2);

    // Scale point
    long sx = 0;
    long sy = 0;
    if( !Clipper_lua::scalePointToLong(x, y, sx, sy) )
    {
        lua_pushliteral(L, "Number out of range");
        lua_error(L);
    }

    // Store it
    point.X = sx;
    point.Y = sy;


    // Argument 2 should be a table of points
    lua_pushnil(L); // Index = nil for first iteration
    while( lua_next(L, 2) )
    {
        if( lua_istable(L, -1) )
        {
            // Pull our x,y from the table
            lua_getfield(L, -1, "x");
            x = lua_tonumber(L, -1);
            lua_getfield(L, -2, "y");
            y = lua_tonumber(L, -1);
            lua_pop(L, 2); // Pop off our x,y

            if( !Clipper_lua::scalePointToLong(x, y, sx, sy) )
            {
                lua_pushliteral(L, "Number out of range");
                lua_error(L);
            }
            subject << IntPoint(sx, sy);
        }
        lua_pop(L, 1); // Pop old value; our index needs to be on top of the stack (so we can lua_next)
    }

    int result = ClipperLib::PointInPolygon(point, subject);
    lua_pushboolean(L, result == 1); // Whether it is inside the polygon
    lua_pushboolean(L, result == -1); // Whether it is on the polygon

    return 2;
}

int Clipper_lua::clean(lua_State *L)
{
    int top = lua_gettop(L);
    if( top != 1 && top != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);

    Path subject;
    Path solution;
    double distance =   1.415;

    if( top >= 2 )
    {
        checkType(L, LT_NUMBER, 2);
        distance    =   lua_tonumber(L, 2);
    }

    // Argument 1 should be a table of points
    lua_pushnil(L); // Index = nil for first iteration
    while( lua_next(L, 1) )
    {
        if( lua_istable(L, -1) )
        {
            // Pull our x,y from the table
            lua_getfield(L, -1, "x");
            double x = lua_tonumber(L, -1);
            lua_getfield(L, -2, "y");
            double y = lua_tonumber(L, -1);
            lua_pop(L, 2); // Pop off our x,y

            long sx = 0;    // We need to scale the points to fit some ints
            long sy = 0;
            if( !Clipper_lua::scalePointToLong(x, y, sx, sy) )
            {
                lua_pushliteral(L, "Number out of range");
                lua_error(L);
            }
            subject << IntPoint(sx, sy);
        }
        lua_pop(L, 1); // Pop old value; our index needs to be on top of the stack (so we can lua_next)
    }

    ClipperLib::CleanPolygon(subject, solution, distance / pointPrecision);

    pushPathSolution(L, solution);
    return 1;
}

int Clipper_lua::simplify(lua_State *L)
{
    int top = lua_gettop(L);
    if( top != 1 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);

    Path subject;
    Paths solution;

    // Argument 1 should be a table of points
    lua_pushnil(L); // Index = nil for first iteration
    while( lua_next(L, 1) )
    {
        if( lua_istable(L, -1) )
        {
            // Pull our x,y from the table
            lua_getfield(L, -1, "x");
            double x = lua_tonumber(L, -1);
            lua_getfield(L, -2, "y");
            double y = lua_tonumber(L, -1);
            lua_pop(L, 2); // Pop off our x,y

            long sx = 0;    // We need to scale the points to fit some ints
            long sy = 0;
            if( !Clipper_lua::scalePointToLong(x, y, sx, sy) )
            {
                lua_pushliteral(L, "Number out of range");
                lua_error(L);
            }
            subject << IntPoint(sx, sy);
        }
        lua_pop(L, 1); // Pop old value; our index needs to be on top of the stack (so we can lua_next)
    }

    ClipperLib::SimplifyPolygon(subject, solution);

    pushPathsSolution(L, solution);
    return 1;
}
