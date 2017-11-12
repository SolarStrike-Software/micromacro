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
	--print("Getting weight for", index);
	return self.synapses[index].weight;
end

function Neuron:getGradient()
	return self.gradient;
end

function Neuron:feed(prevLayer)
	-- Step 1: Sum up previous layer's outputs
	local sum	=	0.0;

	for i,v in pairs(prevLayer) do
		--print("\t", i, v);
		sum	=	sum +
			v:getOutputValue() *
			v:getOutputWeight(self.index);
			--v.synapses[self.index].weight;
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
	local	eta		=	0.15;	-- Overall Net learning rate, should be in range [0..1]
	local	alpha	=	0.5;	-- Momentum, multiple of last deltaWeight, should be in range [0..1]

	for i,v in pairs(prevLayer) do
		local prevLayerNeuron	=	v;
		local oldDeltaWeight	=	prevLayerNeuron.synapses[self.index].deltaWeight;

		local newDeltaWeight	=	eta * prevLayerNeuron:getOutputValue() * self.gradient + alpha  * oldDeltaWeight;
		prevLayerNeuron.synapses[self.index].deltaWeight	=	newDeltaWeight;
		prevLayerNeuron.synapses[self.index].weight			=	prevLayerNeuron.synapses[self.index].weight + newDeltaWeight;
	end
end

function Neuron:transferFunction(x)
	-- tanh() implementation
	if( x == 0 ) then
		return 0.0;
	end

	local neg = false;

	if( x < 0 ) then
		x = -x;
		neg = true;
	end

	if( x < 0.54930614433405 ) then
		local y = x * x;
		x = x + x * y *
		((-0.96437492777225469787e0  * y +
		  -0.99225929672236083313e2) * y +
		  -0.16134119023996228053e4) /
		(((0.10000000000000000000e1  * y +
		   0.11274474380534949335e3) * y +
		   0.22337720718962312926e4) * y +
		   0.48402357071988688686e4);
	else
		x = math.exp(x);
		x = 1.0 - 2.0 / (x * x + 1.0);
	end

	if( neg ) then
		x = -x;
	end

	return x;
end

function Neuron:transferFunctionDerivative(x)
	return 1.0 - x*x;
end

function Neuron:sumDOW(nextLayer)
	-- Sum of contribution to errors at the nodes we feed from this neuron
	local sum = 0.0;
	for i,v in pairs(nextLayer) do
		sum	=	sum + self.synapses[i].weight * nextLayer[i]:getGradient();
	end

	return sum;
end

function Neuron:randomWeight()
	return math.random();	-- Without arguments, returns a value within the range [0..1]
end