/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include "vector3d_lua.h"
#include "luatypes.h"
#include "types.h"
#include "error.h"
#include "strl.h"

#include <math.h>
#include <cmath>

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

using MicroMacro::Vector3d;
using MicroMacro::Quaternion;

const char *LuaType::metatable_vector3d = "vector3d";

int Vector3d_lua::regmod(lua_State *L)
{
    const luaL_Reg meta[] = {
        {"__tostring", tostring},
        {"__add", add},
        {"__sub", sub},
        {"__div", div},
        {"__mul", mul},
        {"__unm", unm},
        {"__len", length},
        {NULL, NULL}
    };

    const luaL_Reg methods[] = {
        {"set", set},
        {"length", length},
        {"normal", normal},
        {"rotateAboutX", rotateAboutX},
        {"rotateAboutY", rotateAboutY},
        {"rotateAboutZ", rotateAboutZ},
        {"rotateAbout", rotateAbout},
        {"dot", dot},
        {"cross", cross},
        {"moveTowards", moveTowards},
        {"lerp", lerp},
        {"slerp", slerp},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LuaType::metatable_vector3d);
    luaL_setfuncs(L, meta, 0);
    luaL_newlib(L, methods);
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1); // Pop table

    return MicroMacro::ERR_OK;
}

int Vector3d_lua::tostring(lua_State *L)
{
    checkType(L, LT_TABLE, 1);
    Vector3d vec = lua_tovector3d(L, 1);

    char buffer[64];
    slprintf(buffer, sizeof(buffer) - 1, "(%0.2f, %0.2f, %0.2f)", vec.x, vec.y, vec.z);

    lua_pushstring(L, buffer);
    return 1;
}

int Vector3d_lua::add(lua_State *L)
{
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);

    Vector3d vec1 = lua_tovector3d(L, 1);
    Vector3d vec2 = lua_tovector3d(L, 2);
    Vector3d result = vec1 + vec2;

    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::sub(lua_State *L)
{
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE | LT_NUMBER, 2);

    Vector3d vec1 = lua_tovector3d(L, 1);
    Vector3d vec2 = lua_tovector3d(L, 2);
    Vector3d result = vec1 - vec2;

    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::mul(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_NUMBER | LT_TABLE, 2);

    Vector3d vec1 = lua_tovector3d(L, 1);
    Vector3d result;
    if( lua_istable(L, 2) )
    {
        Vector3d vec2 = lua_tovector3d(L, 2);
        result = vec1 * vec2;
    }
    else
    {
        double scale = lua_tonumber(L, 2);
        result = vec1 * scale;
    }

    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::div(lua_State *L)
{
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE | LT_NUMBER, 2);

    Vector3d result;

    if( lua_istable(L, 2) )
    {
        Vector3d vec1 = lua_tovector3d(L, 1);
        Vector3d vec2 = lua_tovector3d(L, 2);
        result = vec1 / vec2;
    }
    else
    {
        Vector3d vec1 = lua_tovector3d(L, 1);
        double scale = lua_tonumber(L, 2);
        result = vec1 / scale;
    }

    // Prevent division by zero
    if( std::isnan(result.x) || std::isinf(result.x)
            || std::isnan(result.y) || std::isinf(result.y)
            || std::isnan(result.z) || std::isinf(result.z) )
    {
        pushLuaErrorEvent(L, "Attempt to divide by zero or illegal operation.");
        return 0;
    }

    lua_pushvector3d(L, result);
    return 1;
}

//Unary minus (inverse)
int Vector3d_lua::unm(lua_State *L)
{
    checkType(L, LT_TABLE, 1);

    Vector3d vec = lua_tovector3d(L, 1);
    vec.x = -vec.x;
    vec.y = -vec.y;
    vec.z = -vec.z;

    lua_pushvector3d(L, vec);
    return 1;
}

int Vector3d_lua::set(lua_State *L)
{
    int top = lua_gettop(L);
    if( top != 1 && top != 3 && top != 4 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);

    if( top == 4 )
    {
        checkType(L, LT_NUMBER, 2);
        checkType(L, LT_NUMBER, 3);
        checkType(L, LT_NUMBER, 4);
        lua_setfield(L, 1, "z"); // Set Z to value on top of stack, pop it
        lua_setfield(L, 1, "y"); // Set Y to value on top of stack, pop it
        lua_setfield(L, 1, "x"); // Set X to value on top of stack, pop it
    }
    else if( top == 3 )
    {
        checkType(L, LT_NUMBER, 2);
        checkType(L, LT_NUMBER, 3);
        lua_pushnumber(L, 0); // Push 0 for Z

        lua_setfield(L, 1, "z"); // Set Z to value on top of stack, pop it
        lua_setfield(L, 1, "y"); // Set Y to value on top of stack, pop it
        lua_setfield(L, 1, "x"); // Set X to value on top of stack, pop it
    }

    return 0;
}

int Vector3d_lua::length(lua_State *L)
{
    checkType(L, LT_TABLE, 1);

    Vector3d vec = lua_tovector3d(L, 1);

    lua_pushnumber(L, vec.magnitude());
    return 1;
}

