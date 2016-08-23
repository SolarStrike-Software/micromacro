require('cache/driver/driver');
MemoryDriver	=	CacheDriver();

function MemoryDriver:constructor()
	self.name	=	'Memory-only Cache Driver';

	self.items	=	{};
end

function MemoryDriver:get(itemName, defaultValue)
	-- Returns an item from the cache if it is not expired, returns the (optional) default value if not-exists/not-found
	if( self.items[itemName] and (self.items[itemName].expires_at == nil or self.items[itemName].expires_at > os.time())) then
		return self.items[itemName].value;
	end
	return defaultValue;
end

function MemoryDriver:has(itemName)
	-- Returns true if the item exists and is not expired
	return self:get(itemName) ~= nil;
end

function MemoryDriver:set(itemName, value, minutes)
	-- Sets an item with given value to expire in `minutes` from now (default 1 minute)
	-- Use a negitive value for minutes to never expire
	minutes = minutes or 1;

	local expires = nil;
	if( minutes > 0 ) then
		expires = os.time() + minutes * 60;
	end
	self.items[itemName] = {name = itemName, value = value, expires_at = expires};
end

function MemoryDriver:remember(itemName, minutes, callback)
	-- Returns a remembered item (if set and not expired)
	-- Otherwise, the result of callback will be used to set the item in the cache, and be returned.
	local value	= self:get(itemName);

	if( value ) then
		-- We have a good value still.
		return value;
	else
		-- Value does not exist or is expired; we must set it
		local cbResult	=	callback();
		self:set(itemName, cbResult, minutes);
		return cbResult;
	end
end

function MemoryDriver:rememberForever(itemName, callback)
	-- Same as remember, only it doesn't expire
	return self:remember(itemName, -1, callback);
end

function MemoryDriver:renew(itemName, minutes)
	local expire;
	if( type(minutes) == 'nil' ) then
		expire	=	nil;
	elseif( type(minutes) == 'number' or tonumber(minutes) ) then
		expire	=	minutes * 60;
	else
		expire	=	minutes;
	end

	if( self.items[itemName] ) then
		self.items[itemName].expires_at	=	expire;
	end
end

function MemoryDriver:forget(itemName)
	-- Removes an item from the cache
	self.items[itemName] = nil;
end

function MemoryDriver:flush()
	-- Removes all items from the cache
	self.items = {};
end