require 'console/output'
require 'console/progressbar_style'

ConsoleProgressBar = class.new()

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

    if (self.style.showRaw) then
        local maxLen = string.len(math.ceil(self.max))
        local totalLen = maxLen * 2 + 1
        rawStr = sprintf('%-' .. totalLen .. 's', sprintf('%d/%d', self.current, self.max))
        result = result .. rawStr .. ' '
    end

    if (self.style.showBar) then
        local filledWidth = math.round(ratio * self.width)
        local unfilledWidth = self.width - filledWidth

        local filledBlocksStr = ''
        for i = 1, filledWidth do
            local char = self.style:getFilledChar(i, filledWidth, self.width)
            filledBlocksStr = filledBlocksStr .. sprintf(self.style:getFilledStyle(i, filledWidth, self.width, self.current, self.min, self.max), char)
        end

        local unfilledBlocksStr = ''
        for i = 1, unfilledWidth do
            local char = self.style:getUnfilledChar(i, filledWidth, self.width)
            unfilledBlocksStr = unfilledBlocksStr .. sprintf(self.style:getUnfilledStyle(i, filledWidth, self.width, self.current, self.min, self.max), char)
        end

        result = result .. self.style.startFmt .. filledBlocksStr .. unfilledBlocksStr .. self.style.endFmt .. ' '
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
    if (not self.dirty and time.diff(self.lastDrawTime) < (self.style.minRedrawTime or 0.5)) then
        return
    end

    self:draw()
end
