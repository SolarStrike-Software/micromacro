/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "types.h"
#include <math.h>
#include <stdio.h>

BatchJob &BatchJob::operator=(const BatchJob &o)
{
	this->count = o.count;
	this->type = o.type;
	return *this;
}

std::string getChunkString(MemoryChunk *pChunk, unsigned int offset, unsigned int length, int &err)
{
	err = 0;
	if( (offset+length) > pChunk->size )
	{
		err = -1;
		return "";
	}

	return std::string(pChunk->data+offset, length);
}

Vector3d &Vector3d::operator=(const Vector3d &o)
{
	x = o.x;
	y = o.y;
	z = o.z;
	return *this;
}

Vector3d Vector3d::operator+(const Vector3d &o)
{
	return Vector3d(x+o.x, y+o.y, z+o.z);
}

Vector3d Vector3d::operator-(const Vector3d &o)
{
	return Vector3d(x-o.x, y-o.y, z-o.z);
}

Vector3d Vector3d::operator*(const Vector3d &o)
{
	return Vector3d(x*o.x, y*o.y, z*o.z);
}

Vector3d Vector3d::operator*(double s)
{
	return Vector3d(x*s, y*s, z*s);
}

Vector3d Vector3d::operator/(const Vector3d &o)
{
	return Vector3d(x/o.x, y/o.y, z/o.z);
}

Vector3d Vector3d::operator/(double s)
{
	return Vector3d(x/s, y/s, z/s);
}

Vector3d Vector3d::cross(const Vector3d &o)
{
	Vector3d result;
	result.x = y*o.z - z*o.y;
	result.y = z*o.x - x*o.z;
	result.z = x*o.y - y*o.x;
	return result;
}

double Vector3d::dot(const Vector3d &o)
{
	return x*o.x + y*o.y + z*o.z;
}

Vector3d Vector3d::normal()
{
	double scale = sqrtf(x*x + y*y + z*z);
	return Vector3d(x / scale, y / scale, z / scale);
}

double Vector3d::magnitude()
{
	return sqrt(x*x + y*y + z*z);
}



// Create from 3 Euler angles
Quaternion::Quaternion(Vector3d &vec)
{
	double c1 = cos(vec.z * 0.5);
	double c2 = cos(vec.y * 0.5);
	double c3 = cos(vec.x * 0.5);
	double s1 = sin(vec.z * 0.5);
	double s2 = sin(vec.y * 0.5);
	double s3 = sin(vec.x * 0.5);

	w = c1*c2*c3 + s1*s2*s3;
	x = c1*c2*s3 - s1*s2*c3;
	y = c1*s2*c3 + s1*c2*s3;
	z = s1*c2*c3 - c1*s2*s3;
}

Quaternion::Quaternion(Vector3d &vec, double aRad)
{
	double halfa = aRad/2;
	double sinhalfa = sin(halfa);
	w = cos(halfa);
	x = vec.x * sinhalfa;
	y = vec.y * sinhalfa;
	z = vec.z * sinhalfa;
}

Quaternion &Quaternion::operator=(const Quaternion &o)
{
	w = o.w;
	x = o.x;
	y = o.y;
	z = o.z;
	return *this;
}

Quaternion Quaternion::operator*(double a)
{
	return Quaternion(w*z, x*a, y*a, z*a);
}

Quaternion Quaternion::operator*(const Quaternion &q)
{
	return Quaternion(
		w*q.w - x*q.x - y*q.y - z*q.z,
		w*q.x + x*q.w + y*q.z - z*q.y,
		w*q.y + y*q.w + z*q.x - x*q.z,
		w*q.z + z*q.w + x*q.y - y*q.x
	);
}

Vector3d Quaternion::operator*(const Vector3d &V)
{
	Vector3d v(x, y, z);
	Vector3d vcV = v.cross(V);
	Vector3d result = Vector3d(V.x, V.y, V.z) + vcV*(2*w) + v.cross(vcV)*2;

	return result;
}

Quaternion Quaternion::conjugate()
{
	return Quaternion(w, -x, -y, -z);
}

Quaternion Quaternion::normal()
{
	double len = x*x + y*y + z*z + w*w;
	return Quaternion(w/len, x/len, y/len, z/len);
}
