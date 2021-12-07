
#version 430

subroutine vec4 DebugTextureFunc(vec4);

uniform sampler2D sampler0;
uniform int component;
subroutine uniform DebugTextureFunc debugFunc;

out vec4 my_FragColor0;

layout (index = 0) subroutine(DebugTextureFunc) vec4 DebugSpectrum(vec4 color) {
	return vec4(color.xy * 100000.0, 0.0, 1.0);
}

layout (index = 1) subroutine(DebugTextureFunc) vec4 DebugDisplacement(vec4 color) {
	return vec4((color[component] + 0.4) / 0.8);
}

layout (index = 2) subroutine(DebugTextureFunc) vec4 DebugNormal(vec4 color) {
	return vec4(normalize(color.xyz) * 0.5 + vec3(0.5), 1.0);
	//return vec4(-color.w);
}

void main()
{
	ivec2 loc = ivec2(gl_FragCoord.xy) % ivec2(DISP_MAP_SIZE);
	my_FragColor0 = debugFunc(texelFetch(sampler0, loc, 0));
}
