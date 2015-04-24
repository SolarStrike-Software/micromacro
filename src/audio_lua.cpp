/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifdef AUDIO_ENABLED

#include "audio_lua.h"
#include "types.h"
#include "luatypes.h"
#include "error.h"
#include "strl.h"
#include "event.h"
#include "macro.h"
#include "logger.h"
#include "settings.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>

using MicroMacro::AudioResource;

int Audio_lua::regmod(lua_State *L)
{
	// If the user has opted to disable sound, do nothing!
	int enabled = Macro::instance()->getSettings()->getInt(CONFVAR_AUDIO_ENABLED);
	if( !enabled )
		return MicroMacro::ERR_OK;

	alutInit(0, NULL);
	ALenum error = alGetError();
	if( error != AL_NO_ERROR )
	{ // Throw error
		char buffer[512];
		slprintf(buffer, sizeof(buffer)-1, "Failed to initialize ALUT. Error code %d\n", error);
		fprintf(stderr, buffer);
		Logger::instance()->add(buffer);
		return MicroMacro::ERR_INIT_FAIL;
	}

	static const luaL_Reg _funcs[] = {
		{"load", Audio_lua::load},
		{"unload", Audio_lua::unload},
		{"play", Audio_lua::play},
		{"stop", Audio_lua::stop},
		{"pause", Audio_lua::pause},
		{"getState", Audio_lua::getState},
		{"setLooping", Audio_lua::setLooping},
		{"setVolume", Audio_lua::setVolume},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, AUDIO_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

int Audio_lua::cleanup(lua_State *)
{
	alutExit();
	return MicroMacro::ERR_OK;
}

/*	audio.load(string filename)
	Returns (on success):	userdata (audioresource)
	Returns (on failure):	nil

	Loads a sound file and returns the resource
*/
int Audio_lua::load(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	const char *filename = lua_tostring(L, 1);

	// Create a new, empty AudioResource on the Lua stack
	AudioResource *pResource = static_cast<AudioResource *>(lua_newuserdata(L, sizeof(AudioResource)));
	luaL_getmetatable(L, LuaType::metatable_audioResource);
	lua_setmetatable(L, -2);
	pResource->buffer = AL_NONE;
	pResource->source = AL_NONE;

	// Try loading it
	bool success = true;
	pResource->buffer = alutCreateBufferFromFile(filename);
	if( pResource->buffer == AL_NONE )
		success = false;
	else
	{
		alGenSources(1, &pResource->source);
		alSourcei(pResource->source, AL_BUFFER, pResource->buffer);

		if( alGetError() != AL_NO_ERROR )
			success = false;
	}

	if( !success )
	{ // Throw error
		lua_pop(L, 1); // Pop our resource off the stack.
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failed to load sound resource. Error code %i (%s).",
			errCode, getWindowsErrorString(errCode).c_str());
		return 0;
	}

	return 1;
}

/*	audio.unload(userdata audioresource)
	Returns:	nil

	Unloads an audioresource and sets its buffer & sources to empty (AL_NONE)
*/
int Audio_lua::unload(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	if( pResource->buffer != AL_NONE )
		alDeleteBuffers(1, &pResource->buffer);
	if( pResource->source != AL_NONE )
		alDeleteSources(1, &pResource->source);

	pResource->buffer = AL_NONE;
	pResource->source = AL_NONE;

	return 0;
}

/*	audio.play(userdata audioresource)
	Returns:	nil

	Plays a loaded audioresource
*/
int Audio_lua::play(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	if( pResource->source != AL_NONE )
		alSourcePlay(pResource->source);
	return 0;
}

/*	audio.stop(userdata audioresource)
	Returns:	nil

	Stops an audioresource (if it is playing)
*/
int Audio_lua::stop(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	if( pResource->source != AL_NONE )
		alSourceStop(pResource->source);
	return 0;
}

/*	audio.pause(userdata audioresource)
	Returns:	nil

	Pause an audioresource (if it is playing)
*/
int Audio_lua::pause(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	if( pResource->source != AL_NONE )
		alSourcePause(pResource->source);
	return 0;
}

/*	audio.getState(userdata audioresource)
	Returns:	string [initial|playing|paused|stopped|unknown]

	Returns the state of an audioresource as a string
*/
int Audio_lua::getState(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));

	ALint state;
	alGetSourcei(pResource->source, AL_SOURCE_STATE, &state);

	switch( state )
	{
		case AL_INITIAL:
			lua_pushstring(L, "initial");
		break;
		case AL_PLAYING:
			lua_pushstring(L, "playing");
		break;
		case AL_PAUSED:
			lua_pushstring(L, "paused");
		break;
		case AL_STOPPED:
			lua_pushstring(L, "stopped");
		break;
		default:
			lua_pushstring(L, "unknown");
		break;
	}
	return 1;
}

/*	audio.setLooping(userdata audioresource, boolean looping)
	Returns:	nil

	Sets the "looping" state for an audioresource; true = loop it, false = no looping
*/
int Audio_lua::setLooping(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_BOOLEAN, 2);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));
	bool looping = lua_toboolean(L, 2);

	if( pResource->source != AL_NONE )
	{
		if(looping)
			alSourcei(pResource->source, AL_LOOPING, AL_TRUE);
		else
			alSourcei(pResource->source, AL_LOOPING, AL_FALSE);
	}
	return 0;
}

/*	audio.setVolume(userdata audioresource, number volume)
	Returns:	nil

	Sets the volume for an audioresource. 0 = no volume, 1 = full volume
*/
int Audio_lua::setVolume(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);
	AudioResource *pResource = static_cast<AudioResource *>(lua_touserdata(L, 1));
	float volume = lua_tonumber(L, 2);

	if( pResource->source != AL_NONE )
		alSourcef(pResource->source, AL_GAIN, volume);

	return 0;
}

#endif
