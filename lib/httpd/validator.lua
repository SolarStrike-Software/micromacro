Validator = class.new();

function Validator:constructor(rules)
	self.errorMessages	=	{};
	self.rules			=	rules or {};
end

function Validator:setRules(rulesTable)
	self.rules = rulesTable;
end

function Validator:setRule(field, rule)
	self.rules[field] = rule;
end

function Validator:passes(inputs)
	local passed = true;
	for field,rules in pairs(self.rules) do
		-- Run through each rule
		for i,fullRule in pairs(string.explode(rules, '|')) do
			-- Parse out extra values from the rule name
			local ruleValuesPair = string.explode(fullRule, ':');
			local rule = ruleValuesPair[1];
			local ruleValues = ruleValuesPair[2] or '';

			--print("Rule:", rule, "On:", field);

			local func = self[rule];
			if( type(func) == "function" ) then
				-- Try running the target function and see what happens
				local errStatus, success, failMsg = pcall(func, self, field, inputs[field],
					table.unpack(string.explode(ruleValues, ','))	-- Make sure we include our extra data to pass along
				);

				-- Don't suppress any errors.
				if( not errStatus ) then
					error(success, 0);
				end

				-- If it didn't pass, save the error message and skip to the next input
				if( not success ) then
					passed = false;
					self.errorMessages[field] = failMsg;
					break;
				end
			end
		end
	end

	return passed;
end

function Validator:required(field, value)
	if( value ~= nil and value ~= "" ) then
		return true;									-- Make sure they gave us a value
	else
		return false, sprintf("The %s field is required.", field);
	end
end

Validator['min-len']	=	function(self, field, value, minAmount)
	if( value == nil ) then
		return true;
	end
	minAmount = tonumber(minAmount);

	local succeeded = true;
	local failMsg = '';

	if( type(value) == "string" or type(value) == "number" ) then		-- Make sure length is at least x characters
		succeeded = string.len(tostring(value)) >= minAmount;
		failMsg = sprintf("The %s field must be at least %d characters.", field, minAmount);
	end

	if( not succeeded ) then
		return false, failMsg;
	end

	return succeeded;
end

function Validator:min(field, value, minAmount)
	if( value == nil ) then
		return true;
	end
	minAmount = tonumber(minAmount);

	local succeeded = true;
	local failMsg = '';
	local isNumber	=	tonumber(value) ~= nil;

	if( isNumber ) then				-- Make sure the value is >= x
		succeeded = tonumber(value) >= minAmount;
		failMsg = sprintf("The %s field must be at least %d.", field, minAmount);
	end

	if( not succeeded ) then
		return false, failMsg;
	end

	return succeeded;
end

Validator['max-len']	=	function(self, field, value, maxAmount)
	if( value == nil ) then
		return true;
	end
	maxAmount = tonumber(maxAmount);

	local succeeded = true;
	local failMsg = '';

	if( type(value) == "string" or type(value) == "number" ) then					-- Make sure length is less than x characters
		succeeded = string.len(tostring(value)) <= maxAmount;
		failMsg = sprintf("The %s field cannot be longer than %d characters.", field, maxAmount);
	end

	if( not succeeded ) then
		return false, failMsg;
	end

	return succeeded;
end

function Validator:max(field, value, maxAmount)
	if( value == nil ) then
		return true;
	end
	maxAmount = tonumber(maxAmount);

	local succeeded = true;
	local failMsg = '';
	local isNumber	=	tonumber(value) ~= nil;

	if( isNumber ) then				-- Make sure the value is >= x
		succeeded = tonumber(value) <= maxAmount;
		failMsg = sprintf("The %s field cannot be greater than %d.", field, maxAmount);
	end

	if( not succeeded ) then
		return false, failMsg;
	end

	return succeeded;
end

function Validator:type(field, value, ...)
	if( value == nil ) then
		return true;
	end
	local typesAccepted = {...};
	if( #typesAccepted == 0 ) then
		error("You need to pass at least one type to this validator.", 2);
	end

	local acceptedTypesStr = typesAccepted[1];
	for i = 2,#typesAccepted do
		acceptedTypesStr = acceptedTypesStr .. ', ' .. typesAccepted[i];
	end

	-- We forcefully converted it to a string, so we might need to correct it.
	if( tonumber(value) ) then
		value = tonumber(value);
	end

	local found = table.find(typesAccepted, type(value));
	if( not found ) then					-- Make sure length is less than x characters
		succeeded = false
		return false, sprintf("The %s field must be one of these: %s.", field, acceptedTypesStr);
	end

	return true;
end

Validator['in'] = function (self, field, value, ...) -- We do this to get around that 'in' is a reserved name
	if( value == nil ) then
		return true;
	end
	local typesValues = {...};
	if( #typesValues == 0 ) then
		error("You need to pass at least one value to this validator.", 2);
	end

	local typesValuesStr = typesValues[1];
	for i = 2,#typesValues do
		typesValuesStr = typesValuesStr .. ', ' .. typesValues[i];
	end

	local found = table.find(typesValues, value);
	if( not found ) then					-- Make sure length is less than x characters
		succeeded = false
		return false, sprintf("The %s field must be one of these: %s.", field, typesValuesStr);
	end

	return true;
end

function Validator:getErrors()
	return self.errorMessages;
end
