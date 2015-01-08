Timestamp = class.new();

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


function Timestamp:now()
	return Timestamp(os.time());
end

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

function Timestamp:toHuman()
	return os.date("%Y-%m-%d  %H:%M:%S", self.timevalue);
end

function Timestamp:format(fmt)
	return os.date(fmt, self.timevalue);
end

function Timestamp:diffForHumans()
	local function pluralize(word, count)
		if( count == 1 ) then
			return word;
		else
			return word .. "s";
		end
	end

	local now = os.time();
	local suffix = "ago";
	local diffSeconds = now - self.timevalue;

	if( now < self.timevalue ) then
		diffSeconds = self.timevalue - now;
		suffix = "from now";
	end

	if( diffSeconds < 60 ) then
		return math.floor(diffSeconds) .. " " .. pluralize("second", diffSeconds) .. " " .. suffix;
	end

	local diffMinutes = diffSeconds / 60;
	if( diffMinutes < 60 ) then
		return math.floor(diffMinutes) .. " " .. pluralize("minute", diffMinutes) .. " " .. suffix;
	end

	local diffHours = diffMinutes / 60;
	if( diffHours < 60 ) then
		return math.floor(diffHours) .. " " .. pluralize("hour", diffHours) .. " " .. suffix;
	end

	local diffDays = diffHours / 24;
	return math.floor(diffDays) .. " " .. pluralize("day", diffDays) .. " " .. suffix;
end

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
	return Timestamp(self.timevalue + hours*60*60);
end

function Timestamp:subHours(hours)
	return Timestamp(self.timevalue - hours*60*60);
end

function Timestamp:addDays(days)
	return Timestamp(self.timevalue + days*24*60*60);
end

function Timestamp:subDays(days)
	return Timestamp(self.timevalue - days*24*60*60);
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

function Timestamp:isPast()
	return self.timevalue <= os.time();
end

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

function meta:__lt(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue < other.timevalue;
end

function meta:__gt(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue > other.timevalue;
end

function meta:__eq(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue == other.timevalue;
end

function meta:__sub(other)
	if( type(other) ~= "table" or not other.timevalue ) then
		error("Object does not appear to be a valid timestamp.", 2);
	end

	return self.timevalue - other.timevalue;
end