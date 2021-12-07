
#version 430

// NOTE: also defined in fragment shader
#define BLEND_START		8		// m
#define BLEND_END		200		// m

in vec3 my_Position;

layout (binding = 0) uniform sampler2D displacement;
layout (binding = 1) uniform sampler2D perlin;

uniform mat4 matLocal;
uniform mat4 matWorld;
uniform mat4 matViewProj;
uniform vec4 uvParams;
uniform vec2 perlinOffset;
uniform vec3 eyePos;

out vec3 vdir;
out vec2 tex;

void main()
{
	// NOTE: also defined in fragment shader
	const vec3 perlinFrequency	= vec3(1.12, 0.59, 0.23);
	const vec3 perlinAmplitude	= vec3(0.35, 0.42, 0.57);

	// transform to world space
	vec4 pos_local = matLocal * vec4(my_Position, 1.0);
	vec2 uv_local = pos_local.xy * uvParams.x + vec2(uvParams.y);
	vec3 disp = texture(displacement, uv_local).xyz;

	pos_local = matWorld * pos_local;
	vdir = eyePos - pos_local.xyz;
	tex = uv_local;

	// blend with Perlin waves
	float dist = length(vdir.xz);
	float factor = clamp((BLEND_END - dist) / (BLEND_END - BLEND_START), 0.0, 1.0);
	float perl = 0.0;

	if (factor < 1.0) {
		vec2 ptex = uv_local + uvParams.zw;
		
		float p0 = texture(perlin, ptex * perlinFrequency.x + perlinOffset).a;
		float p1 = texture(perlin, ptex * perlinFrequency.y + perlinOffset).a;
		float p2 = texture(perlin, ptex * perlinFrequency.z + perlinOffset).a;

		perl = dot(vec3(p0, p1, p2), perlinAmplitude);
	}

	disp = mix(vec3(0.0, perl, 0.0), disp, factor);
	gl_Position = matViewProj * vec4(pos_local.xyz + disp, 1.0);
}
