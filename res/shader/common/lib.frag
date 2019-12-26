#version 130
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_explicit_attrib_location : require

/* The user shader which links against the lib should implement these */
lowp vec4 final_shade ();
vec3 surface_normal ();

/*
 * The locations must match the constants UNIFORM_LOC_*
 * specified in the source!
 */
#define LIGHTING_LSPACE 0u
#define LIGHTING_SSPACE 1u
#define SHADE_FINAL 2u
layout (location = 0) uniform uint stage;
layout (location = 1) uniform sampler2D prev_shadow_map;

layout (location = 2) uniform sampler2D depth_map;
layout (location = 3) uniform vec3 light_pos;
layout (location = 6) uniform vec3 light_rgb;
layout (location = 9) uniform mat4 light_view;

in vec3 world_normal;
in vec3 world_pos;
in mat3 TBN;
in vec4 screen_crd;

vec3 get_illum ();
vec3 sspace_light ();

const float EXP_FACTOR = 40.0;
const float NOBLEED_FACTOR = 0.4;
const float DEPTH_BIAS = 3e-4;

void main ()
{
	switch (stage) {
	case LIGHTING_LSPACE:

		// fill EVSM moments
		float depth = screen_crd.z / screen_crd.w;
		float pos = exp(EXP_FACTOR * depth);
		float neg = -exp(-EXP_FACTOR * depth);
		gl_FragColor = vec4(pos, pos*pos, neg, neg*neg);
		break;

	case LIGHTING_SSPACE:

		// calculate screenspace lighting
		gl_FragColor = vec4(sspace_light(), 1.0);
		break;

	case SHADE_FINAL:

		// call the actual user shader
		gl_FragColor = final_shade();
		gl_FragColor.rgb *= get_illum();
		break;
	}
}

in vec4 lspace_pos;

float chebyshev (vec2 moments, float depth);
vec3 sspace_light ()
{
	vec3 norm = TBN * normalize(surface_normal());

	vec3 lcoord = lspace_pos.xyz / lspace_pos.w;
	lcoord.z -= DEPTH_BIAS;
	float bright = max(1.0 - length(lcoord.xy), 0.0);

	vec4 moments = texture(depth_map, lcoord.st * 0.5 + 0.5);
	float val_pos = exp(EXP_FACTOR * lcoord.z);
	float val_neg = -exp(-EXP_FACTOR * lcoord.z);

	float illum = min(
		chebyshev(moments.xy, val_pos),
		chebyshev(moments.zw, val_neg));

	float cosine = dot(norm, normalize(light_pos - world_pos));
	illum *= bright * clamp(cosine, 0.0, 1.0);
	return light_rgb * illum + get_illum();
}

float linstep (float mn, float mx, float v)
{
	return clamp((v - mn) / (mx - mn), 0.0, 1.0);
}

float chebyshev (vec2 moments, float depth)
{
	if (depth <= moments.x)
		return 1.0;

	float variance = moments.y - moments.x * moments.x;
	variance = max(variance, 0.0);

	float d = depth - moments.x;
	float p_max = variance / (variance + d * d);

	return linstep(NOBLEED_FACTOR, 1.0, p_max);
}

vec3 get_illum ()
{
	vec2 s = (screen_crd.xyz / screen_crd.w).xy;
	s *= 0.5;
	s += 0.5;
	return texture(prev_shadow_map, s).rgb;
}