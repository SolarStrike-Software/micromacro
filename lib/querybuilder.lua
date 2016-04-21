QueryBuilder = class.new();

local NULL_substitute = math.mininteger;

function QueryBuilder:constructor()
	self._main_method = 'select';
	self._select = {};
	self._set = {};
	self._values = {};
	self._where = {};
	self._from = '';
	self._limit = nil;
end

function QueryBuilder:escape(str)
	if( type(str) ~= 'string' ) then
		return str;
	end

	-- Escape single quote by doubling it (two single quotes, not one double quote)
	return string.gsub(str, "'", "''");
end

function QueryBuilder:select(targets, escape)
	escape = escape or true;

	if( type(targets) == 'string' ) then
		if( escape ) then
			targets = self:escape(targets);
		end
		table.insert(self._select, targets);
	elseif( type(targets) == 'table' ) then
		for i,v in pairs(targets) do
			if( escape ) then
				v = self:escape(v);
			end
			table.insert(self._select, v);
		end
	end

	self._main_method = 'select';
	return self;
end

function QueryBuilder:update(tab)
	self._main_method = 'update';
	self._from = tab;
	return self;
end

function QueryBuilder:set(field, value, escape)
	escape = escape or true;
	if( escape ) then
		value = self:escape(value);
	end

	if( type(value) == 'nil' ) then
		self._set[field] = NULL_substitute;
	else
		self._set[field] = value;
	end
	return self;
end

function QueryBuilder:delete(tab)
	self._main_method = 'delete';
	self._from = tab;
	return self;
end

function QueryBuilder:from(target)
	self._from = target;
	return self;
end

function QueryBuilder:insert(tab)
	self._main_method = 'insert';
	self._from = tab;
	return self;
end

function QueryBuilder:values(field, value, escape)
	escape = escape or true;
	if( escape ) then
		value = self:escape(value);
	end

	if( type(value) == 'nil' ) then
		self._values[field] = NULL_substitute;
	else
		self._values[field] = value;
	end
	return self;
end

function QueryBuilder:where(_field, _expression, _value, escape)
	escape = escape or true;
	if( escape ) then
		_expression	=	self:escape(_expression);
		_value		=	self:escape(_value);
	end

	-- If we're only given field and value, assume = expression
	if( _value == nil and _expression ) then
		_value		=	_expression;
		_expression	=	'=';
	end

	table.insert(self._where, {
		field		=	_field,
		expression	=	_expression,
		value		=	_value,
	});

	return self;
end

function QueryBuilder:limit(count)
	self._limit = count;
	return self;
end

function QueryBuilder:orderBy(field, direction)
	if( direction == nil ) then
		direction = 'asc';
	end
end

-- For non-main-method specific stuff
function QueryBuilder:formatQuery()
	local query = '';
	-- Build join(s)

	-- Build where statements
	if( #self._where ) then
		local tmp = '';
		local first = true;
		for i,v in pairs(self._where) do
			-- Use 'where' for first entry, 'and' for others
			if( first ) then
				first = false;
				tmp = ' WHERE '
			else
				tmp = tmp .. ' AND ';
			end

			tmp = tmp .. '`' .. v.field .. '` ' .. v.expression .. ' \'' .. v.value .. '\'';
		end
		query = query .. tmp;
	end

	-- Build or where statements



	-- Build like statments

	-- Build limits
	if( self._limit ) then
		query = query .. ' LIMIT ' .. self._limit;
	end

	return query .. ';';
end

function QueryBuilder:get()
	local query = '';

	if( self._main_method == 'delete' ) then
		-- Gen a DELETE query
		query = 'DELETE FROM `' .. self._from .. '`';
	elseif( self._main_method == 'update' ) then
		-- Gen an UPDATE query
		query = 'UPDATE `' .. self._from .. '`';

		local tmp = '';
		if( #self._set ) then
			local first = true;
			for i,v in pairs(self._set) do
				-- Don't put a comma (,) in front of the first entry
				if( first ) then
					first = false;
				else
					tmp = tmp .. ',';
				end

				-- If the value is NULL, don't encapsulate it
				tmp = tmp .. '`' .. i .. '` = ';
				if( v == NULL_substitute ) then
					tmp = tmp .. 'NULL';
				else
					tmp = tmp .. '\'' .. v .. '\'';
				end
			end
			query = query .. ' SET ' .. tmp;
		end
	elseif( self._main_method == 'insert' ) then
		-- Gen an INSERT INTO query
		query = 'INSERT INTO `' .. self._from .. '`';

		local tmp = '';
		if( #self._values ) then
			-- Split the fields and values
			local fields = {};
			local values = {};
			for i,v in pairs(self._values) do
				fields[i] = i;

				if( v == NULL_substitute ) then
					values[i] = 'NULL';
				elseif( type(v) == 'string' ) then
					values[i] = "'" .. v .. "'";
				else
					values[i] = v;
				end
			end

			local fieldPart = string.implode(fields, ',');
			local valuePart = string.implode(values, ',');
			tmp = ' (' .. fieldPart .. ') VALUES (' .. valuePart .. ')';
		end
		query = query .. tmp;
	elseif( self._main_method == 'select' ) then
		-- Gen a SELECT query
		if( #self._select == 0 ) then -- Assume SELECT *
			query = 'SELECT `*`';
		else
			local count = #self._select;
			local tmp = '';
			for i,v in pairs(self._select) do
				tmp = tmp .. '`' .. v .. '`';
				if( i < count ) then
					tmp = tmp .. ',';
				end
			end
			query = 'SELECT ' .. tmp;
		end

		query = query .. ' FROM `' .. self._from .. '`';
	end

	query = query .. self:formatQuery();

	return query;
end