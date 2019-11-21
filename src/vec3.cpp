#include "vec3.h"
#include <cmath>

vec3::vec3 (float xx, float yy, float zz)
{
	x = xx;
	y = yy;
	z = zz;
}

vec3::vec3 (float xx, float yy)
{
	x = xx;
	y = yy;
	z = 0.0;
}

vec3::vec3 (float c)
{
	x = c;
	y = c;
	z = c;
}

vec3::vec3 ()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;
}

vec3 operator+ (vec3 one, vec3 other)
{
	return vec3(
		one.x + other.x,
		one.y + other.y,
		one.z + other.z);
}

vec3 operator- (vec3 one, vec3 other)
{
	return vec3(
		one.x - other.x,
		one.y - other.y,
		one.z - other.z);
}

vec3 operator* (vec3 one, vec3 other)
{
	return vec3(
		one.x * other.x,
		one.y * other.y,
		one.z * other.z);
}

vec3 operator/ (vec3 one, vec3 other)
{
	return vec3(
		one.x / other.x,
		one.y / other.y,
		one.z / other.z);
}


vec3 operator* (vec3 v, float c)
{
	return vec3(c * v.x,
	            c * v.y,
	            c * v.z);
}

vec3 operator* (float c, vec3 v)
{
	return v * c;
}

vec3 operator/ (vec3 v, float c)
{
	return vec3(v.x / c,
	            v.y / c,
	            v.z / c);
}

vec3& vec3::operator+= (vec3 v)
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}

vec3& vec3::operator-= (vec3 v)
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}

vec3& vec3::operator*= (float c)
{
	x *= c;
	y *= c;
	z *= c;
	return *this;
}

vec3& vec3::operator/= (float c)
{
	x /= c;
	y /= c;
	z /= c;
	return *this;
}

vec3& vec3::norm ()
{
	float r = abs(*this);
	if (r == 0.0)
		return *this;
	r = 1.0 / r;
	x *= r;
	y *= r;
	z *= r;
	return *this;
}

float abs (const vec3& v)
{
	return sqrt(v.x * v.x
	          + v.y * v.y
	          + v.z * v.z);
}

std::ostream& operator<< (std::ostream& s, vec3 v)
{
	return (s << v.x << ' ' << v.y << ' ' << v.z);
}

#include "inc_general.h"

std::istream& operator>> (std::istream& s, vec3& v)
{
	return (s >> v.x >> v.y >> v.z);
}

vec3 atovec3 (const char* s)
{
	vec3 r;
	sscanf(s, "%f %f %f", &r.x, &r.y, &r.z);
	return r;
}

void atovec3 (const char* s, vec3& v)
{
	sscanf(s, "%f %f %f", &v.x, &v.y, &v.z);
}

vec3 atovec3 (const std::string& s)
{
	vec3 r;
	sscanf(s.c_str(), "%f %f %f", &r.x, &r.y, &r.z);
	return r;
}

void atovec3 (const std::string& s, vec3& v)
{
	sscanf(s.c_str(), "%f %f %f", &v.x, &v.y, &v.z);
}
