CacheDriver = class.new();

function CacheDriver:constructor()
	self.name	=	'Base Cache Driver';
end

function CacheDriver:getName()
	return self.name;
end