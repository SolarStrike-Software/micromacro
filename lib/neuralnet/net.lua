require('neuralnet/neuron');
require('neuralnet/synapse');

NeuralNet	=	class.new();

function NeuralNet:constructor(...)
	local topology = {...};
	self.layers	=	{};

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
		--printf("Neurons for layer: %d, outputs: %d\n", topology[i], numOutputs);
		for index = 1, topology[i]+1 do
			--print("Inserting new neuron", index, "into layer", i, "Outputs", numOutputs);
			table.insert(newLayer, Neuron(index, numOutputs));
		end

		-- Force bias to 1.0
		newLayer[#newLayer]:setOutputValue(1.0);
	end
end

function NeuralNet:feed(inputs)
	assert(#inputs == #self.layers[1] - 1, "Number of inputs does not match network topology.");

	-- Set input values into first layer
	for i,v in pairs(inputs) do
		self.layers[1][i]:setOutputValue(v);
	end

	-- Propagate forward
	for i = 2,#self.layers do
		local prevLayer	=	self.layers[i - 1];
		for j,k in pairs(self.layers[i]) do
			k:feed(prevLayer);
		end
	end
end

function NeuralNet:backPropagation(...)
	local targets = {...};
	table.insert(targets, 1.0); -- Insert a 1.0 for the bias neuron

	-- Calculate RMS
	local layer	=	self.layers[#self.layers];
	local rms	=	0.0;

	for i = 1,#layer do -- Iterate over each neuron in the last layer
		local delta = targets[i] - layer[i]:getOutputValue();
		rms = rms + delta * delta;

		-- Calculate gradient while we're at it
		layer[i]:calculateOutputGradients(targets[i]);
	end
	rms = math.sqrt(rms / (#layer-1));
	
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

		for i,v in pairs(layer) do -- Iterate over each neuron
			v:updateInputWeights(prevLayer);
		end
	end
end

function NeuralNet:getResults()
	local results	=	{};
	local layer	=	self.layers[#self.layers]; -- Get last layer

	for i = 1,#layer - 1 do -- Chop off the bias neuron
		table.insert(results, layer[i]:getOutputValue());
	end

	return results;
end