require('pathfinding/point');

local Point = Pathfinding.Point;

--[[
	Checks if a specific point is along a segment. Returns true if it is, otherwise false.
	All three arguments should be a Point object
]]
function Pathfinding.pointOnSegment(point, segmentStart, segmentEnd)
	local dx,dy = segmentEnd.x - segmentStart.x, segmentEnd.y - segmentStart.x;
	return (((point.x - segmentStart.x) * dy) == ((point.y - segmentStart.y) * dx));
end

--[[
	Finds the point along two segments (points point1,point2 and point3,point4) that intersect.
	If constrain is true, then the two segments must overlap. Otherwise, the intersection point may be outside of the segments.

	Returns a Point at the point of intersection if one is available, otherwise returns nil
]]
function Pathfinding.lineIntersection(vertex1, vertex2, vertex3, vertex4, constrain)
	assert(vertex1 and vertex2 and vertex3 and vertex3);
	assert(vertex1.x and vertex1.y);
	assert(vertex2.x and vertex2.y);
	assert(vertex3.x and vertex3.y);
	assert(vertex4.x and vertex4.y);



	local line1 = {x1 = vertex1.x, y1 = vertex1.y, x2 = vertex2.x, y2 = vertex2.y};
	local line2 = {x1 = vertex3.x, y1 = vertex3.y, x2 = vertex4.x, y2 = vertex4.y};
	local denominator = (line1.x2 - line1.x1)*(line2.y2 - line2.y1) - (line2.x2 - line2.x1)*(line1.y2 - line1.y1);

	-- If denominator is 0, lines are parallel
	if( denominator == 0 ) then
		return nil;
	end

	local nA	=	(line2.x2 - line2.x1)*(line1.y1 - line2.y1) - (line2.y2 - line2.y1)*(line1.x1 - line2.x1);
	local nB	=	(line1.x2 - line1.x1)*(line1.y1 - line2.y1) - (line1.y2 - line1.y1)*(line1.x1 - line2.x1);

	-- Calculate intermediate fractional point
	local uA	=	nA / denominator;
	local uB	=	nB / denominator;

	if( constrain ) then
		-- We could check if fractional point (both uA and uB) is between (or includes) 0 and 1
		-- to check if the segments overlap
		-- If uA and uB are outside of the range of 0..1, then the segments do not overlap
		if( uA < 0 or uA > 1 or uB < 0 or uB > 1 ) then
			return nil;
		end

		-- If we're touching end pieces, don't count is as an intersection
		if( (uA == 0 or uA == 1) and (uB == 0 or uB == 1) ) then
			return nil;
		end
	end

	local nX	=	line1.x1 + (uA * (line1.x2 - line1.x1));
	local nY	=	line1.y1 + (uA * (line1.y2 - line1.y1));
	return Point(nX, nY);
end


-- Checks if there's visibility between two nodes.
-- Returns a boolean - true if there's visibility, otherwise returns false
function Pathfinding.checkVisibility(polygons, point1, point2)
	-- Iterate over every segment in all polygons, check for line collisions
	for i,polygon in pairs(polygons) do

		-- If this line spans across the interior of the polygon we don't have visibility
		local midpoint = Point((point1.x + point2.x) / 2, (point1.y + point2.y) / 2);
		local inPoly, onPoly = clipper.pointInPoly(midpoint, polygon:getPoints());
		if( inPoly ) then
			return false;
		end


		for j,point in pairs(polygon:getPoints()) do
			local v1 = polygon:getPoint(j);
			local v2 = polygon:getPoint(polygon:nextIndex(j));

			local intersection = Pathfinding.lineIntersection(point1, point2, v1, v2, true);
			if( intersection ~= nil ) then
				return false;
			end

		end
	end

	return true;
end