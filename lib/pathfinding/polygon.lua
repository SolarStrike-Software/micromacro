require('pathfinding/point');
local Polygon = class.new();
Pathfinding.Polygon = Polygon;

local Point = Pathfinding.Point;

function Polygon:constructor(points)
	self.points = {};

	if( type(points) == "table" ) then
		for i,v in pairs(points) do -- Copy table rather than go by reference
			table.insert(self.points, Point(v.x, v.y));
		end
	end
end

function Polygon:toNodes()
	local nodes = {};

	-- Create a copy of the points first
	for i,point in pairs(self.points) do
		table.insert(nodes, Point(point.x, point.y));
	end

	-- Iterate over it again and connect the points
	for i,point in pairs(nodes) do
		-- Connect this point to previous & next points
		local prevIndex = self:previousIndex(i);
		local nextIndex = self:nextIndex(i);
		point:connectTo(nodes[prevIndex]);
		point:connectTo(nodes[nextIndex]);
	end

	return nodes;
end

function Polygon:getPoint(index)
	if( index < 1 or index > #self.points ) then
		return nil;
	end

	return self.points[index];
end

function Polygon:getPoints()
	return self.points;
end

-- Get the index of a vertice that is previous to 'fromIndex'; Loops around
function Polygon:previousIndex(fromIndex)
	assert(#self.points); -- Cannot get previous if there's no vertices
	assert(fromIndex ~= 0); -- Cannot be 0
	if( fromIndex < 0 ) then
		-- If negative, loop around in reverse.
		local absIndex = math.abs(fromIndex) % #self.points;
		fromIndex =  #self.points - absIndex + 1;
	end

	local newIndex = (fromIndex - 1);
	if( newIndex == 0 ) then
		newIndex = #self.points;
	end
	return newIndex;
end

-- Get the index of a vertice that is next to 'fromIndex'; Loops around
function Polygon:nextIndex(fromIndex)
	assert(#self.points); -- Cannot get previous if there's no vertices
	assert(fromIndex ~= 0); -- Cannot be 0
	if( fromIndex < 0 ) then
		-- If negative, loop around in reverse.
		local absIndex = math.abs(fromIndex) % #self.points;
		fromIndex =  #self.points - absIndex + 1;
	end

	local newIndex = (fromIndex + 1);
	if( newIndex == #self.points + 1 ) then
		newIndex = 1;
	end
	return newIndex;
end

-- Scale the polygon up by some amount. A negative number will scale down.
-- Because it is possible that a polygon will be split into multiple polygons in this process,
-- a table of polygons is expected to be returned
function Polygon:offset(amount, joinType)
	local joinType = joinType or 'miter';
	local solutions = clipper.offset(self.points, amount, joinType);

	local polygons = {};
	for i,v in pairs(solutions) do
		table.insert(polygons, Polygon(v));
	end

	return polygons;
end