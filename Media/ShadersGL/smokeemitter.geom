
#version 440

layout(points) in;
layout(points, max_vertices = 40) out;

in vec4 vs_particlePos[];
in vec4 vs_particleVel[];
in vec4 vs_particleColor[];

layout(xfb_buffer = 0, xfb_stride = 48) out GS_OUTPUT {
	layout(xfb_offset = 0) vec4 particlePos;
	layout(xfb_offset = 16) vec4 particleVel;
	layout(xfb_offset = 32) vec4 particleColor;
} my_out;

layout(binding = 0) uniform sampler2D randomTex;
layout(location = 1) uniform float time;
layout(location = 2) uniform float emitRate;

vec3 randomVector(float xi) {
	return texture(randomTex, vec2(xi, 0.5f)).xyz - vec3(0.5);
};

float randomScalar(float xi) {
	return 2.0 * texture(randomTex, vec2(xi, 0.5f)).x - 1.0;
};

float randomFloat(float xi) {
	return texture(randomTex, vec2(xi, 0.5f)).x;
};

void main()
{
	float seed = (time * 123525.0 + gl_PrimitiveIDIn * 1111.0) / 1234.0;
	float age = vs_particleVel[0].w;

	if (age >= emitRate) {
		for (int i = 0; i < 40; ++i) {
			vec3 vel;

			vel.x = 0.45 * randomScalar(seed);
			vel.y = 1.0f;
			vel.z = 0.45 * randomScalar(seed + 4207.56);

			vel = normalize(vel);

			my_out.particlePos.xyz	= vs_particlePos[0].xyz;
			my_out.particlePos.w	= 1.0;
			my_out.particleVel.xyz	= vel * 0.75;
			my_out.particleVel.w	= 0.0;
			my_out.particleColor	= vec4(1.0);

			EmitVertex();
			seed += 4207.56;
		}
	}
}
