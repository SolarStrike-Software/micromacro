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
        return data
    end

    if( rType == '-' ) then -- error (ie. -ERR something went wrong)
        error(data, 3)
    end

    if( rType == ':' ) then -- integer (ie. ":0\r\n")
        return tonumber(data)
    end


    local rType, len, data = string.match(response, '^([$:*])([%-%d]+)\r\n(.*)$') --'^([$+-:*])(\d+)(.*)$')
    if( rType == nil ) then
        return nil
    end

    if( rType == '$' ) then -- bulk string (ie. $5\r\nhello\r\n)
        if( tonumber(len) < 0 ) then -- Null element, len should be -1
            return nil
        end
        return string.sub(data, 0, len)
    end

    if( rType == '*' ) then -- array
        local array = {}
        local c = 1
        for i = 1, len do
            local nextSet = string.match(data, '^[$%+%-:*]%d+\r\n[^\r\n]+', c)
            local nextSet = nextSet .. '\r\n' -- Re-add the trailing \r\n that we chopped above
            c = c + string.len(nextSet)
            table.insert(array, self:parseResponse(nextSet))
        end

        return array
    end

    return response
end

function Redis:send(msg)
    self.socket:send(msg .. "\r\n")
    local response = self:getResponse()
    return response
end
