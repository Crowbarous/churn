#version 130

/* The user shade which links against the lib should implement this */
vec4 vertex_transform (vec4 v);

out vec2 tex_crd;
out vec3 world_normal;
out vec3 world_pos;

out vec4 lspace_pos;
out vec4 sspace_pos;

attribute vec3 world_tangent;
attribute vec3 world_bitangent;
out mat3 TBN;

const uint LIGHTING_LSPACE = 0u;
const uint LIGHTING_SSPACE = 1u;
const uint SHADE_FINAL = 2u;
uniform uint stage;

uniform mat4 lspace_model;

void main ()
{
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix *
		vertex_transform(gl_Vertex);

	tex_crd = gl_MultiTexCoord0.st;
	world_normal = (gl_ModelViewMatrix * vec4(gl_Normal, 0.0)).xyz;
	world_pos = (gl_ModelViewMatrix * gl_Vertex).xyz;

	if (stage == LIGHTING_SSPACE) {
		TBN = mat3(
			(gl_ModelViewMatrix * vec4(world_tangent, 0.0)).xyz,
			(gl_ModelViewMatrix * vec4(world_bitangent, 0.0)).xyz,
			world_normal);
		sspace_pos = gl_Position;
		lspace_pos = lspace_model * gl_Vertex;
	}
}
