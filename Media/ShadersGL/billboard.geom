
#version 440

layout(location = 0) uniform mat4 matWVP;
layout(location = 1) uniform mat4 matWorldView;
layout(location = 2) uniform vec2 clipRadius;
layout(location = 3) uniform float particleLife;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in vec4 vs_particlePos[];
in vec4 vs_particleVel[];
in vec4 vs_particleColor[];

out gl_PerVertex {
	vec4 gl_Position;
};

out vec3 tex;

void main()
{
	vec4 pos = matWVP * vs_particlePos[0];
	vec4 vpos = matWorldView * vs_particlePos[0];

	float age = vs_particleVel[0].w;
	float normage = age / particleLife;

	vec2 screenradius = clipRadius / vpos.z;

	gl_Position = pos + vec4(-screenradius.x, screenradius.y, 0.0, 0.0) * pos.w;
	tex = vec3(0.0, 1.0, normage);

	EmitVertex();

	gl_Position = pos + vec4(-screenradius.x, -screenradius.y, 0.0, 0.0) * pos.w;
	tex = vec3(0.0, 0.0, normage);

	EmitVertex();

	gl_Position = pos + vec4(screenradius.x, screenradius.y, 0.0, 0.0) * pos.w;
	tex = vec3(1.0, 1.0, normage);

	EmitVertex();

	gl_Position = pos + vec4(screenradius.x, -screenradius.y, 0.0, 0.0) * pos.w;
	tex = vec3(1.0, 0.0, normage);

	EmitVertex();
}
