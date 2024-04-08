Assert = class.new()

local ASSERTION_FAILED_MESSAGE<const> = 'assertion failed!'

function Assert.getFailMessage()
    return ASSERTION_FAILED_MESSAGE
end

function Assert:constructor(unitTester)
    self.unitTester = unitTester
end

function Assert:expectError(closure, msg)
    local success, err = pcall(closure)
	self.unitTester:incrementAssertCount()
	
    -- We expect that it did NOT succeed. So if it did, we error.
    if success then
        error(msg or ASSERTION_FAILED_MESSAGE, 2)
    end
end
