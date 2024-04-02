ConsoleOutput = class.new()


function ConsoleOutput:constructor()
    self.useAnsi = true
    self.ansiAvailable = self:isAnsiAvailable()

    self.styles = {
        ['default'] ='%s',
        ['petty'] = "\x1b[90m%s\x1b[0m",
        ['info'] = "\x1b[96m%s\x1b[0m",
        ['comment'] = "\x1b[33m%s\x1b[0m",
        ['success'] = "\x1b[32m%s\x1b[0m",
        ['fail'] = "\x1b[31m%s\x1b[0m",
        ['warning'] = "\x1b[30;43m%s\x1b[0;0m",
        ['error'] = "\x1b[1;97;41m%s\x1b[0;0m",
    }
end

function ConsoleOutput:setStyle(style, fmt)
    self.styles[style] = fmt
end

function ConsoleOutput:isAnsiAvailable()
    if( os.getenv('NO_COLOR') ) then
        return false
    end

    if( os.getenv('TERM_PROGRAM') == 'Hyper' ) then
        return true
    end

    if( io.stdout == nil ) then
        return false
    end

    return true;
end

function ConsoleOutput:write(msg)
    io.write(msg)
end

function ConsoleOutput:writeln(msg)
    self:write(msg or '')
    self:write("\n")
end

function ConsoleOutput:sstyle(style, msg)
    if( not(self.useAnsi and self.ansiAvailable) or self.styles[style] == nil ) then
        return msg
    end

    return sprintf(self.styles[style], msg)
end

function ConsoleOutput:style(style, msg)
    self:write(self:sstyle(style, msg))
end

function ConsoleOutput:default(msg)
    self:style('default', msg)
    self:writeln()
end

function ConsoleOutput:petty(msg)
    self:style('petty', msg)
    self:writeln()
end

function ConsoleOutput:info(msg)
    self:style('info', msg)
    self:writeln()
end

function ConsoleOutput:comment(msg)
    self:style('comment', msg)
    self:writeln()
end

function ConsoleOutput:success(msg)
    self:style('success', msg)
    self:writeln()
end

function ConsoleOutput:fail(msg)
    self:style('fail', msg)
    self:writeln()
end

function ConsoleOutput:warning(msg)
    self:style('warning', msg)
    self:writeln()
end

function ConsoleOutput:error(msg)
    self:style('error', msg)
    self:writeln()
end
