require('cache/driver/driver');
DBDriver	=	CacheDriver();

local filename	=	'cache.db';
function DBDriver:constructor()
	self.name	=	'SQL Database Cache Driver';

	local init	=	false;
	if( not filesystem.fileExists(filename) ) then
		init	=	true;
	end

	self.db		=	sqlite.open(filename);
	if( init ) then
		self:createNew();
	end

	self:flushExpired();
end

function DBDriver:destructor()
	if( self.db ) then
		sqlite.close(self.db);
	end
end

function DBDriver:createNew()
	-- Drops and recreates the table
	local sql = [[
	DROP TABLE IF EXISTS "main"."cache";
	CREATE TABLE "cache" (
	"id"  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
	"key"  TEXT NOT NULL,
	"value"  TEXT,
	"expires_at"  INTEGER NOT NULL
	);
]]
	sqlite.execute(self.db, sql);
end

function DBDriver:flushExpired()
	-- Removes expired entries
	local sql = [[
	DELETE FROM `cache` WHERE `expires_at` <= %d
]]
	sqlite.execute(self.db, sprintf(sql, os.time()));
end

function DBDriver:get(itemName, defaultValue)
	-- Returns an item from the cache if it is not expired, returns the (optional) default value if not-exists/not-found
	local sql = [[
	SELECT `value` from `cache` WHERE `key` = '%s' AND `expires_at` > %d OR `expires_at` IS NULL LIMIT 1
]]
	local results = sqlite.execute(self.db, sprintf(sql, itemName, os.time()));

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
	INSERT OR REPLACE INTO `cache` (id, key, value, expires_at) values
	((SELECT `id` FROM `cache` WHERE `key` = "%s"), "%s", "%s", %s);
]]

	local minutesStr;
	if( minutes >= 0 ) then
		minutesStr = os.time() + minutes*60;
	else
		minutesStr = "NULL";
	end
	sqlite.execute(self.db, sprintf(sql, itemName, itemName, value, minutesStr));
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

function DBDriver:forget(itemName)
	-- Removes an item from the cache
	local sql = [[
		DELETE FROM `cache` WHERE `key` = "%s"
]]
	sqlite.execute(self.db, sprintf(sql, itemName));
end

function DBDriver:flush()
	-- Removes all items from the cache
	self:createNew();
end