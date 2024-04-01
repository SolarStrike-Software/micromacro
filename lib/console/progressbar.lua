require 'console/output'
ConsoleProgressBar = class.new()

local output = ConsoleOutput()
output:setStyle('filledbar', "\x1b[38;5;82m%s\x1b[0m")
output:setStyle('unfilledbar', "\x1b[38;5;234m%s\x1b[0m")

local function saveCursorPosition()
    io.write('\x1b[s')
end

local function restoreCursorPosition()
    io.write('\x1b[u')
end

function ConsoleProgressBar:constructor(min, max, width)
    self.min = min or 0
    self.max = max or 100
    self.current = self.min
    self.width = width or 20

    self.showPercent = true
    self.showRaw = true
    self.showBar = true

    self.startFmt = '['
    self.endFmt = ']'
    self.emptyChar = '█'
    self.fullChar = '█'

    self.minRedrawTime = 0.5
    self.lastDrawTime = time.getNow()
    self.lastDrawRatio = 0
    self.began = false
    self.dirty = false
end

getmetatable(ConsoleProgressBar).__gc = function ()
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
    if( not self.began ) then
        self:begin()
    end

    self.current = self.current + (amount or 1)
    local ratio = self:getRatio()

    if( (ratio - self.lastDrawRatio) > 0.01 ) then
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

    if( self.showRaw ) then
        local maxLen = string.len(math.ceil(self.max))
        local totalLen = maxLen*2 + 1
        rawStr = sprintf('%-' .. totalLen .. 's', sprintf('%d/%d', self.current, self.max))
        result = result .. rawStr .. ' '
    end

    if( self.showBar ) then
        local filledBlocks = math.round(ratio*self.width)
        barStr = self.startFmt
            .. output:sstyle('filledbar', string.rep(self.fullChar, filledBlocks))
            .. output:sstyle('unfilledbar', string.rep(self.emptyChar, self.width - filledBlocks))
            .. self.endFmt

        result = result .. barStr .. ' '
    end

    if( self.showPercent ) then
        percentStr = sprintf('%3d%%', math.round(ratio * 100))
        result = result .. percentStr .. ' '
    end

    io.write(result)

    self.lastDrawTime = time.getNow()
    self.lastDrawRatio = ratio
end

function ConsoleProgressBar:update()
    if( not self.dirty and time.diff(self.lastDrawTime) < self.minRedrawTime ) then
        return
    end

    self:draw()
end
