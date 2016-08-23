require('cache/driver/db');
require('cache/driver/memory');

Cache = class.new();

local driverMap = {
	db		=	DBDriver,
	memory	=	MemoryDriver,
};

local defaultDriverId	=	'db';
function Cache:constructor(requestedDriver, config)
	local driverId;
	if( driverMap[requestedDriver] ) then
		driverId	=	requestedDriver;
	else
		driverId	=	defaultDriverId;
	end

	self.driver		=	driverMap[driverId](config);
end

function Cache:driverName()
	return self.driver:getName();
end

function Cache:get(itemName, defaultValue)
	return self.driver:get(itemName, defaultValue);
end

function Cache:has(itemName)
	return self.driver:has(itemName);
end

function Cache:set(itemName, value, minutes)
	return self.driver:set(itemName, value, minutes);
end

function Cache:remember(itemName, minutes, callback)
	return self.driver:remember(itemName, minutes, callback);
end

function Cache:rememberForever(itemName, callback)
	return self.driver:rememberForever(itemName, callback);
end

function Cache:renew(itemName, minutes)
	return self.driver:renew(itemName, minutes);
end

function Cache:forget(itemName)
	return self.driver:forget(itemName);
end

function Cache:flush()
	return self.driver:flush();
end