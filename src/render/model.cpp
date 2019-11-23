#include "inc_general.h"
#include "model.h"
#include "render.h"
#include "input/cmds.h"
#include <cassert>

void t_model::render () const
{
	glCallList(display_list_id);
}

void t_model::load (const t_model_mem& verts)
{
	display_list_id = glGenLists(1);
	glNewList(display_list_id, GL_COMPILE);
	glBegin(GL_TRIANGLES);
	for (const t_vertex& v: verts.verts) {
		glNormal3f(v.norm.x, v.norm.y, v.norm.z);
		glTexCoord2f(v.tex.u, v.tex.v);
		glVertex3f(v.pos.x, v.pos.y, v.pos.z);
	}
	glEnd();
	glEndList();

	bbox = verts.bbox;
}

void t_model_mem::calc_bbox ()
{
	// just in case, make sure 0 is in
	bbox = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } };
	for (const t_vertex& v: verts)
		bbox.update(v.pos);
	// just in case, extend slightly
	bbox.start -= { 0.5, 0.5, 0.5 };
	bbox.end += { 0.5, 0.5, 0.5 };
}

bool t_model_mem::load_obj (std::string path)
{
	std::ifstream f(path);
	if (!f)
		return false;

	std::vector<vec3> points;
	std::vector<vec3> normals;
	std::vector<t_texcrd> texcoords;

	auto pack =
		[] (char a, char b) constexpr -> uint16_t
		{ return ((a << 8) | b); };

	for (std::string line; std::getline(f, line); ) {

		int comment = line.find('#');
		if (comment != std::string::npos)
			line.erase(comment);

		if (line.size() < 2)
			continue;

		uint16_t p = pack(line[0], line[1]);

		switch (p) {
		case pack('v', ' '): {
			// vertex
			float x, y, z;
			sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
			points.push_back({ x, y, z });
			break;
		}
		case pack('v', 'n'): {
			// vertex normal
			float x, y, z;
			sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
			normals.push_back({ x, y, z });
			break;
		}
		case pack('v', 't'): {
			// tex coord
			float u, v;
			sscanf(line.c_str(), "%*s %f %f", &u, &v);
			texcoords.push_back({ u, 1.0f - v });
			break;
		}
		case pack('f', ' '): {
			// face
			int v[3];
			int n[3];
			int t[3];
			sscanf(line.c_str(), "%*s %i/%i/%i %i/%i/%i %i/%i/%i",
					&v[0], &t[0], &n[0],
					&v[1], &t[1], &n[1],
					&v[2], &t[2], &n[2]);

			for (int i = 0; i < 3; i++) {
				verts.push_back(
					{ points[v[i]-1],
					  normals[n[i]-1],
					  texcoords[t[i]-1] });
			}

			break;
		}
		default: {
			// might be something in the format
			// we are unaware of
			continue;
		}
		}
	}

	calc_bbox();
	assert(verts.size() % 3 == 0);

	return true;
}

void t_model_mem::dump_rvd (std::string path) const
{
	std::ofstream f(path, std::ios::binary);
	if (!f)
		return;
	int32_t vertnum = verts.size();
	f.write((char*) &vertnum, sizeof(vertnum));
	f.write((char*) verts.data(), vertnum * sizeof(t_vertex));
}

bool t_model_mem::load_rvd (std::string path)
{
	std::ifstream f(path, std::ios::binary);
	verts.clear();

	if (!f)
		return false;

	int32_t vertnum = -1;

	int filesize;
	f.seekg(0, f.end);
	filesize = f.tellg();
	f.seekg(0, f.beg);

	f.read((char*) &vertnum, sizeof(vertnum));

	if (filesize != vertnum * sizeof(t_vertex) + sizeof(vertnum))
		return false;

	assert(vertnum % 3 == 0);

	verts.resize(vertnum);
	f.read((char*) verts.data(), vertnum * sizeof(t_vertex));

	calc_bbox();

	return true;
}

COMMAND_ROUTINE (obj2rvd)
{
	if (ev != PRESS)
		return;
	if (args.empty())
		return;

	const std::string& in = args[0];
	std::string out;

	if (args.size() > 1) {
		out = args[1];
	} else {
		// add .rvd at the end or instead of .obj
		out = in;
		int size = out.size();
		if (size > 4 && out.compare(size-4, 4, ".obj") == 0)
			out.erase(size-4, std::string::npos);
		out += ".rvd";
	}

	t_model_mem model;
	model.load_obj(in);
	model.dump_rvd(out);
}
