require('httpd/route');
require('httpd/error');
require('httpd/view');
require('httpd/json');

Httpd	=	class.new();

function Httpd:constructor(config)
	self.port	=	8080;
	self.ip		=	'127.0.0.1';

	self:init(config);
	self.clients = {};
end

function Httpd:init(config)
	if( not config or type(config) ~= 'table' ) then
		config = {};
	end

	local baseDir		=	filesystem.getCWD();

	self.port			=	config.port	or	self.port;
	self.ip				=	config.ip	or	self.ip;
	self.server			=	network.socket('tcp');
	self.controllerDir	=	config.controllerDir	or	(baseDir .. '/Controllers/');
	self.viewDir		=	config.viewDir			or	(baseDir .. '/Views/');
	self.staticDir		=	config.staticDir		or	(baseDir .. '/Static/');
	self.clients		=	{};

	print("self.controllerDir:", self.controllerDir);

	local success	=	self.server:listen(self.ip, self.port);
	if( success ) then
		printf("Listening on %s:%d\n", self.ip, self.port);
	else
		printf("Failed to listen on port %d\n", self.port);
	end
end

function Httpd:handleEvent(event, ...)
	local listeners = {
		socketconnected	=	function(self, socket, listenSockId)
			if( listenSockId ~= self.server ) then
				printf("Client %d from %s (to %s) connected.\n", socket:id(), socket:remoteIp(), socket:ip());
				self.clients[socket:id()]	=	{socket = socket, status = 'initial', data = '', dataLen = 0, flashvars = {}};
			end
		end,

		socketdisconnected	=	function(self, sockId)
			--self:removeClient(sockId);
			self.clients[sockId]	=	nil;
			printf("Client %d disconnected.\n", sockId);
		end,

		socketerror			=	function(self, sockId, errId)
			print("Socket error:", sockId, errId);
			--self:removeClient(sockId);
			self.clients[sockId]	=	nil;
		end,

		socketreceived		=	function(self, sockId, data)
			-- This is a message for the httpd to handle
			local client	=	self.clients[sockId];
			if( client ) then -- Always ensure that the socket is available (remote could have closed it between events)
				self:handleMessage(client, data);
			else
				self.clients[sockId]	=	nil;
			end
		end,
	};

	if( listeners[event] ) then
		listeners[event](self, ...);
		return true;
	else
		return false;
	end
end

function Httpd:handleMessage(client, data)
	-- If still receiving data, keep holding it until we've got the full thing
	local method, resource;
	if( client.status == 'receiving' ) then
		client.data = client.data .. data;

		printf("\tReceived another %d bytes, total so far: %d\n", string.len(data), string.len(client.data));
		if( string.len(client.data) >= client.dataLen ) then
			client.status = 'done';
		else
			return;
		end
	end

	if( client.status == 'initial' ) then
		-- Split header & content
		local headerLength	=	string.find(data, "\r\n\r\n");

		if( not headerLength ) then -- Malformed input
			response	=	stdError(400);
			success = client.socket:send(self:renderResponse(response));
			return;
		end

		local headerStr		=	string.sub(data, 0, headerLength);
		local contentStr	=	string.sub(data, headerLength + 4); -- +4 for the \r\n\r\n

		-- Read & use header
		local header = {};
		local eol	=	string.find(headerStr, "\r\n");
		local firstline	=	string.sub(headerStr, 0, eol);
		method, resource = string.match(firstline, "^([A-Z]*) (.*) HTTP/1%.?");

		if( not method and not resource ) then -- Malformed input
			response	=	stdError(400);
			success = client.socket:send(self:renderResponse(response));
			return;
		end

		header[method]	=	resource;

		-- Get any other header vars
		local offset = eol + 2; -- +2 for \r\n
		local line;
		local var,value;
		while(offset < headerLength) do
			eol			=	string.find(headerStr, "\r\n", offset) or headerLength; -- Not found must mean last line
			line		=	string.sub(headerStr, offset, eol - 1);
			var, value	=	string.match(line, "([a-zA-Z%-]*): (.*)");
			if( var and value ) then
				header[var]	=	value;
			end

			offset = eol + 2;
		end

		-- Parse cookies
		header['Cookies']	=	{};
		if( header['Cookie'] ) then
			local cookies	=	string.explode(header['Cookie'], '; ');
			for i,v in pairs(cookies) do
				local name,value	=	string.match(v, "([%a%d%-%_]+)=(.*)%s?");
				name	=	urldecode(name);
				value	=	urldecode(value);
				header['Cookies'][name]	=	value;
				if( string.match(name, "^flashvar_") ) then
					table.insert(client.flashvars, name);
				end
			end
		end

		-- Hold it in the client
		client.header	=	header;

		-- If POST method, ensure they have a valid Content-Length
		if( string.upper(method) == 'POST' ) then
			local cl	=	tonumber(header['Content-Length']);
			if( not cl ) then
				response	=	stdError(411);
				success = client.socket:send(self:renderResponse(response));
				return;
			end

			printf("recvd: %d, expecting: %d\n", string.len(contentStr), cl);
			client.data		=	contentStr;
			client.dataLen	=	cl;
			if( string.len(contentStr) < cl ) then
				-- If still receiving info, wait for all of it.
				client.status	=	'receiving';
				return;
			end
		end
	end

	response = Route:handle(self, client.header, client.data);
	printf("\t%s %s\t%d\n", method, resource, response.code);

	-- Mark flashfars for removal if not re-set
	for i,v in pairs(client.flashvars) do
		if( not response.setCookies[v] ) then
			response.setCookies[v]	=	{value="", expire=0};
		end
	end

	local success;
	if( response ) then
		success = client.socket:send(self:renderResponse(response));
	else
		response	=	stdError(500);
		success = client.socket:send(self:renderResponse(response));
	end

	if( not success ) then
		printf("For some reason sending a response has failed; error in send()...\n");
	end

	-- Reset status back to initial now that we've finished
	client.status = 'initial';
end

function Httpd:renderResponse(response)
	local code			=	response.code or 200;
	local data			=	response.content or '';
	local header		=	response.header or {};
	local setCookies	=	response.setCookies or {};

	setCookiesStr	=	'';
	local defaultExpireTime	=	30*24*60*60;
	for i,v in pairs(setCookies) do
		local	name	=	urlencode(i);
		local	value	=	urlencode(v.value or "");
		local	expire	=	Timestamp:now():addSeconds(v.expire or defaultExpireTime):toUtc();

		setCookiesStr	=	setCookiesStr .. sprintf("Set-Cookie: %s=%s;", name, value)
			..	sprintf("Domain=%s;", self.ip)
			..	sprintf("Path=%s;", "/")
			..	sprintf("Expires=%s;\r\n", expire);
	end


	local length	=	string.len(data);
	header['Content-Type']	=	header['Content-Type'] or 'text/html';
	
	local headerExtra = '';
	for i,v in pairs(header) do
		headerExtra	=	headerExtra .. i .. ": " .. v .. "\r\n";
	end

	-- NOTE: we need to have our Set-Cookie header bits *before* Content-Type (regular header stuff)
	local output	=	sprintf("HTTP/1.0 %d Document follows\r\nServer: %s \r\nContent-Length: %d\r\n%s%s\r\n%s", code, self.ip, length, setCookiesStr, headerExtra, data);
	return output;
end