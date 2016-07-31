-- Helper to camelize method names
function camelize(str)
	str	=	string.gsub(str, "(%-%l)", function(capture)
		return string.upper(string.sub(capture, 2));
	end);
	return str;
end

function getMimeType(filename)
	local extension	=	string.match(filename, "%.([0-9A-Za-z]*)$");
	local readAs	=	'r';

	local contentType	=	'text/html';

	-- Text/HTML or other plain types
	if( extension		==	'css' ) then
		contentType		=	'text/css';
	elseif( extension	==	'csv' ) then
		contentType		=	'text/csv';
	elseif( extension	==	'text' or extension == 'txt' ) then
		contentType		=	'text/plain';
	elseif( extension	==	'xml' ) then
		contentType		=	'application/xml';

	-- Scripts
	elseif( extension	==	'js' ) then
		contentType		=	'text/javascript';
	elseif( extension	==	'php' ) then
		contentType		=	'redirect/php';
	elseif( extension	==	'lua' ) then
		contentType		=	'text/lua';

	-- Fonts
	elseif( extension	==	'eot' ) then
		contentType		=	'application/vnd.ms-fontobject';
		readAs			=	'rb';
	elseif( extension	==	'ttf' ) then
		contentType		=	'application/x-font-ttf';
		readAs			=	'rb';
	elseif( extension	==	'woff' or extension == 'woff2' ) then
		contentType		=	'application/x-font-woff';
		readAs			=	'rb';


	-- Images
	elseif( extension	==	'bmp' ) then
		contentType		=	'image/bmp';
		readAs			=	'rb';
	elseif( extension	==	'ico' ) then
		contentType		=	'application/vnd.microsoft.icon';
		readAs			=	'rb';
	elseif( extension 	== 'jpeg' or extension == 'jpg' ) then
		contentType 	=	'image/jpeg';
		readAs			=	'rb';
	elseif( extension == 'png' ) then
		contentType		=	'image/png';
		readAs			=	'rb';
	elseif( extension		==	'svg' ) then
		contentType		=	'image/svg+xml';
		readAs			=	'rb';
	
	-- Audio/video
	elseif( extension	==	'mp2' ) then
		contentType		=	'audio/mpeg';
		readAs			=	'rb';
	elseif( extension	==	'mp3' ) then
		contentType		=	'video/mpeg';
		readAs			=	'rb';
	elseif( extension	==	'mp4' ) then
		contentType		=	'video/mp4';
		readAs			=	'rb';
	elseif( extension	==	'wmv' ) then
		contentType		=	'video/x-ms-wmv';
		readAs			=	'rb';
	elseif( extension	==	'wav' ) then
		contentType		=	'audio/wav';
		readAs			=	'rb';

	-- Compressed files/archives
	elseif( extension	==	'7z' ) then
		contentType		=	'application/x-7z-compressed';
		readAs			=	'rb';
	elseif( extension	==	'gzip' or extension == 'gz' ) then
		contentType		=	'application/x-gzip';
		readAs			=	'rb'
	elseif( extension	==	'rar' or string.match(extension, "r%d%d") ) then
		contentType		=	'application/x-rar-compressed';
		readAs			=	'rb';
	elseif( extension	==	'tar' ) then
		contentType		=	'application/x-tar';
		readAs			=	'rb';
	elseif( extension	==	'zip' or extension	==	'z' ) then
		contentType		=	'application/x-compressed';
		readAs			=	'rb';

	-- That's all, folks
	end

	return contentType, readAs;
end

function parseQueryString(str)
	str					=	urldecode(str);
	local queryParts	=	string.explode(str, "&");
	local queryTab		=	{};

	for i,v in pairs(queryParts) do
		--local varvalue	=	string.explode(v, "=");
		local	equalPos	=	string.find(v, "=");
		if( equalPos ) then
			local name		=	string.sub(v, 1, equalPos-1);
			local value		=	string.sub(v, equalPos+1);
			local index;

			-- TODO: decode HTML entities
			if( string.match(name, "%b[]") ) then
				-- We may have an array in the query string, so sort that out
				name, index	=	string.match(name, "(.*)(%b[])");
				index		=	string.sub(index, 2, -2);	-- Strip [ and ] from the capture

				local tab	=	queryTab[name] or {};
				if( index == "" ) then
					table.insert(tab, value);	-- No given index, just insert
				else
					table.insert(tab, index, value);
					--tab[index]	=	value;		-- Insert at given index
				end

				-- If we had to create the sub-table, remember to insert it
				if( not queryTab[name] ) then
					queryTab[name]	=	tab;
				end
			else
				queryTab[name]	=	value;
			end
		end
	end

	return queryTab;
end

function urldecode(str)
	str	=	string.gsub(str, "+", " ");
	str	=	string.gsub(str, "%%(%x%x)", function(hex)
		local dec	=	tonumber(hex, 16);
		return string.char(dec);
	end);
	return str;
end

function urlencode(str)
	str	=	string.gsub(str, "([^%a%d%-%_])", function(escapeme)
		local replacement = string.format("%%%02X", string.byte(escapeme));
		return replacement;
	end);

	return str;
end

function htmlentities(str)
	str	=	string.gsub(str, '&', '&amp;');
	str	=	string.gsub(str, '\"', '&quot;');
	str	=	string.gsub(str, '\'', '&apos;');
	str	=	string.gsub(str, '<', '&lt;');
	str	=	string.gsub(str, '>', '&gt;');
	return str;
end