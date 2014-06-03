--[[ Logging settings ---------------------------------------------------------
	logRemovalDays				Remove logs older than this number in days (0 = disable)
	logDirectory				Location to store log files
]]
logRemovalDays = 1; 
logDirectory = "logs";


--[[ Script settings ----------------------------------------------------------
	scriptDirectory				This is the default script directory. Do not include trailing slash.
]]
scriptDirectory = "./scripts";


--[[ Memory & Process settings ------------------------------------------------
	memoryStringBufferSize		Size of the buffer (in bytes) used when handling strings in memory
]]
memoryStringBufferSize = 128;


--[[ Other things
	audioEnabled				Whether or not to enable OpenAl sound output
	yieldTimeSlice				Whether or not to yield time back to the system. true = reduce CPU usage, false = max CPU usage, but still yield to other processes
]]
audioEnabled = true;
yieldTimeSlice = true;