int Vector3d_lua::normal(lua_State *L)
{
    if( lua_gettop(L) != 1 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);

    Vector3d vec = lua_tovector3d(L, 1);

    lua_pushvector3d(L, vec.normal());
    return 1;
}

// Dot product
int Vector3d_lua::dot(lua_State *L)
{
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);

    Vector3d vec1 = lua_tovector3d(L, 1);
    Vector3d vec2 = lua_tovector3d(L, 2);

    lua_pushnumber(L, vec1.dot(vec2));
    return 1;
}

// Cross product
int Vector3d_lua::cross(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);

    Vector3d vec1 = lua_tovector3d(L, 1);
    Vector3d vec2 = lua_tovector3d(L, 2);

    lua_pushvector3d(L, vec1.cross(vec2));
    return 1;
}

int Vector3d_lua::rotateAboutX(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_NUMBER, 2);

    Vector3d vec = lua_tovector3d(L, 1);
    Vector3d axis = Vector3d(1, 0, 0);
    double radAngle = lua_tonumber(L, 2);

    Quaternion rotation(axis, radAngle);
    Vector3d result = rotation * vec;

    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::rotateAboutY(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_NUMBER, 2);

    Vector3d vec = lua_tovector3d(L, 1);
    Vector3d axis = Vector3d(0, 1, 0);
    double radAngle = lua_tonumber(L, 2);

    Quaternion rotation(axis, radAngle);
    Vector3d result = rotation * vec;

    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::rotateAboutZ(lua_State *L)
{
    if( lua_gettop(L) != 2 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_NUMBER, 2);

    Vector3d vec = lua_tovector3d(L, 1);
    Vector3d axis = Vector3d(0, 0, 1);
    double radAngle = lua_tonumber(L, 2);

    Quaternion rotation(axis, radAngle);
    Vector3d result = rotation * vec;

    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::rotateAbout(lua_State *L)
{
    if( lua_gettop(L) != 3 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);
    checkType(L, LT_NUMBER, 3);

    Vector3d vec = lua_tovector3d(L, 1);
    Vector3d axis = lua_tovector3d(L, 2);
    double radAngle = lua_tonumber(L, 3);

    Quaternion rotation(axis, radAngle);
    Vector3d result = rotation * vec;

    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::lerp(lua_State *L)
{
    if( lua_gettop(L) != 3 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);
    checkType(L, LT_NUMBER, 3);

    Vector3d vec = lua_tovector3d(L, 1);
    Vector3d target = lua_tovector3d(L, 2);
    double ratio = lua_tonumber(L, 3);

    Vector3d result = vec + (target - vec) * ratio;
    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::slerp(lua_State *L)
{
    if( lua_gettop(L) != 3 )
        wrongArgs(L);
    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);
    checkType(L, LT_NUMBER, 3);

    Vector3d vec = lua_tovector3d(L, 1);
    Vector3d target = lua_tovector3d(L, 2);
    double ratio = lua_tonumber(L, 3);

    double dot = vec.dot(target);
    dot = std::max(-1.0, std::min(dot, 1.0)); // Clamp between -1 and 1
    float theta = acos(dot) * ratio;
    Vector3d relative = (target - vec * dot).normal();

    Vector3d result = vec * cos(theta) + relative * sin(theta);
    lua_pushvector3d(L, result);
    return 1;
}

int Vector3d_lua::moveTowards(lua_State *L)
{
    if( lua_gettop(L) != 3 )
        wrongArgs(L);

    checkType(L, LT_TABLE, 1);
    checkType(L, LT_TABLE, 2);
    checkType(L, LT_NUMBER, 3);

    Vector3d current = lua_tovector3d(L, 1);
    Vector3d target = lua_tovector3d(L, 2);
    double dist = lua_tonumber(L, 3);
    Vector3d delta;
    Vector3d result;

    // Find the difference
    delta.x = (target.x - current.x);
    delta.y = (target.y - current.y);
    delta.z = (target.z - current.z);


    // Normalize
    result = Vector3d(delta.x, delta.y, delta.z);
    double scale = sqrtf(result.x * result.x + result.y * result.y + result.z * result.z);
    result.x /= scale;
    result.y /= scale;
    result.z /= scale;

    // Scale
    result.x *= dist;
    result.y *= dist;
    result.z *= dist;

    // Add original vector
    result.x += current.x;
    result.y += current.y;
    result.z += current.z;

    // If overshot, clamp to target
    if( dist > 0 && (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z) < dist * dist )
        result = target;

    lua_pushvector3d(L, result);
    return 1;
}


MicroMacro::Vector3d lua_tovector3d(lua_State *L, int index)
{
    Vector3d vec;
    lua_getfield(L, index, "x");
    vec.x = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "y");
    vec.y = lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "z");
    vec.z = lua_tonumber(L, -1);
    lua_pop(L, 1);

    return vec;
}

void lua_pushvector3d(lua_State *L, const Vector3d &vec)
{
    lua_newtable(L);

    luaL_getmetatable(L, LuaType::metatable_vector3d);
    lua_setmetatable(L, -2);

    lua_pushnumber(L, vec.x);
    lua_setfield(L, -2, "x");

    lua_pushnumber(L, vec.y);
    lua_setfield(L, -2, "y");

    lua_pushnumber(L, vec.z);
    lua_setfield(L, -2, "z");
}
