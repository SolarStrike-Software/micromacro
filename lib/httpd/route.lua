require('httpd/response');
require('httpd/request');
require('httpd/error');
require('httpd/helpers');
require('httpd/validator');

local _Route	=	class.new();

function _Route:constructor()
	self.routes					=	{};
end

function _Route:controller(index, className)
	self.routes[index]	=	className;
end

function _Route:loadRoutes()
	include('routes.lua', true);
end

function _Route:handle(httpd, header, content)
	if( not httpd and not header or not content ) then
		error("One or missing argument to _Route:handle()", 2);
	end

	-- Helper to load a controller
	function loadController(controllerName)
		local status,output	=	pcall(dofile, httpd.controllerDir .. controllerName .. ".lua");
		return status, output;
	end



	-- (Re-) load routes
	self:loadRoutes();

	-- Figure out what method we should use. If invalid, error it
	local method, path, query, fragment;
	if( header['GET'] ) then -- GET method
		method		=	'GET';
	elseif( header['POST'] ) then -- POST method
		method		=	'POST';
	else
		-- We can't handle this type of request
		return stdError(504);
	end


	-- Parse URI and make use of it
	path, query, fragment	=	string.match(header[method], "^/([0-9A-Za-z%-%_%/%.]*)%??(.*)#?([0-9A-Za-z%-%_]*)");
	if( not path ) then -- If we can't make sense of it, throw a 400
		return stdError(400);
	end

	-- If this is a static path, return the static content
	if( method == 'GET' ) then
		if( path ~= "" and filesystem.fileExists(httpd.staticDir .. path) ) then
			local contentType,readAs	=	getMimeType(path);

			local file	=	io.open(httpd.staticDir .. path, readAs);
			local str	=	file:read("*all");
			file:close();

			return Response(200, str, {['Content-Type'] = contentType});
		end
	end

	local queryTab	=	{};
	local fileTab	=	{};
	local postTab	=	{};

	-- If there is a file uploaded, we need to extract its contents
	if( method == 'POST' ) then
		local boundary = string.match(header['Content-Type'], "multipart/form%-data; boundary=(.*)$");
		if( boundary ) then
			local boundaryLen	=	string.len(boundary);
			local beginBlock	=	string.find(content, "--" .. boundary, 1, true) + boundaryLen + 2;;
			--local endBlock		=	string.find("content", "--" .. boundary .. "--", beginBlock) + boundaryLen + 4;

			while(true) do
				local disposition	=	string.match(content, "Content%-Disposition: form%-data;(.-)\r\n", beginBlock);

				if( not disposition ) then
					break;
				end

				local name			=	string.match(disposition, "%sname%s?=%s?\"(.-)\";?");
				local filename		=	string.match(disposition, "%sfilename%s?=%s?\"(.-)\";?");

				local contentBeginsAt	=	(string.find(content, "\r\n\r\n", beginBlock, true) or 0) + 4;
				local contentEndsAt		=	string.find(content, "\r\n--" .. boundary, beginBlock, true) string.len(content);
				local fileContents		=	string.sub(content, contentBeginsAt, contentEndsAt - 1);

				fileTab[name]	=	{
					name		=	name,
					filename	=	filename,
					size		=	string.len(fileContents),
					content		=	fileContents,
				};

				beginBlock	=	contentEndsAt;
				if( beginBlock >= string.len(content) or string.sub(content, beginBlock, 2) == "--" ) then
					break;
				end
			end
		end
	end

	-- Parse query & post strings
	queryTab	=	parseQueryString(query);
	postTab		=	parseQueryString(content);

	
	-- Of the pathname, parse controller and method out
	local class, func, rest	=	string.match(path, "^([0-9A-Za-z%-%_]*)%/?([0-9A-Za-z%-%_]*)/?(.*)");
	local controllerName	=	self.routes[class];

	local extraUriParts		=	string.explode(rest, "/");

	-- Special case, if path == "", override with default registered controller
	if( path == "" ) then
		if( self.routes['/'] ) then
			controllerName	=	self.routes['/'];
		end
	end

	if( not class or not controllerName ) then -- If no route for it, 404
		printf("Could not parse controller name or no route exists for \'%s\'\n", path);
		return stdError(404);
	end
	
	-- Load the controller, run appropriate function (if found)
	local status,result = loadController(controllerName);
	if( not status ) then -- If there's some error loading the file, then show it
		return stdError(500, result);
	end

	local controller = _G[controllerName];
	if( not controller ) then -- If we fail to load the controller file or the controller object isn't found, 404
		printf("Controller file was loaded but global class \'%s\' is not found\n", controllerName);
		return stdError(404);
	end

	if( func ~= "" ) then
		-- Fix func (camelcase)
		local first,rest = string.sub(func, 1, 1), string.sub(func, 2);
		func = string.upper(first) .. camelize(rest);
	else
		-- Assume <method>Index
		func	=	'Index';
	end

	-- Prepend method
	func	=	string.lower(method) .. func;

	if( not controller[func] ) then -- Once more, we check the existance of the function before attempting to call it
		printf("Controller exists but could not locate function %s() inside of it.\n", func);
		return stdError(404);
	end

	-- Call controller's constructor (if available)
	if( type(controller['constructor']) == 'function' ) then
		local status,result	=	pcall(controller['constructor'], controller);
		if( not status ) then
			return stdError(500, result);
		end
	end

	-- Set View's directory
	View.viewDir	=	httpd.viewDir;

	-- Build request
	request		=	Request();
	request.method		=	string.lower(method);
	request.fragment	=	fragment;
	request.query		=	queryTab;
	request.post		=	postTab;
	request.files		=	fileTab;
	request.cookies		=	header['Cookies'];
	request.uriSegments	=	extraUriParts;

	local errors		=	request:getCookie('flashvar_error');
	errors				=	string.explode(errors or '', "\n");
	if( #errors > 0 ) then
		print("Request has errors");
		request.errors	=	errors;
	end

	local inputStr		=	request:getCookie('flashvar_oldinput');

	if( inputStr ) then
		local inputParts	=	string.explode(inputStr, '&');
		for i,v in pairs(inputParts) do
			print("Part:", v);
			local name,value	=	string.match(v, "(.*)=(.*)");
			name	=	urldecode(name);
			value	=	urldecode(value);

			request.oldInputs[name]	=	value;
		end
	end

	-- If a validator is set, try to validate the request
	if( controller.validators and type(controller.validators) == 'table' and type(controller.validators[func]) == 'table' ) then
		local validator	=	Validator(controller.validators[func]);
		local status,result	=	pcall(validator.passes, validator, request:all());

		if( not status ) then
			-- Error in the validator
			return Response(500, result);
		end
		if( not result ) then
			-- Validation failed
			return ValidationError(validator:getErrors(), header['Referer']):withInput(request);
		end
	end

	local status,result	=	pcall(controller[func], controller, request, table.unpack(extraUriParts));

	if( status ) then
		-- All OK
		if( type(result) == 'table' ) then
			return result;
		elseif( type(result) == 'string' ) then
			return Response(200, result);
		else
			return stdError(500, 'Invalid response from controller');
		end
	else
		-- Something went wrong in the function
		return stdError(500, result);
	end
end

Route	=	_Route();