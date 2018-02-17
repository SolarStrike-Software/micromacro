require('neuralnet/neuron');
require('neuralnet/synapse');

NeuralNet	=	class.new();

function NeuralNet:constructor(...)
	local topology;
	if( type(...) == "table" ) then
		topology = ...;
	else
		topology = {...};
	end

	self.layers	=	{};
	self.recentAverageError = 1.0;

	local numLayers	=	#topology;
	for i = 1,numLayers do
		-- Create a new layer
		local newLayer = {};
		table.insert(self.layers, newLayer);

		local numOutputs = 0;		-- Last layer should always have zero outputs
		if( i < numLayers ) then	-- If not last layer, have x outputs
			numOutputs	=	topology[i+1];
		end

		-- Fill the layer's neurons, add a bias neuron in each layer
		for index = 1, topology[i]+1 do
			table.insert(newLayer, Neuron(index, numOutputs));
		end

		-- Force bias to 1.0
		newLayer[#newLayer]:setOutputValue(1.0);
	end
end

function NeuralNet:feed(...)
	local inputs;
	if( type(...) == "table" ) then
		inputs = ...;
	else
		inputs = {...};
	end
	assert(#inputs == #self.layers[1] - 1, "Number of inputs does not match network topology.");

	-- Set input values into first layer
	for i,v in pairs(inputs) do
		self.layers[1][i]:setOutputValue(v);
	end



	-- Propagate forward
	for i = 2,#self.layers do
		local prevLayer	=	self.layers[i - 1];
		for j = 1, #self.layers[i] - 1 do
			local k = self.layers[i][j];
			k:feed(prevLayer);
		end
	end
end

function NeuralNet:backPropagation(...)
	local targets;
	if( type(...) ~= "table" ) then
		targets = {...};
	else
		targets = ...;
	end
	table.insert(targets, 1.0); -- Insert a 1.0 for the bias neuron

	-- Calculate RMS
	local layer	=	self.layers[#self.layers];
	local rms	=	0.0;

	for i = 1,#layer-1 do -- Iterate over each neuron in the last layer except bias
		local delta = targets[i] - layer[i]:getOutputValue();
		rms = rms + delta * delta;
	end
	rms = rms / (#layer-1);
	rms = math.sqrt(rms);

	-- Recent average RMS
	local recentAverageSmoothing = 100.0;
	self.recentAverageError = (self.recentAverageError * recentAverageSmoothing + rms) / (recentAverageSmoothing + 1.0);
	
	for i = 1,#layer-1 do
		-- Calculate gradient while we're at it
		layer[i]:calculateOutputGradients(targets[i]);
	end

	-- Calculate hidden layer gradients
	for i = #self.layers - 1, 2, -1 do
		local hiddenLayer	=	self.layers[i];
		local nextLayer		=	self.layers[i+1];

		-- Iterate over each neuron in hidden layer
		for j,k in pairs(hiddenLayer) do
			k:calculateHiddenGradients(nextLayer);
		end
	end

	-- Now update synapse weights
	for l = #self.layers, 2, -1 do
		local layer		=	self.layers[l];
		local prevLayer	=	self.layers[l-1];

		for i = 1, #layer - 1 do
			layer[i]:updateInputWeights(prevLayer);
		end
	end

	table.remove(targets); -- Pop last element to remove the bias neuron
end

function NeuralNet:getRecentAverageError()
	return self.recentAverageError;
end

function NeuralNet:getResults()
	local results	=	{};
	local layer	=	self.layers[#self.layers]; -- Get last layer

	for i = 1,#layer - 1 do -- Chop off the bias neuron
		table.insert(results, layer[i]:getOutputValue());
	end

	return results;
end

-- Returns a table containing all the data to save the network (topology, layer data, neuron weights)
function NeuralNet:getExportTable()
	local export = {};

	export = {};
	for i,layer in pairs(self.layers) do
		local tmpLayerData = {};
		for n,neuron in pairs(layer) do
			local tmpNeuronData = {};
			for s, synapse in pairs(neuron.synapses) do
				table.insert(tmpNeuronData, synapse.weight);
			end
			table.insert(tmpLayerData, tmpNeuronData);
		end
		table.insert(export, tmpLayerData);
	end

	return export;
end

-- Saves to a file
function NeuralNet:save(filename)
	local file = io.open(filename, "w");
	local export = self:getExportTable();

	file:write("return {");
	for i = 1, #export do
		file:write("{");
		for j,k in pairs(export[i]) do
			file:write("{");
			for n,m in pairs(k) do
				if( n < #k ) then
					file:write(sprintf("%f,", m));
				else
					file:write(sprintf("%f", m));
				end
			end
			if( j < #export[i] ) then
				file:write("},");
			else
				file:write("}");
			end
		end

		if( i < #export) then
			file:write("},");
		else
			file:write("}");
		end
	end
	file:write("}");
	file:close();
end

-- Load from a file
function NeuralNet:load(filename)
	local file = io.open(filename, "r");

	if( not file ) then
		error(sprintf("Could not open `%s` for reading.", filename), 2);
	end
	local data = file:read("*all");
	local chunk = load(data);
	local status, import = pcall(chunk);

	if( not status ) then
		error(sprintf("`%s` does not appear to be a valid neural network table", filename), 2);
	end

	if( type(import) ~= "table" ) then
		error("File does not contain a valid table.", 2);
	end

	local topology = {};
	for i,v in pairs(import) do
		table.insert(topology, #v - 1);
	end

	self:constructor(topology); -- Recreate network topology

	-- Step through, reassign weights
	for i = 1, #self.layers-1 do
		for j,neuron in pairs(self.layers[i]) do
			for n,synapse in pairs(neuron.synapses) do
				synapse.weight = import[i][j][n];
			end
		end
	end
end