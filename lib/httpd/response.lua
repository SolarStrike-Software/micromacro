Response			=	class.new();

function Response:constructor(code, content, header, setCookies)
	self.code		=	code or 200;		-- Assume HTTP code 200 (OK) if not given
	self.content	=	content or "";		-- What to display on the page
	self.header		=	header or {};		-- Any header overrides our response should have
	self.setCookies	=	setCookies or {};	-- Any cookies we want to set
end

function Response:withInput(request)
	local inputs	=	request:all();
	local oldInputStr	=	'';
	for i,v in pairs(inputs) do
		if( string.len(oldInputStr) > 0 ) then
			oldInputStr = oldInputStr .. '&';
		end
		oldInputStr	=	oldInputStr .. sprintf("%s=%s", urlencode(i), urlencode(v));
	end
	self.setCookies['flashvar_oldinput']	=	{value	=	oldInputStr};
	return self;
end

function Response:with(varname, varvalue)
	local fullname				=	sprintf('flashvar_%s', varname);
	self.setCookies[fullname]	=	{value = varvalue};
	return self;
end



JsonResponse		=	class.new(Response);
function JsonResponse:constructor(item)
	Response.constructor(self);
	self.code		=	200;
	self.content	=	json_encode(item);
end

Redirect			=	class.new(Response);
function Redirect:constructor(to)
	Response.constructor(self);
	self.code		=	302;
	self.header		=	{['Location'] = '/' .. to};
end

ValidationError		=	class.new(Response);
function ValidationError:constructor(errors, referer)
	Response.constructor(self);
	referer	=	referer or '/';
	errStr	=	'';
	for i,v in pairs(errors) do
		errStr = errStr .. v .. "\n";
	end

	self.code		=	302;
	self.header		=	{['Location']	=	referer};
	self.setCookies	=	{['flashvar_error'] = {value = errStr}};
end