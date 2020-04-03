#include "misc.h"
#include <cstdio>
#include <cstdlib>

[[noreturn]]
void fatal (const char* format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "Fatal: ");
	if (format != nullptr)
		vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);

	// cannot use fancy facilities like must_quit
	// since we must be able to fatal() out of anywhere
	exit(EXIT_FAILURE);
}

void warning (const char* format, ...)
{
	va_list args;
	va_start(args, format);
	fprintf(stderr, "Warning: ");
	if (format != nullptr)
		vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(args);
}


std::ostream& operator<< (std::ostream& s, const vec3& v)
{
	return (s << v.x << ' ' << v.y << ' ' << v.z);
}

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

std::string vec3toa (const vec3& v)
{
	using std::to_string;
	return to_string(v.x) + ' ' + to_string(v.y) + ' ' + to_string(v.z);
}

vec3 min_components (const vec3& a, const vec3& b)
{
	using std::min;
	return vec3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z));
}

vec3 max_components (const vec3& a, const vec3& b)
{
	using std::max;
	return vec3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z));
}


static vec3 euler_dir[3] =
	{ { 1.0, 0.0, 0.0 },
	  { 0.0, 1.0, 0.0 },
	  { 0.0, 0.0, 1.0 } };

mat4 rotate_xyz (const vec3& angles)
{
	mat4 r(1.0);
	for (int i = 0; i < 3; i++)
		r = glm::rotate(r, angles[i], euler_dir[i]);
	return r;
}


bool str_starts_with (const std::string& haystack, const std::string& needle)
{
	if (needle.size() > haystack.size())
		return false;

	for (int i = 0; i < needle.size(); i++) {
		if (haystack[i] != needle[i])
			return false;
	}

	return true;
}

