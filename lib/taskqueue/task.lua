Task = class.new();

function Task:constructor(_func, _time)
	-- Ensure that we were given a function to execute.
	if( type(_func) ~= "function" ) then
		return
	end

	if( type(_time) == "number" ) then
		self.driver = 'relative';
	elseif( type(_time) == "table" and _time.isPast ) then
		self.driver = 'timestamp';
	end

	self.fullTime = _time;		-- Save time between runs
	self.func = _func;			-- And the actual function to run
	self.timeLeft = _time;		-- And set our timer to full
end

function Task:update(dt)
	dt = dt or time.deltaTime(); -- Use dt supplied, or rely on the global one.

	if( self.driver == "relative" ) then
		-- Remove a small piece of time from our timer
		self.timeLeft = self.timeLeft - dt;

		-- Now check if it is ready to run
		if( self.timeLeft < 0 ) then
			local ret = self.func();

			-- Return our functions return value to the task queue
			return false;
		end
	elseif( self.driver == "timestamp" ) then
		-- Check if it is ready to run
		if( self.fullTime:isPast() ) then
			local ret = self.func();

			-- Return our value to task queue
			return false;
		end
	end


	-- Return true to signal that we need to keep this task for now.
	return true;
end