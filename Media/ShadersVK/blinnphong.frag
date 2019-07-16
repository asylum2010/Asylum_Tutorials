
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D baseColor;

layout (location = 0) in vec3 wnorm;
layout (location = 1) in vec3 vdir;
layout (location = 2) in vec3 ldir;
layout (location = 3) in vec2 tex;

layout (location = 0) out vec4 my_FragColor0;

void main()
{
	vec4 base = texture(baseColor, tex);

	vec3 n = normalize(wnorm);
	vec3 l = normalize(ldir);
	vec3 v = normalize(vdir);
	vec3 h = normalize(v + l);

	float d = clamp(dot(l, n), 0.0, 1.0);
	float s = clamp(dot(h, n), 0.0, 1.0);

	s = pow(s, 80.0);

	my_FragColor0.rgb = base.rgb * d + vec3(s);
	my_FragColor0.a = base.a;
}
