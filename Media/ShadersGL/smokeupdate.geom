
#version 440

layout(points) in;
layout(points, max_vertices = 120) out;

in vec4 vs_particlePos[];
in vec4 vs_particleVel[];
in vec4 vs_particleColor[];

layout(location = 2) uniform float particleLife;

layout(xfb_buffer = 0) out GS_OUTPUT {
	layout(xfb_offset = 0) vec4 particlePos;
	layout(xfb_offset = 16) vec4 particleVel;
	layout(xfb_offset = 32) vec4 particleColor;
} my_out;

void main()
{
	float age = vs_particleVel[0].w;

	if (age < particleLife) {
		my_out.particlePos		= vs_particlePos[0];
		my_out.particleVel		= vs_particleVel[0];
		my_out.particleColor	= vs_particleColor[0];

		EmitVertex();
	}
}
