--[[	Language & translation helper
	License: Modified BSD; see license.txt



	To use, place this in your initialization:
		require('language');
		Lang:init('lang', 'en');
	
	Where 'lang' is the name of your language folder, and 'en' is the name of your language.

	Place nested translations in the script's path as follows:
	myScript/
		lang/
			en/
				whatever.lua
				somethingelse.lua
				etc.lua
			de/
				whatever.lua
				somethingelse.lua
				etc.lua

	Where 'en', 'de', etc. represent your languages of choice.


	Example of a translation file (en/whatever.lua):
		return {
			['hello.world'] = 'Hello World';
		};

	Example of alternative translation file (de/whatever.lua):
		return {
			['hello.world'] = 'Hallo Welt';
		};


	To use a translated string:
		print("Hello World:", Lang:get('whatever.hello.world')
]]

Lang = class.new();

function Lang:constructor()
	self.translations = {};
	self.loaded = {};
	self.lang = 'en';
	self.dir = 'lang';
end
Lang:constructor();

function Lang:init(dir, lang)
	if( dir ) then
		self.dir = dir;
	else
		self.dir = filesystem.getCWD() .. '/lang';
	end

	if( lang ) then
		self.lang = lang;
	end
	self.loaded = {};
end

function Lang:setLanguage(lang)
	if( self.lang ~= lang ) then
		-- We're actually changing the language, so must reset our loaded string
		self.loaded = {};
	end
	self.lang = lang;
end

function Lang:get(szName)
	local file = 'global';
	local lookupName = szName;

	-- Check to see if a sub-file was given, parse it and the string name from it.
	local fileName,subName = string.match(szName, '^(%w*)%.(%w*)');
	if( fileName and subName ) then
		file = fileName;
		lookupName = subName;
	end

	-- Load the file if needed.
	if( file ) then
		if( not self.loaded[file] ) then
			local fullpath = self.dir .. "/" .. self.lang .. "/" .. file .. ".lua";
			if( filesystem.fileExists(fullpath) ) then
				self.loaded[file] = include(fullpath);
			end
		end
	end

	-- Lookup the string, return it if it exists.
	if( self.loaded[file] ) then	-- Double-check this in case the loading failed.
		if( self.loaded[file][lookupName] ) then
			return self.loaded[file][lookupName];
		else
			return szName;
		end
	end

	return szName;
end