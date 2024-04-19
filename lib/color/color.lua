HSV = class.new()
RGB = class.new()

local HSV_MAX_HUE<const> = 360.0
local RGB_MAX<const> = 255

--[[ HSV ]]
function HSV:constructor(h, s, v)
    self.h = h or 0
    self.s = s or 0
    self.v = v or 0

    self:adjust()
end

-- Adjust properties to keep them in expected ranges
function HSV:adjust()
    -- Like modulus, but can actually be faster to compute
    while (self.h >= HSV_MAX_HUE) do
        self.h = self.h - HSV_MAX_HUE
    end

    while (self.h < 0) do
        self.h = self.h + HSV_MAX_HUE
    end

    -- wrap around once > 1.0
    if (self.s > 1.0) then
        self.s = self.s - math.floor(self.s)
    elseif (self.s < 0.0) then
        self.s = math.ceil(math.abs(self.s)) - math.abs(self.s)
    end

    if (self.v > 1.0) then
        self.v = self.v - math.floor(self.v)
    elseif (self.v < 0.0) then
        self.v = math.ceil(math.abs(self.v)) - math.abs(self.v)
    end
end

function HSV:toRgb()
    self:adjust()
    local c = self.v * self.s
    local x = c * (1 - math.abs((self.h / 60) % 2 - 1))
    local m = self.v - c
    local r, g, b = 0, 0, 0

    if self.h >= 0 and self.h < 60 then
        r, g, b = c, x, 0
    elseif self.h >= 60 and self.h < 120 then
        r, g, b = x, c, 0
    elseif self.h >= 120 and self.h < 180 then
        r, g, b = 0, c, x
    elseif self.h >= 180 and self.h < 240 then
        r, g, b = 0, x, c
    elseif self.h >= 240 and self.h < 300 then
        r, g, b = x, 0, c
    else
        r, g, b = c, 0, x
    end

    return RGB((r + m) * RGB_MAX, (g + m) * RGB_MAX, (b + m) * RGB_MAX)
end

local meta = getmetatable(HSV)
function meta:__tostring()
    self:adjust()
    return sprintf("HSV(%.10g, %.10g, %.10g)", self.h or 0, self.s or 0, self.v or 0)
end

--[[ RGB ]]
function RGB:constructor(r, g, b)
    self.r = r or 0
    self.g = g or 0
    self.b = b or 0

    self:adjust()
end

-- Adjust properties to keep them in expected ranges
function RGB:adjust()
    while self.r < 0 do
        self.r = self.r + RGB_MAX
    end
    while self.r > RGB_MAX do
        self.r = self.r - RGB_MAX
    end

    while self.g < 0 do
        self.g = self.g + RGB_MAX
    end
    while self.g > RGB_MAX do
        self.g = self.g - RGB_MAX
    end

    while self.b < 0 do
        self.b = self.b + RGB_MAX
    end
    while self.b > RGB_MAX do
        self.b = self.b - RGB_MAX
    end
end

function RGB:toHsv()
    self:adjust()
    local r, g, b = self.r / RGB_MAX, self.g / RGB_MAX, self.b / RGB_MAX
    local maxValue = math.max(r, g, b)
    local minValue = math.min(r, g, b)
    local delta = maxValue - minValue

    local h, s, v

    -- hue
    if delta == 0 then
        h = 0
    elseif maxValue == r then
        h = 60.0 * (((g - b) / delta) % 6)
    elseif maxValue == g then
        h = 60.0 * (((b - r) / delta) + 2)
    else
        h = 60.0 * (((r - g) / delta) + 4)
    end

    -- saturation
    if maxValue == 0 then
        s = 0
    else
        s = delta / maxValue
    end

    -- value
    v = maxValue

    return HSV(h, s, v)
end

local meta = getmetatable(RGB)
function meta:__tostring()
    self:adjust()
    return sprintf("RGB(%g, %g, %g)", math.round(self.r or 0), math.round(self.g or 0), math.round(self.b or 0))
end
