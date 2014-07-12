/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef HASH_LUA
#define HASH_LUA

    #define HASH_MODULE_NAME        "hash"
    #define SHA1_HASH_LEN           20
    #define SHA1_HEX_LEN            40

    typedef struct lua_State lua_State;

    class Hash_lua
    {
        protected:
            static int sha1(lua_State *);
            static int sha1_file(lua_State *);

        public:
            static int regmod(lua_State *);
    };

#endif
