Assert = class.new()

local ASSERTION_FAILED_MESSAGE<const> = 'assertion failed!'
Assert.failMessage = ASSERTION_FAILED_MESSAGE

function Assert:expectError(closure, msg)
	local success, err = pcall(closure)
	-- We expect that it did NOT succeed. So if it did, we error.
	if success then error(msg or "assertion failed!", 2) end
end