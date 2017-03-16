/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "event.h"
#include "strl.h"
#include "error.h"
#include <string.h>
#include <new>

MicroMacro::Event &MicroMacro::Event::operator=(const MicroMacro::Event &o)
{
	type = o.type;
	idata1 = o.idata1;
	idata2 = o.idata2;
	idata3 = o.idata3;
	msg = o.msg;
	pSocket = o.pSocket;

	customEventDatas.clear();
	for(unsigned int i = 0; i < customEventDatas.size(); i++)
		customEventDatas.push_back(o.customEventDatas.at(i));

	return *this;
}
