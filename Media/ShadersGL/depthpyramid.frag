
#version 330

uniform sampler2D depth;
uniform int prevMipLevel;

out vec4 my_FragColor0;

void main()
{
	ivec2 loc = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(depth, prevMipLevel);
	
	ivec2 coordinate = clamp(loc * 2 + ivec2(loc.x & 1, loc.y & 1), ivec2(0), size - ivec2(1));
	float value = texelFetch(depth, coordinate, prevMipLevel).r;
	
	my_FragColor0 = vec4(value, 0.0, 0.0, 0.0);
}
