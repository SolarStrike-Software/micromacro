Model	=	class.new();

local __protected = {'parent'};

function Model:constructor(fields)
	self.fields = {};
	if( type(fields) == 'table' ) then
		for i,v in pairs(fields) do
			self.fields[i]	=	v;
		end
	end
end


-- Metamethods below.
local meta = getmetatable(Model);


-- Whenever we try to access a Model.<somevar>
function meta:__index(name)
	return rawget(self.fields, name);
end

-- Whenever we try to assign a Model.<somevar>
function meta:__newindex(name, value)
	if( not table.find(__protected, name) ) then
		if( name == 'fields' ) then
			rawset(self, name, value);
		else
			rawset(self.fields, name, value);
		end
	end
end

-- If we call pairs(Model), redirect to its' fields
function meta:__pairs()
	return pairs(self.fields);
end

function meta:__ipairs()
	return ipairs(self.fields);
end

function meta:__tostring()
	local str = '{';

	local first = true;
	for i,v in pairs(self.fields) do
		if( first ) then
			first = false;
		else
			str = str .. ', ';
		end

		local indexStr;
		if( type(i) == 'string' ) then
			indexStr = i;
		elseif( type(i) == 'number' ) then
			indexStr = '[' .. i .. ']';
		end

		if( type(v) == 'string' ) then
			str = str .. sprintf('%s = \"%s\"', indexStr, string.gsub(v, "\"", '\\\"'));
		elseif( type(v) == 'number' ) then
			str = str .. sprintf('%s = %d', indexStr, v);
		elseif( type(v) == 'table' ) then
			str = str .. sprintf('%s = %s', indexStr, tostring(v));
		end
	end
	str = str .. '}';
	return str;
end