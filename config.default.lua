--[[ Logging settings ---------------------------------------------------------
    logRemovalDays  Remove logs older than this number in days (0 = disable)
    logDirectory    Location to store log files
    logLevel        Required log level to reach before the message will be logged
]]
logRemovalDays  = 7;
logDirectory = "./logs";
logLevel = 6;


--[[ Script settings ----------------------------------------------------------
    scriptDirectory     This is the default script directory. Do not include trailing slash.
]]
scriptDirectory = "./scripts";


--[[ Memory & Process settings ------------------------------------------------
    memoryStringBufferSize  Size of the buffer (in bytes) used when handling strings in memory.
]]
memoryStringBufferSize = 128;

--[[ Network settings ---------------------------------------------------------
    networkEnabled      Whether or not to enable network functions
    networkBufferSize   Size of the buffer (in bytes) used when reading socket data.
    recvQueueSize       Maximum number of packets we will hold in each socket's receive queue
]]
networkEnabled = true;
networkBufferSize = 10240;
recvQueueSize = 100;

--[[ Error settings -----------------------------------------------------------
    styleErrors         Whether to render error messages with style
    fileStyle           The style for filenames in error messages
    lineNumberStyle     The style for line numbers in error messages
    errMessageStyle     The style for the error message itself

    All error messages are styled using ANSI format. Please see the docs
    for more information on how to appropriately use the available options:
    
    https://solarstrike.net/docs/micromacro/ansi-console
]]
styleErrors = true;
fileStyle = "\x1b[38;5;35m";
lineNumberStyle = "\x1b[38;5;44m";
errMessageStyle = "\x1b[38;5;228m";

--[[ Other things -------------------------------------------------------------
    yieldTimeSlice  Whether to yield time back to the system (prevent 100% CPU)
]]
yieldTimeSlice = true;
