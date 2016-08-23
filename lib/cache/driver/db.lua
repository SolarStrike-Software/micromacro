require('cache/driver/driver');
DBDriver	=	CacheDriver();

--[[
	DB driver accepts config options:
		file		-	The name (and optionally path) to read/store the cache file
]]

local defaultFilename	=	'cache.db';
local defaultTable		=	'cache';
function DBDriver:constructor(config)
	config			=	config or {};
	self.name		=	'SQL Database Cache Driver';
	self.filename	=	config.file or defaultFilename;
	self.table		=	config.table or defaultTable;
	local init		=	false;

	if( not filesystem.fileExists(self.filename) ) then
		init		=	true;
	end

	self.db		=	sqlite.open(self.filename);

	self:createIfNotExists();
	self:flushExpired();
end

function DBDriver:destructor()
	if( self.db ) then
		sqlite.close(self.db);
	end
end

function DBDriver:drop()
	local sql = [[DROP TABLE IF EXISTS "main"."%s";]]
	sqlite.execute(self.db, sprintf(sql, self.table));
end

function DBDriver:createIfNotExists()
	-- Drops and recreates the table
	local sql = [[
	CREATE TABLE IF NOT EXISTS "%s" (
	"id"  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	"key"  TEXT NOT NULL,
	"value"  TEXT,
	"expires_at"  INTEGER NOT NULL
	);
]]
	sqlite.execute(self.db, sprintf(sql, self.table, self.table));
end

function DBDriver:flushExpired()
	-- Removes expired entries
	local sql = [[
	DELETE FROM `%s` WHERE `expires_at` <= %d
]]
	sqlite.execute(self.db, sprintf(sql, self.table, os.time()));
end

function DBDriver:get(itemName, defaultValue)
	-- Returns an item from the cache if it is not expired, returns the (optional) default value if not-exists/not-found
	local sql = [[
	SELECT `value` from `%s` WHERE `key` = '%s' AND `expires_at` > %d OR `expires_at` IS NULL LIMIT 1
]]
	local results = sqlite.execute(self.db, sprintf(sql, self.table, itemName, os.time()));

	if( results and #results >= 1 ) then
		return results[1].value;
	end
	return defaultValue;
end

function DBDriver:has(itemName)
	-- Returns true if the item exists and is not expired
	return self:get(itemName) ~= nil;
end

function DBDriver:set(itemName, value, minutes)
	-- Sets an item with given value to expire in `minutes` from now (default 1 minute)
	-- Use a negitive value for minutes to never expire
	minutes = minutes or 1;
	local sql = [[
	INSERT OR REPLACE INTO `%s` (id, key, value, expires_at) values
	((SELECT `id` FROM `%s` WHERE `key` = "%s"), "%s", "%s", %s);
]]

	local minutesStr;
	if( minutes >= 0 ) then
		minutesStr = os.time() + minutes*60;
	else
		minutesStr = "NULL";
	end
	sqlite.execute(self.db, sprintf(sql, self.table, self.table, itemName, itemName, value, minutesStr));
end

function DBDriver:remember(itemName, minutes, callback)
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

function DBDriver:rememberForever(itemName, callback)
	-- Same as remember, only it doesn't expire
	return self:remember(itemName, -1, callback);
end

function DBDriver:renew(itemName, minutes)
	local sql = [[UPDATE `%s` SET `expires_at` = %s WHERE `key` = '%s']]

	local expire;
	if( type(minutes) == "nil" ) then
		expire	=	"NULL";
	elseif( type(minutes) == "number" or tonumber(minutes) ) then
		expire	=	tostring(os.time() + minutes * 60);
	else
		expire	=	tostring(minutes);
	end

	sqlite.execute(self.db, sprintf(sql, self.table, expire, itemName));
end

function DBDriver:forget(itemName)
	-- Removes an item from the cache
	local sql = [[
		DELETE FROM `%s` WHERE `key` = "%s"
]]
	sqlite.execute(self.db, sprintf(sql, self.table, itemName));
end

function DBDriver:flush()
	-- Removes all items from the cache
	self:drop();
	self:createIfNotExists();
end