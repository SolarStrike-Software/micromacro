/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include "event.h"
#include "strl.h"
#include "error.h"
#include <string.h>
#include <new>

MicroMacro::Event &MicroMacro::Event::operator=(const MicroMacro::Event &o)
{
    type = o.type;
    data.clear();
    for(unsigned int i = 0; i < data.size(); i++)
        data.push_back(o.data.at(i));

    return *this;
}
