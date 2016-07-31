--[[
	Returns true if the given table appears to be an array
	ie. it has numerical indicies
--]]
local function isArray(tab)
	local i	=	0;
	for v in pairs(tab) do
		i	=	i + 1;
		if( type(tab[i]) == 'nil' ) then
			return false;
		end
	end

	return true;
end

--[[
	Escapes a string for use in a JSON value
--]]
local function escapeStr(str)
	local replacements = {
		["\n"]	=	"\\n",
		["\b"]	=	"\\b",
		["\f"]	=	"\\f",
		["\r"]	=	"\\r",
		["\t"]	=	"\\t",
	};
	str	=	string.gsub(str, "%c", function(char)
		return replacements[char] or string.format("\\u%04x", string.byte(char));
	end);

	str	=	string.gsub(str, "\"", "\\\"");
	str	=	string.gsub(str, "\\", "\\\\");
	str	=	string.gsub(str, "/", "\\/");
	return str;
end

--[[
	Returns a JSON class representation of a given table
]]
local function tabToClass(tab)
	local pairsStr = '';
	for i,v in pairs(tab) do
		local name	=	"\"" .. escapeStr(i) .. "\"";
		local value	=	json_encode(v);

		if( string.len(pairsStr) > 0 ) then pairsStr = pairsStr .. ',' end;
		pairsStr	=	pairsStr .. sprintf("%s:%s", name, value);
	end

	return '{' .. pairsStr .. '}';
end

--[[
	Returns a JSON array representation of a given table
--]]
local function tabToArray(tab)
	local pairsStr = '';
	for i,v in pairs(tab) do
		local value	=	json_encode(v);

		if( string.len(pairsStr) > 0 ) then pairsStr = pairsStr .. ',' end;
		pairsStr	=	pairsStr .. value;

	end

	return '[' .. pairsStr .. ']';
end

--[[
	Returns a JSON encoded copy of the given item
--]]
function json_encode(item)
	local t = type(item);
	if( t == 'table' ) then
		if( isArray(item) ) then
			return tabToArray(item);
		else
			return tabToClass(item);
		end
	end

	if( t == 'nil' or item == "null" ) then
		return 'null';
	end

	if( t == 'number' ) then
		return item;
	end

	if( t == 'string' ) then
		return "\"" .. escapeStr(item) .. "\"";
	end

	if( t == 'boolean' ) then
		if( item ) then
			return 'true';
		else
			return 'false';
		end
	end



	return sprintf("\"Unhandled type: %s\"", t);
end