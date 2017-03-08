require('httpd/response');

_View	=	class.new();

function _View:constructor()
	self.viewDir	=	'Views';
end

function _View:render(str)
	-- Handle includes
	str	=	string.gsub(str, "%@include%(%s*[%\'%\"]([0-9A-Za-z%.%-%_]*)[%\'%\"]%s*%)", function(viewname)
		local subView	=	class.new(self);
		local result	=	subView:make(viewname);
		return result.content;
	end);

	-- Handle foreaches
	str	=	string.gsub(str, "%@foreach%(%s*(.-)%s+as%s+([0-9A-Za-z%_]*),%s*([0-9A-Za-z%_]*)%)%s*(%b{})", function(tabGiven, indexName, valueName, subContent)
		-- Chop off the encapsulating { and }
		subContent	=	string.sub(subContent, 2, -2);
		local i,v;
		local tab = {};
		local chunk	=	load('return(' .. tabGiven .. ')');
		if( _ENV[tabGiven] ) then
			tab = _ENV[tabGiven];
		elseif( type(chunk) == 'function' ) then
			tab = chunk();
		else
			return nil;
		end

		local replacement = '';
		for i,v in pairs(tab) do
			_ENV[indexName]	=	i;
			_ENV[valueName]	=	v;
			replacement = replacement .. self:render(subContent);
		end

		return replacement;
	end);

	-- Handle fors
	str	=	string.gsub(str, "%@for%(%s*([0-9A-Za-z%_]*)%s*=%s*([0-9A-Za-z%_]*)%s*,%s*([0-9A-Za-z%_]*)%)%s*(%b{})", function(varName, startCount, endCount, subContent)
		-- Chop off the encapsulating { and }
		subContent	=	string.sub(subContent, 2, -2);

		local replacement = '';
		for i = startCount,endCount do
			_ENV[varName]	=	sprintf("%d", i);
			replacement		=	replacement .. self:render(subContent);
		end

		return replacement;
	end);

	-- Handle if-else statements
	str	=	string.gsub(str, "%@if%s*(%b())%s*(%b{})%s*else%s*(%b{})", function(condition, ifContent, elseContent)
		condition	=	string.sub(condition, 2, -2);
		local chunk	=	load('return(' .. condition ..')');
		local status,result	=	pcall(chunk);

		if( not status ) then
			return nil;
		end

		if( result ) then
			return self:render(string.sub(ifContent, 2, -2)); -- "Chop of { and }, but render the contents
		else
			return self:render(string.sub(elseContent, 2, -2));
		end
	end);

	-- Handle standard if statements
	str	=	string.gsub(str, "%@if%s*(%b())%s*(%b{})", function(condition, subContent)
		condition	=	string.sub(condition, 2, -2);
		local chunk	=	load('return(' .. condition ..')');
		local status,result	=	pcall(chunk);

		if( not status ) then
			return nil;
		end

		if( result ) then
			return self:render(string.sub(subContent, 2, -2)); -- "Chop of { and }, but render the contents
		else
			return "";
		end
	end);

	-- Handle substitutions
	str	=	string.gsub(str, "{{#(.-)#}}", function(inner) -- Non-output
		inner	=	string.gsub(inner, "%%}", "}");
		inner	=	string.gsub(inner, "%%{", "{");

		local chunk;
		if( string.match(inner, "%s*if%s*%b()%s*then%s*.-end") or string.match(inner, "%x+%s*=%s*.*") ) then
			chunk = load(inner);
		else
			chunk = load('return('.. inner .. ')');
		end
		local status,result	=	pcall(chunk)

		return '';
	end);

	str	=	string.gsub(str, "{{!(.-)!}}", function(inner) -- Un-escaped
		inner	=	string.gsub(inner, "%%}", "}");
		inner	=	string.gsub(inner, "%%{", "{");

		local chunk;
		if( string.match(inner, "%s*if%s*%b()%s*then%s*.-end") or string.match(inner, "%x+%s*=%s*.*") ) then
			chunk = load(inner);
		else
			chunk = load('return('.. inner .. ')');
		end
		local status,result	=	pcall(chunk)

		if( status ) then
			if( result == nil ) then
				return 'nil';
			else
				return result;
			end
		end
		return result;
	end);

	str	=	string.gsub(str, "{{(.-)}}", function(inner) -- Escaped
		inner	=	string.gsub(inner, "%%}", "}");
		inner	=	string.gsub(inner, "%%{", "{");

		local chunk;
		if( string.match(inner, "%s*if%s*%b()%s*then%s*.-end") or string.match(inner, "%x+%s*=%s*.*") ) then
			chunk = load(inner);
		else
			chunk = load('return('.. inner .. ')');
		end
		local status,result	=	pcall(chunk)

		if( status ) then
			if( result == nil ) then
				return 'nil';
			else
				return htmlentities(result);
			end
		end
		return result;
	end);

	return str;
end

function _View:make(fileName, data)
	data	=	data or {};
	local fullPath	=	self.viewDir .. fileName .. '.view.lua';
	if( not filesystem.fileExists(fullPath) ) then
		printf("Could not load view %s\n", fullPath);
		return stdError(500, sprintf("Could not load view %s", fullPath));
	end

	local file	=	io.open(fullPath, 'r');
	local str	=	file:read("*all");
	file:close();


	-- Make data available
	local origEnvVars = {};
	for i,v in pairs(data) do
		origEnvVars[i]	=	_ENV[i];
		_ENV[i]			=	v;
	end

	local sections	=	{};
	str = string.gsub(str, "%@beginsection%(%s*[%\'%\"](.-)[%\'%\"]%s*%)(.-)%@endsection", function(sectionName, sectionContent)
		sections[sectionName]	=	sectionContent;
		return sectionContent;
	end);

	-- If this view extends another, we need to load it and insert our sections into it
	local template = string.match(str, "^%s*@extends%(%s*[%\'%\"]([0-9A-Za-z%.%-%_]*)[%\'%\"]%s*%)");
	if( template and template ~= fileName ) then
		-- Chop first line out (extension details)
		str	=	string.sub(str, string.find(str, "\n")+1);
		local subView	=	class.new(self);
		local result	=	subView:make(template);
		str	=	string.gsub(result.content, "@section%(%s*[%\'%\"](.-)[%\'%\"],?%s*[%\'%\"]?(.-)[%\'%\"]?%s*%)", function(sectionName, defaultContent)
			return sections[sectionName] or defaultContent or "";
		end);
	end

	-- Avoid pattern errors before final rendering
	--str = string.gsub(str, '%%', '%%%%')

	-- Render content
	str	=	self:render(str);

	-- Revert env vars to original values
	for i,v in pairs(data) do
		_ENV[i]	=	origEnvVars[i];
	end

	return Response(200, str);
end

View	=	_View();