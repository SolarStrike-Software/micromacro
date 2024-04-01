require 'console/output'
ConsoleProgressBarStyle = class.new()
ConsoleProgressBar = class.new()

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
end

--[[
    Get the styling for the full block at a specific position (index from the left side of the bar).
    `width` will also be passed, and is the total width of the progress bar in characters.

    You may override this function to provide per-character styling.
    See RainbowConsoleProgressBarStyle:getFilledStyle() for an example
]]
function ConsoleProgressBarStyle:getFilledStyle(position, width)
    return self.filledBarFmt
end

SolidGreenConsoleProgressBarStyle = ConsoleProgressBarStyle()
SolidGreenConsoleProgressBarStyle.fullChar = ' '
SolidGreenConsoleProgressBarStyle.emptyChar = ' '
SolidGreenConsoleProgressBarStyle.filledBarFmt = "\x1b[38;5;41;48;5;46m%s\x1b[0m"
SolidGreenConsoleProgressBarStyle.unfilledBarFmt = "\x1b[38;5;1m%s\x1b[0m"

RainbowConsoleProgressBarStyle = ConsoleProgressBarStyle()
RainbowConsoleProgressBarStyle.fullChar = ' '
RainbowConsoleProgressBarStyle.emptyChar = ' '
RainbowConsoleProgressBarStyle.filledBarFmt = "\x1b[38;5;1;48;5;1m%s\x1b[0m"
RainbowConsoleProgressBarStyle.unfilledBarFmt = "\x1b[38;5;1;48;5;234m%s\x1b[0m"

function RainbowConsoleProgressBarStyle:getFilledStyle(position, width)
    local ratio = position / width
    local colors = {160, 166, 172, 178, 184, 190, 154, 118, 82, 84, 85, 48, 42, 36, 30, 24, 27, 25, 67, 62, 57}
    local index = math.floor(ratio * #colors) + 1
    local color = colors[math.min(index, #colors)]

    return "\x1b[38;5;233;48;5;" .. sprintf("%d", color) .. "m%s\x1b[0m"
end

local function saveCursorPosition()
    io.write('\x1b[s')
end

local function restoreCursorPosition()
    io.write('\x1b[u')
end

function ConsoleProgressBar:constructor(min, max, width, style)
    self.min = min or 0
    self.max = max or 100
    self.current = self.min
    self.width = width or 20
    self.style = style or ConsoleProgressBarStyle();

    self.minRedrawTime = 0.5
    self.lastDrawTime = time.getNow()
    self.lastDrawRatio = 0
    self.began = false
    self.dirty = false
end

getmetatable(ConsoleProgressBar).__gc = function()
    self:finish()
end

function ConsoleProgressBar:begin()
    saveCursorPosition()
    self.began = true
end

function ConsoleProgressBar:getRatio()
    return (math.min(self.current, self.max) - self.min) / (self.max - self.min)
end

function ConsoleProgressBar:advance(amount)
    if (not self.began) then
        self:begin()
    end

    self.current = self.current + (amount or 1)
    local ratio = self:getRatio()

    if ((ratio - self.lastDrawRatio) > 0.01) then
        self.dirty = true
    end

    self:update()
end

function ConsoleProgressBar:finish()
    self:draw()
    io.write('\n')
    self.began = false
end

function ConsoleProgressBar:draw()
    restoreCursorPosition()
    local ratio = self:getRatio()

    local result = ''
    local percentStr = ''
    local rawStr = ''
    local barStr = ''

    if (self.style.showRaw) then
        local maxLen = string.len(math.ceil(self.max))
        local totalLen = maxLen * 2 + 1
        rawStr = sprintf('%-' .. totalLen .. 's', sprintf('%d/%d', self.current, self.max))
        result = result .. rawStr .. ' '
    end

    if (self.style.showBar) then
        local filledBlocks = math.round(ratio * self.width)
        local unfilledBlocks = self.width - filledBlocks

        local filledBlocksStr = ''
        for i = 1, filledBlocks do
            filledBlocksStr = filledBlocksStr .. sprintf(self.style:getFilledStyle(i, self.width), self.style.fullChar)
        end

        barStr = self.style.startFmt .. filledBlocksStr ..
                     sprintf(self.style.unfilledBarFmt, string.rep(self.style.emptyChar, unfilledBlocks)) ..
                     self.style.endFmt

        result = result .. barStr .. ' '
    end

    if (self.style.showPercent) then
        percentStr = sprintf('%3d%%', math.round(ratio * 100))
        result = result .. percentStr .. ' '
    end

    io.write(result)

    self.lastDrawTime = time.getNow()
    self.lastDrawRatio = ratio
end

function ConsoleProgressBar:update()
    if (not self.dirty and time.diff(self.lastDrawTime) < self.minRedrawTime) then
        return
    end

    self:draw()
end
