#include "vis.h"
#include "render.h"
#include "resource.h"
#include "core/core.h"
#include <algorithm>

/*
 * An octree is used to store the world polygons, then walked to
 *   determine the currently visible set.
 * The map can specify the leaf capacity, which will get a leaf
 *   split when exceeded, and maximum leaf depth, beyond which
 *   no leaf will ever be split.
 */

int oct_leaf_capacity = 0;
int oct_max_depth = 0;

unsigned int occ_shader_prog;
unsigned int occ_fbo;
unsigned int occ_fbo_texture;
unsigned int occ_query;

constexpr int occ_fbo_size = 256;

/*
 * The ID of the octant in which point is
 * if the midpoint of the bbox is origin
 */
uint8_t which_octant (vec3 origin, vec3 point)
{
	return (point.x > origin.x)
	    + ((point.y > origin.y) << 1)
	    + ((point.z > origin.z) << 2);
}

/*
 * The bbox of an octant with given ID, given the parent
 */
t_bound_box octant_bound (t_bound_box parent, uint8_t octant_id)
{
	vec3 mid = (parent.start + parent.end) * 0.5;
	t_bound_box r = parent;
	(octant_id & 1 ? r.start : r.end).x = mid.x;
	(octant_id & 2 ? r.start : r.end).y = mid.y;
	(octant_id & 4 ? r.start : r.end).z = mid.z;
	return r;
}

struct t_occlusion_plane
{
	std::vector<vec3> points;
};
unsigned int occ_planes_display_list;


struct t_world_triangle
{
	t_vertex v[3];
	t_material* mat;
};
std::vector<t_world_triangle> world_tris;
t_bound_box world_bounds;

struct oct_node
{
	/* Triangles within this node with the same material */
	struct mat_group {
		t_material* mat;
		unsigned int display_list;
	};

	union {
		/* First vertex of triangle in world_tris, eg 0, 3, 6 */
		std::vector<int> bucket;
		std::vector<mat_group> material_buckets;
	};

	bool leaf;
	oct_node* children[8];
	t_bound_box actual_bounds;

	void build (t_bound_box bounds, int level);
	void make_leaf ();

	void render_tris () const;

	oct_node ();
	~oct_node ();
};
oct_node root;


void oct_node::build (t_bound_box bounds, int level)
{
	// build the bounds
	actual_bounds = bounds;
	for (const int& d: bucket) {
		for (int i = 0; i < 3; i++) {
			vec3& v = world_tris[d].v[i].pos;
			actual_bounds.update(v);
		}
	}

	if (level > oct_max_depth || bucket.size() <= oct_leaf_capacity) {
		make_leaf();
		return;
	}

	leaf = false;
	for (int i = 0; i < 8; i++)
		children[i] = new oct_node;

	vec3 bb_mid = (bounds.start + bounds.end) * 0.5;

	for (int d: bucket) {
		vec3 tri_mid(0.0);
		for (int i = 0; i < 3; i++)
			tri_mid += world_tris[d].v[i].pos;
		tri_mid /= 3.0;
		children[which_octant(bb_mid, tri_mid)]->bucket.push_back(d);
	}

	bucket.clear();

	for (int i = 0; i < 8; i++)
		children[i]->build(octant_bound(bounds, i), level + 1);
}

void oct_node::make_leaf ()
{
	std::map<t_material*, std::vector<t_vertex>> m;

	for (const int& d: bucket) {
		for (int i = 0; i < 3; i++)
			m[world_tris[d].mat].push_back(
					world_tris[d].v[i]);
	}

	// the bucket is in union with material buckets,
	// so we have to destroy and construct them manually
	bucket.~vector();
	material_buckets = std::vector<mat_group>();
	material_buckets.reserve(m.size());

	for (std::pair<t_material* const, std::vector<t_vertex>>& p: m) {
		t_material* mat = p.first;
		std::vector<t_vertex>& vertices = p.second;

		unsigned int dlist = glGenLists(1);
		glNewList(dlist, GL_COMPILE);
		glBegin(GL_TRIANGLES);
		for (const t_vertex& v: vertices) {
			glNormal3f(v.norm.x, v.norm.y, v.norm.z);
			glTexCoord2f(v.tex.u, v.tex.v);
			glVertex3f(v.pos.x, v.pos.y, v.pos.z);
		}
		glEnd();
		glEndList();

		material_buckets.push_back({ mat, dlist });
	}
}

void oct_node::render_tris () const
{
	for (const mat_group& gr: material_buckets) {
		gr.mat->apply();
		glCallList(gr.display_list);
	}
}

oct_node::oct_node ()
{
	leaf = true;
	bucket = std::vector<int>(0);
}

oct_node::~oct_node ()
{
	if (leaf)
		return;
	for (int i = 0; i < 8; i++)
		delete children[i];
}



std::vector<oct_node*> visible_leaves;
int total_visible_nodes;

void vis_render_bbox (const t_bound_box& box);
void fill_visible_sub (oct_node* node)
{
	glBeginQuery(GL_SAMPLES_PASSED, occ_query);
	vis_render_bbox(node->actual_bounds);
	glEndQuery(GL_SAMPLES_PASSED);

	unsigned int visible;
	glGetQueryObjectuiv(occ_query, GL_QUERY_RESULT, &visible);
	if (visible <= 0)
		return;

	if (node->leaf) {
		visible_leaves.push_back(node);
		return;
	}
	for (int i = 0; i < 8; i++)
		fill_visible_sub(node->children[i]);
}

