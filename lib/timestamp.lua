Timestamp = class.new();

-- Create a timestamp from given date (by Y/M/D/...), UNIX timestamp, or just an "empty" timestamp
function Timestamp:constructor(y, mo, d, h, m, s)
	if( y and mo and d ) then
		-- Looks like we have day/month/year, so create a time from it
		local timeTab = {
			year = y,
			month = mo,
			day = d,
			hour = h,
			min = m,
			sec = s,
		};
		self.timevalue = os.time(timeTab);
	elseif( y ) then
		-- Maybe just the UNIX timestamp
		self.timevalue = y;
	else
		self.timevalue =  0;
	end
end

-- Returns 'now' as a Timestamp object
function Timestamp:now()
	return Timestamp(os.time());
end

-- Returns 'today' (at 00:00) as a Timestamp object
function Timestamp:today()
	local timeTab = {
		day = os.date("%d"),
		month = os.date("%m"),
		year = os.date("%Y"),
		hour = 0,
		min = 0,
		sec = 0,
	};

	return Timestamp(os.time(timeTab));
end


function Timestamp:yesterday()
	return Timestamp:today():subDays(1);
end


function Timestamp:tomorrow()
	return Timestamp:today():addDays(1);
end

-- Returns the local offset from UTC in hours
function Timestamp:timezoneOffset()
	local now	=	os.time();
	local utc	=	os.time(os.date("!*t", now));
	local seconds	=	os.difftime(now, utc);
	return seconds / 3600;
end

-- Returns a string details the time, in a human-readable format
-- Adjusted based on the timezone offset (if requested), or defaults to local time
-- tz_offset = 0 results in returning UTC time
function Timestamp:toHuman(tz_offset)
	if( tz_offset ) then
		-- If given a timezone offset, return output for that timezone.
		return os.date("!%Y-%m-%d  %H:%M:%S", self.timevalue + (tz_offset*3600));
	end

	-- Otherwise, return local time
	return os.date("%Y-%m-%d  %H:%M:%S", self.timevalue);
end

function Timestamp:toUtc()
	return os.date("%a, %d %b %Y %H:%M:%S UTC", self.timevalue);
end


-- Return a string of the time in any format requested
function Timestamp:format(fmt)
	return os.date(fmt, self.timevalue);
end


-- These below functions return a new timestamp object with the requested
-- time difference added/subtracted to it. It is pretty self explanatory.
function Timestamp:addSeconds(sec)
	return Timestamp(self.timevalue + sec);
end

function Timestamp:subSeconds(sec)
	return Timestamp(self.timevalue - sec);
end

function Timestamp:addMinutes(min)
	return Timestamp(self.timevalue + min*60);
end

function Timestamp:subMinutes(min)
	return Timestamp(self.timevalue - min*60);
end

function Timestamp:addHours(hours)
	return Timestamp(self.timevalue + hours*3600);
end

function Timestamp:subHours(hours)
	return Timestamp(self.timevalue - hours*3600);
end

function Timestamp:addDays(days)
	return Timestamp(self.timevalue + days*86400);
end

function Timestamp:subDays(days)
	return Timestamp(self.timevalue - days*86400);
end

function Timestamp:addMonths(months)
	local timeTab = {
		year = os.date("%Y"),
		month = os.date("%m") + months,
		day = os.date("%d"),
		hour = os.date("%H"),
		min = os.date("%M"),
		sec = os.date("%S"),
	};
	return Timestamp(os.time(timeTab));
end

function Timestamp:subMonths(months)
	local timeTab = {
		year = os.date("%Y"),
		month = os.date("%m") - months,
		day = os.date("%d"),
		hour = os.date("%H"),
		min = os.date("%M"),
		sec = os.date("%S"),
	};
	return Timestamp(os.time(timeTab));
end

function Timestamp:addYears(years)
	return Timestamp(self.timevalue + years*365*24*60*60);
end

function Timestamp:subYears(years)
	return Timestamp(self.timevalue - years*365*24*60*60);
end


-- Returns true if the timestamp is in the future, otherwise false (past or present)
function Timestamp:isFuture()
	return self.timevalue > os.time();
end


-- Returns true if the timestamp is in the past, otherwise false (future or present)
function Timestamp:isPast()
	return self.timevalue < os.time();
end


-- Returns true if the timestamp is right now, otherwise false (past or future)
function Timestamp:isNow()
	return self.timevalue ~= os.time();
end


-- Returns the difference between two timestamps in whichever requested unit.
function Timestamp:diffInSeconds(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue - other.timevalue;
end

function Timestamp:diffInMinutes(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return math.floor((self.timevalue - other.timevalue)/60);
end

function Timestamp:diffInHours(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return math.floor((self.timevalue - other.timevalue)/(60*60));
end

function Timestamp:diffInDays(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return math.floor((self.timevalue - other.timevalue)/(60*60*24));
end


local meta = getmetatable(Timestamp);
function meta:__tostring()
	return self:toHuman();
end

-- If this timestamp is prior to the 'other'
function meta:__lt(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue < other.timevalue;
end


-- If this timestamp is after the 'other'
function meta:__gt(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue > other.timevalue;
end


-- If the timestamps are the same
function meta:__eq(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue == other.timevalue;
end

-- Difference, in seconds, between two timestamps
function meta:__sub(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue - other.timevalue;
end