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
        error(msg or self:getFailMessage(), 2)
    end
end

function Assert:same(left, right, msg)
    self.unitTester:incrementAssertCount()

    if type(left) ~= type(right) or left ~= right then
        if type(left) == 'string' then
            left = '"' .. left .. '"'
        end
        if type(right) == 'string' then
            right = '"' .. right .. '"'
        end
        error(msg or sprintf('(%s) %s != (%s) %s. %s', type(left), left, type(right), right, self:getFailMessage()), 2)
    end
end

function Assert:different(left, right, msg)
    self.unitTester:incrementAssertCount()

    if type(left) ~= type(right) then
        return
    end

    if left ~= right then
        return
    end

    if type(left) == 'string' then
        left = '"' .. left .. '"'
    end
    if type(right) == 'string' then
        right = '"' .. right .. '"'
    end

    error(msg or sprintf('(%s) %s == (%s) %s. %s', type(left), left, type(right), right, self:getFailMessage()), 2)
end

function Assert:contains(haystack, needle, msg)
    self.unitTester:incrementAssertCount()

    if (type(haystack) ~= 'table') then
        error('Argument #1 is not a table', 2)
    end

    if (not table.find(haystack, needle)) then
        error(msg or 'value not found in table. ' .. self:getFailMessage(), 2)
    end
end

function Assert:notContains(haystack, needle, msg)
    self.unitTester:incrementAssertCount()

    if (type(haystack) ~= 'table') then
        error('Argument #1 is not a table', 2)
    end

    if (table.find(haystack, needle)) then
        error(msg or 'value is contained in table but not expected. ' .. self:getFailMessage(), 2)
    end
end
