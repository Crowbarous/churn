#include "core/core.h"
#include "core/entity.h"
#include "input/cmds.h"
#include "input/input.h"
#include "render/render.h"
#include "render/vis.h"
#include <cassert>

bool must_quit;
int exit_code;
unsigned long long tick;

void init_core ()
{
	must_quit = false;
	exit_code = 0;
	tick = 0;

	fill_ent_registry();
}

COMMAND_ROUTINE (nop)
{
	return;
}

COMMAND_ROUTINE (exit)
{
	if (ev != PRESS)
		return;
	exit_code = 0;
	if (!args.empty())
		exit_code = std::atoi(args[0].c_str());
	must_quit = true;
}

COMMAND_ROUTINE (echo)
{
	if (ev != PRESS)
		return;

	for (const std::string& arg: args)
		std::cout << arg << " ";

	std::cout << std::endl;
}

void update ()
{
	tick++;

	for (e_base* e: ents.vec)
		e->think();

	while (!signals.empty()) {
		const t_signal& s = signals.top();
		if (s.tick_due > tick)
			break;
		s.execute();
		signals.pop();
	}

	upd_camera_pos();
}

e_base* read_single_entity (std::istream& is)
{
	std::string line;
	std::getline(is, line);

	if (line.empty() || isspace(line[0]))
		fatal("Invalid entity syntax: starts with whitespace");

	while (isspace(line.back()))
		line.pop_back();

	e_base* ent = ents.spawn(line);
	if (ent == nullptr)
		fatal("Unable to spawn entity \"%s\"", line.c_str());

	t_ent_keyvals kv;
	int kstart, kend;

	while (std::getline(is, line)) {
		while (!line.empty() && isspace(line.back()))
			line.pop_back();
		if (line.empty())
			goto out_loop;

		if (!isspace(line[0])) {
			fatal("Invalid entity syntax: "
				"keyval starts with nonspace");
		}

		kstart = 0;
		while (isspace(line[kstart]))
			kstart++;
		kend = kstart + 1;
		while (kend < line.length() && !isspace(line[kend]))
			kend++;

		if (line[kstart] == '!') {
			// line describes an event
			std::istringstream iss(line.c_str() + kstart + 1);

			std::string event_name;
			t_signal s;
			iss >> event_name >> s.tick_due
				>> s.target >> s.signal_name;
			// get the whole rest of the line
			getline(iss, s.argument);
			ent->events[event_name].push_back(s);

		} else {
			// line describes a key-value pair
			kv.add(line.substr(kstart, kend-kstart),
					line.substr(kend + 1));
		}

		out_loop:
		int nextchar = is.peek();
		if (nextchar == EOF || !isspace(nextchar))
			break;
	}
	ent->apply_keyvals(kv);
	ent->moved();

	return ent;
}

void load_map (std::string path)
{
	vis_destroy_world();
	vis_initialize_world(path);

	std::ifstream f(path + "/ents");
	if (!f)
		fatal("Cannot load map %s: no ents file", path.c_str());
	while (!f.eof())
		read_single_entity(f);
}

COMMAND_ROUTINE (loadmap)
{
	if (ev != PRESS)
		return;
	if (args.empty())
		return;

	std::string path = "res/maps/" + args[0];
	load_map(path);
}


float t_bound_box::volume () const
{
	return (end.x - start.x)
	     * (end.y - start.y)
	     * (end.z - start.z);
}

void t_bound_box::expand (const vec3& pt)
{
	start = min_components(start, pt);
	end = max_components(end, pt);
}

void t_bound_box::expand (const t_bound_box& other)
{
	start = min_components(start, other.start);
	end = max_components(end, other.end);
}

bool t_bound_box::point_in (const vec3& pt) const
{
	return (pt.x >= start.x) && (pt.y >= start.y) && (pt.z >= start.z)
		&& (pt.x <= end.x) && (pt.y <= end.y) && (pt.z <= end.z);
}

bool t_bound_box::point_in (const vec3& pt, float tolerance) const
{
	return (pt.x >= start.x - tolerance) && (pt.y >= start.y - tolerance)
	    && (pt.z >= start.z - tolerance) && (pt.x <= end.x + tolerance)
	    && (pt.y <= end.y + tolerance) && (pt.z <= end.z + tolerance);
}

bool t_bound_box::intersects (const t_bound_box& b) const
{
	t_bound_box tmp = { max_components(start, b.start),
	                    min_components(end, b.end) };
	return tmp.end.x >= tmp.start.x
	    && tmp.end.y >= tmp.start.y
	    && tmp.end.z >= tmp.start.z;
}

void t_bound_box::intersect (const t_bound_box& b)
{
	start = max_components(start, b.start);
	end = min_components(end, b.end);
}

void t_bound_box::intersect_guarded (const t_bound_box& b)
{
	start = max_components(start, b.start);
	end = min_components(end, b.end);
	for (int i = 0; i < 3; i++)
		end[i] = std::max(end[i], start[i]);
}
