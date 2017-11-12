-- Requiring this file enables strict mode
-- Whenever you attempt to access an invalid (nil) global var, an error shall be raised.

setmetatable(_G,
	{
		__index = function(_, key)
			error("Undefined variable `" .. key .. "` used", 2);
		end
	}
);