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

DefaultConsoleProgressBarStyle = ConsoleProgressBarStyle();

--[[ Helpers ]]

local function hsv2rgb(h, s, v)
    local hh = h
    while (hh >= 360.0) do
        hh = hh - 360.0
    end

    hh = hh / 60.0 -- Convert from degrees

    local ff = hh - math.floor(hh)
    local p = v * (1.0 - s)
    local q = v * (1.0 - (s * ff))
    local t = v * (1.0 - (s * (1.0 - ff)))

    hh = math.floor(hh)
    if hh == 0 then
        return v, t, p
    elseif hh == 1 then
        return q, v, p
    elseif hh == 2 then
        return p, v, t
    elseif hh == 3 then
        return p, q, v
    elseif hh == 4 then
        return t, p, v
    else
        return v, p, q
    end
end

--[[ Safe Mode style ]]
SafeModeConsoleProgressBarStyle = class.new(ConsoleProgressBarStyle);
function SafeModeConsoleProgressBarStyle:constructor()
    SafeModeConsoleProgressBarStyle.parent.constructor(self)

    self.fullChar = '='
    self.emptyChar = ' '
    self.filledBarFmt = "%s"
    self.unfilledBarFmt = "%s"
end

--[[ Solid style ]]
SolidConsoleProgressBarStyle = class.new(ConsoleProgressBarStyle)
function SolidConsoleProgressBarStyle:constructor(r, g, b)
    SolidConsoleProgressBarStyle.parent.constructor(self)

    r = math.floor(r or 64)
    g = math.floor(g or 255)
    b = math.floor(b or 64)

    self.fullChar = ' '
    self.emptyChar = ' '
    self.filledBarFmt =
        "\x1b[38;5;41;48;2;" .. sprintf("%d", r) .. ";" .. sprintf("%d", g) .. ";" .. sprintf("%d", b) .. "m%s\x1b[0m"
    self.unfilledBarFmt = "\x1b[38;5;1m%s\x1b[0m"
end

--[[ Stoplight style ]]
StoplightConsoleProgressBarStyle = class.new(ConsoleProgressBarStyle)
function StoplightConsoleProgressBarStyle:constructor()
    StoplightConsoleProgressBarStyle.parent.constructor(self)

    self.fullChar = ' '
    self.emptyChar = ' '
    self.unfilledBarFmt = "\x1b[38;5;1m%s\x1b[0m"
end

function StoplightConsoleProgressBarStyle:getFilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    -- Uncomplete; yellow light
    if step < maxStep then
        return "\x1b[38;5;1;48;5;227m%s\x1b[0m"
    end

    return "\x1b[38;5;1;48;5;46m%s\x1b[0m"
end

--[[ Rainbow style ]]
RainbowConsoleProgressBarStyle = class.new(ConsoleProgressBarStyle)
function RainbowConsoleProgressBarStyle:constructor(saturation)
    RainbowConsoleProgressBarStyle.parent.constructor(self)

    self.fullChar = ' '
    self.emptyChar = ' '
    self.saturation = saturation or 1.0
end

function RainbowConsoleProgressBarStyle:getFilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    local h<const> = (position / totalWidth) * 360
    local r, g, b = hsv2rgb(h, self.saturation, 1.0)
    r = math.round(r * 255)
    g = math.round(g * 255)
    b = math.round(b * 255)

    return "\x1b[48;2;" .. sprintf("%d", r) .. ";" .. sprintf("%d", g) .. ";" .. sprintf("%d", b) .. "m%s\x1b[0m"
end

--[[ Ocean Wave style ]]
OceanWaveConsoleProgressBarStyle = class.new(ConsoleProgressBarStyle)
function OceanWaveConsoleProgressBarStyle:constructor()
    OceanWaveConsoleProgressBarStyle.parent.constructor(self)

    self.fullChar = '~'
    self.emptyChar = ' '
    self.unfilledBarFmt = "\x1b[38;5;1;48;5;234m%s\x1b[0m"
    self.minRedrawTime = 0.1
end

function OceanWaveConsoleProgressBarStyle:getFilledStyle(position, filledWidth, totalWidth, step, minStep, maxStep)
    local i64DiminishFactor<const> = 16776960 -- time.getNow() returns really big numbers; we need to scale it down a lot!
    local timeOffset<const> = math.round(math.sin(time.getNow() / i64DiminishFactor) * 10)
    local colors<const> = {21, 25, 26, 27, 31, 32, 33, 37, 38, 39}
    local color<const> = math.round(timeOffset + position) % (#colors - 1) + 1

    return "\x1b[38;5;" .. sprintf("%d", colors[color]) .. ";48;5;234m%s\x1b[0m"
end

