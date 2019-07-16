
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler2D sampler1;

layout (input_attachment_index = 0, binding = 2) uniform subpassInput input0;
layout (input_attachment_index = 1, binding = 3) uniform subpassInput input1;

layout (location = 0) in vec2 tex;
layout (location = 1) in vec4 cpos;

layout (location = 0) out vec4 outColor;

void main()
{
	vec2 ptex = (cpos.xy / cpos.w) * 0.5 + vec2(0.5);

	vec4 base = texture(sampler1, tex);
	vec4 diffirrad = subpassLoad(input0);
	vec4 specirrad = subpassLoad(input1);

	outColor.rgb = base.rgb * diffirrad.rgb + specirrad.rgb;
	outColor.a = base.a;
}
