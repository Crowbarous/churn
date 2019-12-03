#include "prop.h"
#include "render/resource.h"
#include "core/signal.h"

SIG_HANDLER (prop, setpos)
{
	atovec3(arg, ent->pos);
}

FILL_IO_DATA (prop)
{
	SET_SIG_HANDLER(prop, setpos);
}

void e_prop::think () { }

void e_prop::apply_keyvals (const t_ent_keyvals& kv)
{
	apply_basic_keyvals(kv);

	KV_TRY_GET(kv["model"],
		model = get_model(val);,
		model = get_model("error"); );
	KV_TRY_GET(kv["mat"],
		material = get_material(val);,
		material = &mat_none; );
}

void e_prop::render () const
{
	material->apply();
	model->render();
}

void e_prop::cast_shadow () const
{
	model->render();
}

t_bound_box e_prop::get_bbox () const
{
	return model->bbox;
}
