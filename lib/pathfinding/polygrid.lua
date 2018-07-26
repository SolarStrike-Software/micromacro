require('pathfinding/math');

local PolyGrid = class.new();
Pathfinding.PolyGrid = PolyGrid;

local Point = Pathfinding.Point;
local Polygon = Pathfinding.Polygon;

function PolyGrid:constructor(nodes)
	self.nodes = nodes or {};
	self.offsetPolygons = {};
end

--[[
	(Re-)create a PolyGrid from a set of polygons
--]]
function PolyGrid:createFromPolygons(polygons, actorSize)
	self.nodes				=	{};
	self.offsetPolygons		=	{};


	-- Step 1: offset polygons
	for polyIndex,polygon in pairs(polygons) do
		-- Scale polygon up here
		local solutions = polygon:offset(actorSize);
		for i,v in pairs(solutions) do
			table.insert(self.offsetPolygons, v);
		end
	end

	-- Step 2: Now we can convert polygons to nodes
	for polyIndex, polygon in pairs(self.offsetPolygons) do
		-- Extract nodes from this polygon
		local newNodes = polygon:toNodes();
		for i,v in pairs(newNodes) do
			table.insert(self.nodes, v);
		end
	end

	-- Step 3: Check visibility between nodes, adding connections where there is visibility
	for i,v in pairs(self.nodes) do
		for j,k in pairs(self.nodes) do
			if( v ~= k ) then -- Don't try connecting to yourself...
				local vis = Pathfinding.checkVisibility(self.offsetPolygons, v, k);
				if( vis ) then
					-- Add a connection both ways
					v:connectTo(k);
					k:connectTo(v);
				end
			end
		end
	end

	return self;
end

function PolyGrid:findPath(startPoint, endPoint)

	--[[ Used to insert a new point into the grid, connect it to visible nodes, and returns it's index in the self.nodes table
		Useful for temporarily inserting the start/end points
	--]]
	local function addNode(point)
		-- Make a new copy of the point so we don't mess anything up in the original
		table.insert(self.nodes, Point(point.x, point.y));
		local index = #self.nodes;
		
		-- Check visibility between nodes, adding connections where there is visibility
		local v = self.nodes[index];
		print("index:", index);
		print("point:", v);
		for j,k in pairs(self.nodes) do
			if( v ~= k ) then -- Don't try connecting to yourself...
				local vis = Pathfinding.checkVisibility(self.offsetPolygons, v, k);
				if( vis ) then
					-- Add a connection both ways
					v:connectTo(k);
					k:connectTo(v);
				end
			end
		end

		return index;
	end

	-- Used to remove those (temporary?) nodes, specified by index
	local function removeNode(index)
		-- Remove any connections to this point first
		local node = self.nodes[index];
		for i,v in pairs(self.nodes) do
			if( i ~= index ) then
				v:disconnect(node);
			end
		end
		table.remove(self.nodes, index);
	end

	--[[ "Retrace" our steps to contruct a path by traversing childs & parents
	--]]
	local function retrace(startPoint, endPoint)
		local path = {};

		local function tableReverse(tab)
			-- Only have to swap first half of the table
			for i = 1, math.floor(#tab/2) do
				local tmp = tab[i];			-- Tmp copy this one.
				tab[i] = tab[#tab-i + 1];	-- Flip tab[i] and tab[n-i+1]; +1 because Lua tables start at index 1
				tab[#tab-i + 1] = tmp;
			end
			return tab;
		end

		local currentPoint = endPoint;
		while( currentPoint ~= startPoint ) do
			table.insert(path, Point(currentPoint.x, currentPoint.y));
			currentPoint = currentPoint.parent;
		end

		return tableReverse(path);
	end




	-- Insert the temporary points into the nodes table; we will remove them at the end so we must track their indices
	local tmpStartPointIndex, tmpEndPointIndex = addNode(startPoint), addNode(endPoint);

	-- Now that we've made our temporaries, we can override startPoint/endPoint for ease-of-use
	startPoint	=	self.nodes[tmpStartPointIndex];
	endPoint	=	self.nodes[tmpEndPointIndex];


	local openSet	=	{};
	local closedSet	=	{};

	table.insert(openSet, startPoint);
	while(#openSet > 0 ) do
		local node = openSet[1]; -- 'node' starts off pointing to our start point

		for i = 2, #openSet do
			if( openSet[i]:getFCost() <= node:getFCost() ) then
				if( openSet[i]:getHCost() < node:getHCost() ) then
					-- Total cost is lower & closer to goal, so update our 'node'
					node = openSet[i];
				end
			end
		end

		-- Move node from open to closed
		table.remove(openSet, table.find(openSet, node));
		table.insert(closedSet, node);

		-- Check if we've reached the goal
		if( node == endPoint or (node.x == endPoint.x and node.y == endPoint.y) ) then
			-- Retrace our steps
			local path = retrace(startPoint, endPoint);

			-- Remove temporary nodes
			removeNode(tmpStartPointIndex);
			removeNode(tmpEndPointIndex);

			return path;
		end


		-- Grab connections, set costs
		for i,neighbor in pairs(node:getConnections()) do
			-- Make sure it's not already closed so we aren't wasting time recalculating
			if( not table.find(closedSet, neighbor) ) then
				-- Calculate costs, add to open set
				local newCostToNeighbor = node:getGCost() + math.distance(node.x, node.y, neighbor.x, neighbor.y);
				if( newCostToNeighbor < neighbor:getGCost() or not table.find(openSet, neighbor) ) then
					neighbor:setGCost(newCostToNeighbor);
					neighbor:setHCost(math.distance(neighbor.x, neighbor.y, endPoint.x, endPoint.y));
					neighbor:setParent(node);

					if( not table.find(openSet, neighbor) ) then
						table.insert(openSet, neighbor);
					end
				end
			end
		end
	end

	-- Remove temporary nodes
	removeNode(tmpStartPointIndex);
	removeNode(tmpEndPointIndex);

	return false; -- No path found
end
