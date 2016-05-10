--[[
	Auto-generate encrypted strings for MicroMacro compilation.
	You may want to attach this to your compiler's build steps.

	Command: lua "$(PROJECT_DIR)src\gen.encstrings.lua"

	It will only generate a new header file for you at a minimum
	of 24 hours since last generation. If you add/change any of
	the strings in 'stringList', you should force regeneration.
	To do that, simply delete your local copy of src/encstring.h
	and re-run the script.
]]

local out_filename = "src\\encstring.h";
local stringList =
{
	basicTitle			=	"MicroMacro v%ld.%02ld.%ld",
	logVersionFmt		=	"MicroMacro version %s (%s) %s\n",
	website				=	"SolarStrike Software\thttp://www.solarstrike.net",
	taskIconTitle		=	"MicroMacro",
}



-- Look up our last generation time and see if it is really old or not.
local readFile = io.open(out_filename, "r");
local genNeeded = true;

if( readFile ) then	-- If the file doesn't exist, generate a new one.
	for line in readFile:lines() do
		--//	Generated:	1424296062
		local gentime = string.match(line, "//%s+Generated:%s+(%d+)");
		if( gentime ) then
			local diffTime = os.time() - gentime;

			if( diffTime < 24*60*60 ) then	-- If generated in less than 1 day, skip it.
				genNeeded = false;
			end
			break;
		end
	end
end

if( not genNeeded ) then
	print("Enc strings are new; we won't re-generate them.");
	return
end



local header = [[
/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

//	Generated:	_$GENTIME$_

#ifndef ENCSTRING_H
#define ENCSTRING_H

	#include <stdlib.h>

	namespace EncString
	{
		size_t reveal(char *, size_t, const int *);
		const unsigned long enckey = _$KEY$_;

]]



local footer = [[
	}
#endif
]]


math.randomseed(os.time())
math.random(1);						-- Ensure better randomness
local enckey = math.random(2,999999999);


-- Change any template values.
header = string.gsub(header, "_$GENTIME$_", os.time());
header = string.gsub(header, "_$KEY$_", tostring(enckey));

print("Auto-generating file", out_filename);
print("Lua version", _VERSION);
local outfile = io.open(out_filename, "w");
assert(outfile);

outfile:write(header);
print("Enc enckey:", enckey);


local function bxor(a,b)
	local r = 0
	for i = 0, 31 do
		local x = a / 2 + b / 2;

		if x ~= math.floor (x) then
			r = r + 2^i;
		end

		a = math.floor (a / 2);
		b = math.floor (b / 2);
	end
	return r
end

print("Generating encrypted strings...");
for i,v in pairs(stringList) do
	local valStr = "";
	for j = 1,#v do
		local origC = string.byte(v:sub(j,j));
		local encC;

		if( _VERSION == "Lua 5.1" ) then
			encC = bxor(origC, enckey);
		elseif( _VERSION == "Lua 5.2" ) then
			encC = bit32.bxor(origC, enckey);
		elseif( _VERSION == "Lua 5.3" ) then
			--encC = origC ~ enckey;
			print("origC:", origC, "enckey:", enckey);
			local cmd = "return " .. origC .. " ~ " ..enckey;	-- Store it as a string so < 5.3 don't gag on syntax
			local chunk = load(cmd);
			if( chunk ) then
				encC = chunk();
			end
		end


		valStr = valStr ..string.format("0x%x", encC);

		if( j % 8 == 0 ) then
			valStr = valStr .. ",\n\t\t\t";
		else
			valStr = valStr .. ", "
		end
	end

	valStr = valStr .. "0";
	outfile:write(string.format("\t\tconst int %s[] = {\n\t\t\t%s};\n", i, valStr));
end

outfile:write(footer);