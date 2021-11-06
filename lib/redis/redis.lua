Redis = class.new()

local DEFAULT_TIMEOUT = 10


function Redis:constructor()
    self.socket = nil
end

function Redis:open(host, port, username, password)
    self:close()

    self.host     = host or '127.0.0.1'
    self.port	  = port or 6379
    self.username = username or nil
    self.password = password or nil

    self.socket = network.socket('tcp')
    local result, err = self.socket:connect(self.host, self.port)

    if( not result ) then
        error(err, 2)
    end
end

function Redis:close()
    if( self.socket ~= nil ) then
        self.socket:close()
        self.socket = nil
    end
end

function Redis:getResponse(timeout)
    timeout = timeout or DEFAULT_TIMEOUT

    local startTime = time.getNow()
    local recvd

    repeat
        recvd = self.socket:recv()
        if( recvd ~= nil ) then
            return self:parseResponse(recvd)
        end
        system.rest(10)
    until( time.diff(time.getNow(), startTime) > timeout )

    return nil
end

function Redis:parseResponse(response)
    --print("RAW RESPONSE:", response)
    -- https://redis.io/topics/protocol
    if( type(response) ~= 'string' ) then
        if( response == nil ) then
            return nil
        end

        local typename = type(response)
        error("Received invalid response type `" .. typename .. "` from Redis server", 2)
    end

    local rType, data = string.match(response, '^([%+%-:])(.*)$')
    if( rType == '+' ) then -- simple string (ie. +OK\r\n)
        return data, string.len(data)
    end

    if( rType == '-' ) then -- error (ie. -ERR something went wrong)
        error(data, 3)
    end

    if( rType == ':' ) then -- integer (ie. ":0\r\n")
        return tonumber(data), string.len(data)
    end


    local rType, len, data = string.match(response, '^([$:*])([%-%d]+)\r\n(.*)') --'^([$+-:*])(\d+)(.*)$')
    if( rType == nil ) then
        return nil
    end

    if( rType == '$' ) then -- bulk string (ie. $5\r\nhello\r\n)
        if( tonumber(len) < 0 ) then -- Null element, len should be -1
            return nil
        end
        local value = string.sub(data, 0, len);
        return value, string.len(value) + string.len(len) + 4
    end

    if( rType == '*' ) then -- array
        local array = {}
        local c = 1
        local arraySize = len
        for i = 1, arraySize do
            local value, nextLength = self:parseResponse(string.sub(data, c))
            c = c + nextLength + 1
            table.insert(array, value)
        end
        local bytesConsumed =  (c - 1) + 3 + string.len(len) -- (c-1) + '*' + '\r\n' + strlen(len)
        return array, bytesConsumed
    end

    return response
end

function Redis:buildCommand(...)
    local args = {...}
    local count = 0
    local result = ''

    for i,v in ipairs(args) do
        if( v ~= nil ) then
            result = result .. "$" .. string.len(v) .. "\r\n" .. tostring(v) .. "\r\n"
            count = i
        end
    end

    return "*" .. count .. "\r\n" .. result
end

function Redis:send(msg)
    self.socket:send(msg .. "\r\n")
    local response = self:getResponse()
    return response
end

function Redis:get(key)
    return self:send(self:buildCommand('GET', key))
end

function Redis:set(key, value, expireSeconds)
    if( expireSeconds == nil ) then
        return self:send(self:buildCommand('SET', key, value))
    else
        return self:send(self:buildCommand('SET', key, value, 'PX', math.floor(expireSeconds * 1000.0)))
    end
end

function Redis:delete(key)
    return self:send(self:buildCommand('DEL', key))
end

function Redis:push(key, ...)
    return self:send(self:buildCommand('RPUSH', key, ...))
end

function Redis:pushFront(key, ...)
    local args = {}
    for i,v in pairs({...}) do
        table.insert(args, 1, v)
    end

    return self:send(self:buildCommand('LPUSH', key, table.unpack(args)))
end

function Redis:pop(key, count)
    count = count or 1
    return self:send(self:buildCommand('RPOP', key, count))
end

function Redis:popFront(key, count)
    count = count or 1
    return self:send(self:buildCommand('LPOP', key, count))
end

function Redis:slice(key, from, to)
    from = from or 0
    to = to or -1

    return self:send(self:buildCommand('LRANGE', key, from, to))
end

function Redis:increment(key, amount)
    amount = amount or 1

    return self:send(self:buildCommand('INCRBY', key, amount))
end

function Redis:decrement(key, amount)
    amount = amount or 1

    return self:send(self:buildCommand('DECRBY', key, amount))
end


function Redis:keys(pattern)
    -- Intentionally disabled for "production" type access, so cannot
    -- be requested using normal Redis RESP command structure.
    -- Instead, the plain-text version works, though is still
    -- not recommended for anything except debugging.
    -- Use scan() instead.
    return self:send('KEYS ' .. pattern)
end

function Redis:iterateScan(func, pattern, cursor)
    cursor = cursor or 0
    local cmd = ''

    if( pattern ~= nil ) then
        cmd = self:buildCommand(func, cursor, 'MATCH', pattern)
    else
        cmd = self:buildCommand(func, cursor)
    end

    print("CMD:", cmd)
    local results = self:send(cmd)
    local cursor = 0

    if( type(results) == 'table' ) then
        cursor = results[1] or 0
        results = results[2] or {}
    end

    return tonumber(cursor), results
end

function Redis:doScan(func, pattern)
    local results = {}

    local cursor = 0
    local newItems
    repeat
        cursor, newItems = self:iterateScan(func, pattern, cursor)

        if( #newItems > 0 ) then
            for i,v in pairs(newItems) do
                table.insert(results, v)
            end
        end
    until( cursor == 0 )

    return results
end

function Redis:scan(pattern)
    return self:doScan('SCAN', pattern)
end
