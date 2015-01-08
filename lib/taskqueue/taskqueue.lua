require("taskqueue/task");
local __TaskQueue = class.new();

function __TaskQueue:constructor()
	-- This is our table for holding upcoming tasks
	self.tasks = {};
	self.lastUpdate = time.getNow();
end

function __TaskQueue:push(task, trigger)
	-- Make sure we were given a task object
	if( type(task) ~= "function" ) then
		error("Not given a function to execute for argument #1.", 2);
	end

	-- Need to also know how long until we execute
	if( not(type(trigger) == "number" or (type(trigger) and trigger.isPast)) ) then
		error("Argument #2 to TaskQueue:push() needs to be a number or a timestamp.", 2);
	end

	table.insert(self.tasks, Task(task, trigger));
end

function __TaskQueue:update()
	local now = time.getNow();
	local dt = time.diff(now, self.lastUpdate);

	for i,v in pairs(self.tasks) do
		local ret = v:update(dt);

		-- Remove this task
		if( not ret ) then
			table.remove(self.tasks, i);
		end
	end

	self.lastUpdate = now;
end

TaskQueue = __TaskQueue();