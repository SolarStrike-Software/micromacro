#include "argv.h"

#include "strl.h"
#include <string.h>
#include <stdlib.h>

Argv::Argv()
{
}

Argv::Argv(char *cmdLine)
{
    parse(cmdLine);
}

Argv::~Argv()
{
    flush();
}

void Argv::add(char *value)
{
    unsigned int len = strlen(value);
    cmdArgs.push_back(new char[len + 1]);
    strlcpy(cmdArgs.back(), value, len);
}

void Argv::parse(char *cmdLine)
{
    parseCommandLine(cmdLine, cmdArgs);
}

unsigned int Argv::size()
{
    return cmdArgs.size();
}

void Argv::flush()
{
    for(unsigned int i = 0; i < cmdArgs.size(); i++)
        delete [] cmdArgs.at(i);

    std::vector<char *> emptyVec;
    cmdArgs.swap(emptyVec);
}

const char *Argv::operator[](unsigned int index)
{
    return cmdArgs.at(index);
}
