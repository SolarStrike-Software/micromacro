Request	=	class.new();

function Request:constructor()
	self.method			=	'get';
	self.fragment		=	'';
	self.query			=	{};
	self.post			=	{};
	self.files			=	{};
	self.uriSegments	=	{};
	self.cookies		=	{};

	-- Info from cookies & flashvars
	self.oldInputs		=	{};
	self.errors			=	nil;
end

function Request:input(name)
	if( self.method == 'get' ) then
		return self.query[name];
	elseif( self.method == 'post' ) then
		return self.post[name];
	end
end

function Request:file(name)
	return self.files[name];
end

function Request:oldInput(name, default)
	return self.oldInputs[name] or default or '';
end

function Request:all()
	if( self.method == 'get' ) then
		return self.query;
	elseif( self.method == 'post' ) then
		return self.post;
	end
end

function Request:getCookie(name)
	return self.cookies[name];
end

function Request:uriSegment(index, default)
	return self.uriSegments[index] or default;
end

function Request:getErrors()
	local fullstr = '';
	for i,v in pairs(self.errors) do
		fullstr	=	fullstr .. sprintf('%s<br />\n', htmlentities(v));
	end
	return fullstr;
end