require 'unittest/unittest'

local dir = filesystem.getCWD()

-- Set CWD from argument 1, and correct for relative 'scripts' dir if needed
if (type(args) == "table" and args[1] ~= nil) then
    dir = args[1];
    if (not filesystem.directoryExists(dir)) then
        dir = "scripts/" .. dir
    end
end

if (not filesystem.directoryExists(dir .. "/tests")) then
    printf("Could not tests directory. Are you sure `%s` is a valid project with unit tests?\n", dir);
    return -1
end

filesystem.setCWD(dir)

return UnitTest():run()
