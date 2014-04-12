#include "types.h"

void CMultivar::setNumber(double nv)
{
	fValue = nv;
	type = VT_NUMBER;
}

void CMultivar::setString(std::string nv)
{
	szValue = nv;
	type = VT_STRING;
}

void CMultivar::setNil()
{
	fValue = 0.0;
	szValue = "";
	type = VT_NIL;
}

double CMultivar::getNumber()
{
	return fValue;
}

std::string CMultivar::getString()
{
	return szValue;
}

Multivar_type CMultivar::getType()
{
	return type;
}

BatchJob &BatchJob::operator=(const BatchJob &o)
{
	this->count = o.count;
	this->type = o.type;
	return *this;
}
