Collection	=	class.new();

local __protected = {'parent'};

function Collection:constructor(items)
	self.items = {};
	if( type(items) == 'table' ) then
		for i,v in pairs(items) do
			self.items[i]	=	v;
		end
	end
end

-- Return the number of items behind held
function Collection:count()
	return #self.items;
end

-- Chunks the items into a set of smaller tables with a max size of 'count' each
function Collection:chunk(count)
	local totalSize = #self.items;
	local current = 0;

	local chunks = {};
	while current < totalSize do
		local chunk = {};
		for i = current+1,current+count do
			table.insert(chunk, self.items[i]);
			current = i;
		end

		table.insert(chunks, chunk);
	end
	return chunks;
end

-- Returns the first item
function Collection:first(value)
	return self.items[1];
end

-- Returns the last item
function Collection:last(value)
	return self.items[#self.items];
end

-- Returns true if there are no items, otherwise returns false
function Collection:isEmpty()
	return (#self.items == 0);
end

-- Returns the max of an element within items
function Collection:max(field)
	local _max;
	for i,v in pairs(self.items) do
		local n = tonumber(v[field]);
		if( n ~= nil ) then
			if( _max == nil or _max and n > _max ) then
				_max = n;
			end
		end
	end
	return _max;
end

-- Returns the min of an element within items
function Collection:min(field)
	local _min;
	for i,v in pairs(self.items) do
		local n = tonumber(v[field]);
		if( n ~= nil ) then
			if( _min == nil or _min and n < _min ) then
				_min = n;
			end
		end
	end
	return _min;
end

-- Returns the sum of an element within items
function Collection:sum(field)
	local sum = 0;
	for i,v in pairs(self.items) do
		local n = tonumber(v[field]);
		if( n ~= nil ) then
			sum = sum + n;
		end
	end
	return sum;
end

-- Returns the average of an element within items
function Collection:avg(field)
	return self:sum(field) / self:count(field);
end

-- Returns a collection sorted by a given field in either 'asc' (ascending) or 'desc' (descending) order
function Collection:sortBy(field, direction)
	direction = direction or 'asc'; -- Default ascending

	local function asc(a, b)
		return a[field] < b[field];
	end

	local function desc(a, b)
		return a[field] > b[field];
	end

	local newCollection = Collection(self.items);
	if( direction == 'asc' ) then
		table.sort(newCollection.items, asc);
	elseif( direction == 'desc' ) then
		table.sort(newCollection.items, desc);
	end

	return newCollection;
end

-- Returns a collection in opposite order (last item becomes first, first item becomes last)
function Collection:reverse()
	local newCollection = Collection();
	for i = #self.items,1,-1 do
		table.insert(newCollection.items, self.items[i]);
	end
	return newCollection;
end

-- Returns all items in this collection as a standard table
function Collection:toTable()
	return self.items;
end

-- Returns true if the collection has a field/value pair, else false
function Collection:has(field, value)
	for i,v in pairs(self.items) do
		if( v[field] ) then
			if( v[field] == value ) then
				return true;
			end
		end
	end
	return false;
end

-- Returns a new collection where each field must be unique (first come first serve, others are removed)
function Collection:unique(field)
	local newCollection = Collection();
	for i,v in pairs(self.items) do
		if( not newCollection:has(field, v[field]) ) then
			table.insert(newCollection.items, v);
		end
	end

	return newCollection;
end


function Collection:where(field, expression, value)
	-- If we're only given field and value, assume = expression
	if( value == nil and expression ) then
		value		=	expression;
		expression	=	'==';
	end

	-- Returns true/false of "var expr value" (ie. a = 1, where)
	local function checkExpression(var, expr, value)
		if( expr == '==' ) then
			if( var == value ) then
				return true;
			end
		elseif( expr == '~=' ) then
			if( var ~= value ) then
				return true;
			end
		elseif( expr == '<' ) then
			if( var < value ) then
				return true;
			end
		elseif( expr == '<=' ) then
			if( var <= value ) then
				return true;
			end
		elseif( expr == '>' ) then
			if( var > value ) then
				return true;
			end
		elseif( expr == '>=' ) then
			if( var >= value ) then
				return true;
			end
		end

		return false;
	end


	local newCollection = Collection();
	for i,v in pairs(self.items) do
		-- Only check sub-tables
		if( type(v) == 'table' ) then
			if( checkExpression(v[field], expression, value) ) then
				table.insert(newCollection.items, v);
			end
		end
	end
	return newCollection;
end

-- Returns a collection where 'field' must be one of 'values' (a table of acceptable values)
function Collection:whereIn(field, values)
	local newCollection = Collection();
	for i,v in pairs(self.items) do
		-- Only check sub-tables
		if( type(v) == 'table' ) then
			for j,value in pairs(values) do
				if( v[field] == value ) then
					table.insert(newCollection.items, v);
					break;
				end
			end
		end
	end
	return newCollection;
end

-- Returns a collection where 'field' must NOT be one of 'values' (a table of unacceptable values)
function Collection:whereNotIn(field, values)
	local newCollection = Collection();
	for i,v in pairs(self.items) do
		-- Only check sub-tables
		if( type(v) == 'table' ) then
			local keep = true;
			for j,value in pairs(values) do
				if( v[field] == value ) then
					keep = false;
					break;
				end
			end

			if( keep ) then
				table.insert(newCollection.items, v);
			end
		end
	end
	return newCollection;
end

-- Returns a table of the values of each item in the collection under a field
function Collection:lists(field)
	local tab = {};
	for i,v in pairs(self.items) do
		if( v[field] ) then
			table.insert(tab, v[field]);
		end
	end

	return tab;
end



-- Metamethods below.
local meta = getmetatable(Collection);

-- If we call pairs(Collection), redirect to its' fields
function meta:__pairs()
	return pairs(self.items);
end

function meta:__ipairs()
	return ipairs(self.items);
end

function meta:__tostring()
	local str = "{\n";

	local first = true;
	for i,v in pairs(self.items) do
		if( first ) then
			first = false;
		else
			str = str .. ',\n';
		end

		local indexStr;
		if( type(i) == 'string' ) then
			indexStr = i;
		elseif( type(i) == 'number' ) then
			indexStr = '[' .. i .. ']';
		end

		if( type(v) == 'string' ) then
			str = str .. sprintf('\t%s = \"%s\"', indexStr, string.gsub(v, "\"", '\\\"'));
		elseif( type(v) == 'number' ) then
			str = str .. sprintf('\t%s = %d', indexStr, v);
		elseif( type(v) == 'table' ) then
			str = str .. sprintf('\t%s = %s', indexStr, tostring(v));
		end

		--str = str .. "\n";
	end
	str = str .. '\n}';
	return str;
end