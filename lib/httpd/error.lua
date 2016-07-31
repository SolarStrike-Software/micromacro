require('httpd/response');
-- Helper for returning standard error codes
local errors = {
	[400]	=	'<h1>Error 400 - Bad Request</h1>',
	[403]	=	'<h1>Error 403 - Forbidden</h1>',
	[404]	=	'<h1>Error 404 - Not found</h1>',
	[500]	=	'<h1>Error 500 - Internal Server Error</h1>',
	[504]	=	'<h1>Error 504 - Method Not Allowed</h1>',
};

function stdError(code, extra)
	extra	=	extra or '';
	if( errors[code] ) then
		local fullmsg = errors[code] .. extra;
		return Response(code, fullmsg);
	else
		return Response(code, '<h1>Error ' .. code .. '</h1>' .. extra);
	end
end