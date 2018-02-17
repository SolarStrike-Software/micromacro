Neuron	=	class.new();

function Neuron:constructor(index, numOutputs)
	self.index		=	index;
	self.value		=	1.0;
	self.synapses	=	{};
	self.gradient	=	0.0;
	self.numOutputs	=	numOutputs

	for i = 1,numOutputs + 1 do
		table.insert(self.synapses, Synapse(self:randomWeight(), 0));
	end
end

function Neuron:getOutputValue()
	return self.value;
end

function Neuron:setOutputValue(value)
	self.value	=	value;
end

function Neuron:getOutputWeight(index)
	return self.synapses[index].weight;
end

function Neuron:getGradient()
	return self.gradient;
end

function Neuron:feed(prevLayer)

	-- Step 1: Sum up previous layer's outputs, including bias
	local sum	=	0.0;

	for i,v in pairs(prevLayer) do
		sum	=	sum +
			v:getOutputValue() *
			v:getOutputWeight(self.index);
	end

	self.value	=	self:transferFunction(sum);
end

function Neuron:calculateOutputGradients(targetValue)
	local delta		=	targetValue - self.value;
	self.gradient	=	delta * self:transferFunctionDerivative(self.value);
end

function Neuron:calculateHiddenGradients(nextLayer)
	local dow		=	self:sumDOW(nextLayer);
	self.gradient	=	dow * self:transferFunctionDerivative(self.value);
end

function Neuron:updateInputWeights(prevLayer)
	local	eta		=	0.3;	-- Overall Net learning rate, should be in range [0..1]
	local	alpha	=	0.7;	-- Momentum, multiple of last deltaWeight, should be in range [0..1]

	for i,v in pairs(prevLayer) do
		local prevLayerNeuron	=	v;
		local oldDeltaWeight	=	prevLayerNeuron.synapses[self.index].deltaWeight;

		local newDeltaWeight	=	eta * prevLayerNeuron:getOutputValue() * self.gradient + alpha * oldDeltaWeight;
		local newWeight			=	prevLayerNeuron.synapses[self.index].weight + newDeltaWeight;
		--newWeight = math.min(math.max(newWeight, 0.0), 1.0);
		prevLayerNeuron.synapses[self.index].deltaWeight	=	newDeltaWeight;
		prevLayerNeuron.synapses[self.index].weight			=	newWeight;
	end
end

function Neuron:transferFunction(x)
	return math.tanh(x);
end

function Neuron:transferFunctionDerivative(x)
	return 1.0 - x*x;
end

function Neuron:sumDOW(nextLayer)
	-- Sum of contribution to errors at the nodes we feed from this neuron
	local sum = 0.0;
	for i,v in pairs(nextLayer) do
		sum = sum + (self.synapses[i].weight * nextLayer[i]:getGradient());
	end

	return sum;
end

function Neuron:randomWeight()
	return math.random();	-- Without arguments, returns a value within the range [0..1]
end