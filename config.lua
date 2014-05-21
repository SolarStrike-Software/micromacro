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
]]
audioEnabled = true;