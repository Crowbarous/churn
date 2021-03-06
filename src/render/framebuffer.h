#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "inc_gl.h"
#include <cassert>
#include <array>
#include <vector>

/*
 * Helper / abstraction for OpenGL framebuffers and their attachments.
 * Example of usage:
 *
 *   t_fbo fbo;
 *   fbo.make()
 *      .attach_color(make_tex2d(1024, 1024, GL_RGB), 0)
 *      .attach_color(make_tex2d(1024, 1024, GL_RGBA), 1)
 *      .attach_depth(make_rbo(1024, 1024, GL_DEPTH_COMPONENT));
 *   ...
 *   fbo.apply();
 *   // do the drawing, etc.
 *
 * Note: the attachments might be better off written
 * using templates, or polymorphism, or both -
 * right now this is the simplest, most C-like version
 */


/*
 * The kinds of attachments possible.
 * These do not biject into OpenGL target enums
 * (there isn't a GL_RENDERBUFFER_MULTISAMPLE, for instance)
 */
enum att_target_enum: uint8_t
{
	tex2d, tex2d_msaa,
	tex2d_array, tex2d_array_msaa,
	rbo, rbo_msaa,
};

/*
 * A particular attachment
 */
struct t_attachment
{
	GLuint id = -1;
	GLenum storage_type; /* GL_RGBA32F etc. */

	int width = 0;
	int height = 0;

	short depth = 1; /* Only relevant in 3D targets */
	short samples = 1; /* Only relevant in MSAA targets */
	att_target_enum target;
};


/*
 * Make a correct attachment out of the given struct,
 * which is allocated and populated by caller; fills in ID
 */
t_attachment* attachment_finalize (t_attachment*);

/*
 * Allocate and make an attachment. Different attachments require different
 * parameters with which to be made - the number of samples, dimensions, etc.
 */
t_attachment* make_tex2d (int w, int h, GLenum internal_type);
t_attachment* make_tex2d_msaa (int w, int h,
		GLenum internal_type, short samples);
t_attachment* make_tex2d_array (int w, int h, int d, GLenum internal_type);
t_attachment* make_tex2d_array_msaa (int w, int h, int d,
		GLenum internal_type, short samples);
t_attachment* make_rbo (int w, int h, GLenum internal_type);
t_attachment* make_rbo_msaa (int w, int h,
		GLenum internal_type, short samples);

/*
 * In all functions, slice only matters when the target is 3D
 */
struct t_fbo
{
	GLuint id;
	int width = 0;
	int height = 0;

	constexpr static int num_clr_attachments = 8;

	struct t_attachment_ptr {
		t_attachment* ptr = nullptr;

		/* Which slice of a 3D texture this FBO uses */
		short slice_used = 0;

		bool taken () const { return ptr != nullptr; }
		t_attachment& operator* () { return *ptr; }
		t_attachment* operator-> () { return ptr; }
		const t_attachment& operator* () const { return *ptr; }
		const t_attachment* operator-> () const { return ptr; }
	};

	std::array<t_attachment_ptr, num_clr_attachments> color;
	t_attachment_ptr depth;

	t_fbo& make ();
	t_fbo& assert_complete ();

	t_fbo& attach_color (t_attachment* att, int idx = 0, short slice = 0);
	t_fbo& attach_depth (t_attachment* att, short slice = 0);

	t_fbo& set_mrt_slots (const std::vector<GLenum>& slots);

	void clear_color (int idx = 0);
	void clear_depth ();

	void bind () { glBindFramebuffer(GL_FRAMEBUFFER, id); }
	void apply () { bind(); glViewport(0, 0, width, height); }
};


/*
 * Screenspace buffers: those which correspond to the screen, and must have
 * their resolution updated accordingly, somewhat automatically
 */

void sspace_add_buffer (t_fbo& fbo);
void sspace_resize_buffers (int w, int h);

#endif // FRAMEBUFFER_H