void draw_visible ()
{
	// setup occlusion rendering
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, occ_fbo);
	glViewport(0, 0, occ_fbo_size, occ_fbo_size);
	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgram(occ_shader_prog);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDisable(GL_CULL_FACE);

	// fill z-buffer with data from occlusion planes
	glDepthMask(GL_TRUE);
	glCallList(occ_planes_display_list);
	glDepthMask(GL_FALSE);

	// walk the tree nodes which pass the z-test (and are on screen)
	visible_leaves.clear();
	fill_visible_sub(&root);
	total_visible_nodes = visible_leaves.size();

	// setup proper rendering
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, sdlcont.res_x, sdlcont.res_y);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	// draw world
	for (const oct_node* node: visible_leaves)
		node->render_tris();
}

void read_world_vis_data (std::string path)
{
	std::ifstream f(path);
	if (!f)
		warning("Vis data for world unavailable: %s", path.c_str());

	std::string option;
	while (f >> option) {
		if (option == "oct_depth") {
			f >> oct_max_depth;
		} else if (option == "oct_capacity") {
			f >> oct_leaf_capacity;
		} else if (option == "bounds") {
			f >> world_bounds.start >> world_bounds.end;
		} else {
			warning("Unrecognized option in vis data %s: %s",
					path.c_str(), option.c_str());
		}
	}
}


/*
 * Read world in a different way than what t_model_mem does
 * because we're better off working with faces rather than vertices,
 * and we need to handle materials specified in the obj in
 * a very special way (ie OCCLUDE means it's an occlusion plane
 *                     and goes to a different vector)
 * TODO: maybe redesign OBJ reading to reduce the blatant
 *   code duplication - the only things that are different
 *   are usemtl and face handling
 */
void read_world_obj (std::string path)
{
	std::ifstream f(path);
	if (!f)
		fatal("world obj not found: %s", path.c_str());

	std::vector<vec3> points;
	std::vector<vec3> normals;
	std::vector<t_texcrd> texcoords;

	// we will draw them into a display list right away
	std::vector<vec3> occ_plane_verts;

	auto pack =
		[] (char a, char b) constexpr -> uint16_t
		{ return ((a << 8) | b); };

	std::string cur_material;

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

			if (cur_material == "OCCLUDE") {
				// an occlusion plane
				for (int i = 0; i < 3; i++)
					occ_plane_verts.push_back(points[v[i]]);
			} else {
				// a regular plane
				t_world_triangle tri;
				for (int i = 0; i < 3; i++) {
					tri.v[i] = { points[v[i]-1],
					             normals[n[i]-1],
					             texcoords[t[i]-1] };
				}
				tri.mat = get_material(cur_material);
				world_tris.push_back(tri);
			}

			break;
		}
		case pack('u', 's'): {
			// usemtl - update current material
			char buf[line.length()];
			sscanf(line.c_str(), "%*s %s", buf);
			cur_material = std::string(buf);
		}
		default: {
			// something in the format we are unaware of
			continue;
		}
		}
	}

	occ_planes_display_list = glGenLists(1);
	glNewList(occ_planes_display_list, GL_COMPILE);
	glBegin(GL_TRIANGLES);
	for (const vec3& v: occ_plane_verts)
		glVertex3f(v.x, v.y, v.z);
	glEnd();
	glEndList();
}

void vis_initialize_world (std::string path)
{
	read_world_obj(path + "/geo.obj");
	read_world_vis_data(path + "/vis");

	if (world_bounds.volume() <= 0.0) {
		fatal("Map %s does not specify valid world bounds",
				path.c_str());
	}

	for (int i = 0; i < world_tris.size(); i++)
		root.bucket.push_back({ i });

	root.build(world_bounds, 0);
}

bool t_bound_box::point_in (vec3 pt) const
{
	return (pt.x >= start.x) && (pt.y >= start.y) && (pt.z >= start.z)
		&& (pt.x <= end.x) && (pt.y <= end.y) && (pt.z <= end.z);
}

void vis_render_bbox (const t_bound_box& box)
{
	const vec3& s = box.start;
	const vec3& e = box.end;
	auto quad = [] (vec3 a, vec3 b, vec3 c, vec3 d)
	{
		glVertex3f(a.x, a.y, a.z);
		glVertex3f(b.x, b.y, b.z);
		glVertex3f(c.x, c.y, c.z);
		glVertex3f(d.x, d.y, d.z);
	};

	glDisable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	quad(s, { s.x, e.y, s.z }, { s.x, e.y, e.z }, { s.x, s.y, e.z });
	quad(s, { e.x, s.y, s.z }, { e.x, e.y, s.z }, { s.x, e.y, s.z });
	quad(s, { e.x, s.y, s.z }, { e.x, s.y, e.z }, { s.x, s.y, e.z });
	quad(e, { e.x, e.y, s.z }, { e.x, s.y, s.z }, { e.x, s.y, e.z });
	quad(e, { e.x, s.y, e.z }, { s.x, s.y, e.z }, { s.x, e.y, e.z });
	quad(e, { e.x, e.y, s.z }, { s.x, e.y, s.z }, { s.x, e.y, e.z });
	glEnd();
}

void init_vis ()
{
	// setup the framebuffer in which to test for occlusion
	occ_shader_prog = glCreateProgram();
	glAttachShader(occ_shader_prog,
		get_shader("int/vert_identity", GL_VERTEX_SHADER));
	glAttachShader(occ_shader_prog,
		get_shader("int/frag_null", GL_FRAGMENT_SHADER));
	glLinkProgram(occ_shader_prog);

	glGenFramebuffers(1, &occ_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, occ_fbo);

	glGenTextures(1, &occ_fbo_texture);
	glBindTexture(GL_TEXTURE_2D, occ_fbo_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			occ_fbo_size, occ_fbo_size,
			0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_2D, occ_fbo_texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// setup the query for occlusion testing
	glGenQueries(1, &occ_query);
}