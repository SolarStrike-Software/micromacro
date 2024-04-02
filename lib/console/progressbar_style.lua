ConsoleProgressBarStyle = class.new()

function ConsoleProgressBarStyle:constructor()
    self.showPercent = true
    self.showRaw = true
    self.showBar = true

    self.startFmt = '['
    self.endFmt = ']'
    self.emptyChar = '#'
    self.fullChar = '#'

    self.filledBarFmt = "\x1b[38;5;82m%s\x1b[0m"
    self.unfilledBarFmt = "\x1b[38;5;235m%s\x1b[0m"

    self.minRedrawTime = 0.5
end

--[[
    Get the styling for the full block at a specific position (index from the left side of the bar).
    `filledWidth` is the width, in characters, of the filled section of the bar
    `totalWidth` is the total width (filled & unfilled) of the progress bar in characters.
    `step` is the current step being handled; a number
    `minStep` is the minimal step; the beginning of the work to be processed. Typically 0
    `maxStep` is the maximal step; the end of the work to be processed

    You may override this function to provide per-character styling.
    See RainbowConsoleProgressBarStyle:getFilledStyle() for an example
]]
function ConsoleProgressBarStyle:getFilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    return self.filledBarFmt
end

--[[
    Get the styling for the empty block at a specific position (index from the left side of the bar).
    `filledWidth` is the width, in characters, of the filled section of the bar
    `totalWidth` is the total width (filled & unfilled) of the progress bar in characters.
    `step` is the current step being handled; a number
    `minStep` is the minimal step; the beginning of the work to be processed. Typically 0
    `maxStep` is the maximal step; the end of the work to be processed

    You may override this function to provide per-character styling.
]]
function ConsoleProgressBarStyle:getUnfilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    return self.unfilledBarFmt
end

--[[
    Get a character to use for a filled block of the progress bar.
    Defaults to the fullChar set on the style, but can be overridden to get more control
]]
function ConsoleProgressBarStyle:getFilledChar(position, filledWidth, totalWidth, step, minStep, maxStep)
    return self.fullChar
end

--[[
    Get a character to use for a unfilled block of the progress bar.
    Defaults to the emptyChar set on the style, but can be overridden to get more control
]]
function ConsoleProgressBarStyle:getUnfilledChar(position, filledWidth, totalWidth, step, minStep, maxStep)
    return self.emptyChar
end

DefaultConsoleProgressBarStyle = ConsoleProgressBarStyle()

--[[ Minimalist style ]]
MinimalistConsoleProgressBarStyle = ConsoleProgressBarStyle()
MinimalistConsoleProgressBarStyle.showPercent = false
MinimalistConsoleProgressBarStyle.showRaw = false
MinimalistConsoleProgressBarStyle.showBar = true
MinimalistConsoleProgressBarStyle.fullChar = 'x'
MinimalistConsoleProgressBarStyle.emptyChar = ' '
MinimalistConsoleProgressBarStyle.filledBarFmt = "\x1b[38;5;27m%s\x1b[0m"
MinimalistConsoleProgressBarStyle.unfilledBarFmt = "\x1b[38;5;1m%s\x1b[0m"


--[[ Solid green style ]]
SolidGreenConsoleProgressBarStyle = ConsoleProgressBarStyle()
SolidGreenConsoleProgressBarStyle.fullChar = ' '
SolidGreenConsoleProgressBarStyle.emptyChar = ' '
SolidGreenConsoleProgressBarStyle.filledBarFmt = "\x1b[38;5;41;48;5;46m%s\x1b[0m"
SolidGreenConsoleProgressBarStyle.unfilledBarFmt = "\x1b[38;5;1m%s\x1b[0m"


--[[ Stoplight style ]]
StoplightConsoleProgressBarStyle = ConsoleProgressBarStyle()
StoplightConsoleProgressBarStyle.fullChar = ' '
StoplightConsoleProgressBarStyle.emptyChar = ' '
StoplightConsoleProgressBarStyle.unfilledBarFmt = "\x1b[38;5;1m%s\x1b[0m"

function StoplightConsoleProgressBarStyle:getFilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    -- Uncomplete; yellow light
    if step < maxStep then
        return "\x1b[38;5;1;48;5;227m%s\x1b[0m"
    end

    return "\x1b[38;5;1;48;5;46m%s\x1b[0m"
end

--[[ Rainbow style ]]
RainbowConsoleProgressBarStyle = ConsoleProgressBarStyle()
RainbowConsoleProgressBarStyle.fullChar = ' '
RainbowConsoleProgressBarStyle.emptyChar = ' '

function RainbowConsoleProgressBarStyle:getFilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    local ratio<const> = position / totalWidth
    local colors<const> = {160, 166, 172, 178, 184, 190, 154, 118, 82, 84, 85, 48, 42, 36, 30, 24, 27, 25, 67, 62, 57}
    local index<const> = math.floor(ratio * #colors) + 1
    local color<const> = colors[math.min(index, #colors)]

    return "\x1b[38;5;233;48;5;" .. sprintf("%d", color) .. "m%s\x1b[0m"
end

--[[ Ocean Wave style ]]
OceanWaveConsoleProgressBarStyle = ConsoleProgressBarStyle()
OceanWaveConsoleProgressBarStyle.filledChar = '#'
OceanWaveConsoleProgressBarStyle.emptyChar = ' '
OceanWaveConsoleProgressBarStyle.unfilledBarFmt = "\x1b[38;5;1;48;5;234m%s\x1b[0m"
OceanWaveConsoleProgressBarStyle.minRedrawTime = 0.1

function OceanWaveConsoleProgressBarStyle:getFilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    local i64DiminishFactor<const> = 16776960 -- time.getNow() returns really big numbers; we need to scale it down a lot!
    local timeOffset<const> = math.round(math.sin(time.getNow() / i64DiminishFactor) * 10)
    local colors <const> = {21, 25, 26, 27, 31, 32, 33, 37, 38, 39}
    local color <const> = (timeOffset + position) % (#colors - 1) + 1

    return "\x1b[38;5;" .. sprintf("%d", colors[color]) .. ";48;5;234m%s\x1b[0m"
end

