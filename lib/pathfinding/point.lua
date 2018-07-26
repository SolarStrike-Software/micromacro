local Point = class.new();
Pathfinding.Point = Point;

function Point:constructor(x, y, connections)
	self.x				=	x;
	self.y 				=	y;
	self.connections	=	connections or {};
	self.gCost			=	0;
	self.hCost			=	0;
	self.parent			=	nil;
end

function Point:connectTo(otherPoint)
	if( not self:isConnectedTo(otherPoint) ) then
		table.insert(self.connections, otherPoint);
	end
end

function Point:disconnect(otherPoint)
	for i,v in pairs(self.connections) do
		if( v == otherPoint ) then
			table.remove(self.connections,i);
			return;
		end
	end
end

function Point:isConnectedTo(otherPoint)
	for i,v in pairs(self.connections) do
		if( v == otherPoint ) then
			return true;
		end
	end

	return false;
end

function Point:getConnections()
	return self.connections;
end

function Point:getFCost()
	return self.gCost + self.hCost;
end

function Point:getGCost()
	return self.gCost;
end

function Point:setGCost(val)
	self.gCost = val;
end

function Point:getHCost()
	return self.hCost;
end

function Point:setHCost(val)
	self.hCost = val;
end

function Point:getParent()
	return self.parent;
end

function Point:setParent(val)
	self.parent = val;
end