
#version 430

struct LightParticle
{
	vec4 Color;
	vec4 Previous;
	vec4 Current;
	vec4 VelocityAndRadius;
};

struct ListHead
{
	uvec4 StartAndCount;
};

struct ListNode
{
	uvec4 LightIndexAndNext;
};

layout(std140, binding = 0) readonly buffer HeadBuffer {
	ListHead data[];
} headbuffer;

layout(std140, binding = 1) readonly buffer NodeBuffer {
	ListNode data[];
} nodebuffer;

layout(std140, binding = 2) readonly buffer LightBuffer {
	LightParticle data[];
} lightbuffer;

uniform sampler2D sampler0;
uniform float alpha;
uniform int numTilesX;

in vec4 wpos;
in vec3 wnorm;
in vec3 vdir;
in vec2 tex;

out vec4 my_FragColor0;

float Attenuate(vec3 ldir, float radius)
{
	float atten = dot(ldir, ldir) / radius;

	atten = 1.0 / (atten * 15.0 + 1.0);
	atten = (atten - 0.0625) * 1.066666;

	return clamp(atten, 0.0, 1.0);
}

layout(early_fragment_tests) in;
void main()
{
	ivec2	loc = ivec2(gl_FragCoord.xy);
	ivec2	tileID = loc / ivec2(16, 16);
	int		index = tileID.y * numTilesX + tileID.x;

	vec4 base = texture(sampler0, tex);
	vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 lightcolor;
	vec4 lightpos;

	vec3 irrad;
	vec3 l;
	vec3 h;
	vec3 n = normalize(wnorm);
	vec3 v = normalize(vdir);

	float diff;
	float spec;
	float atten;
	float radius;

	uint start = headbuffer.data[index].StartAndCount.x;
	uint count = headbuffer.data[index].StartAndCount.y;
	uint nodeID = start;
	uint lightID;

	for (uint i = 0; i < count; ++i) {
		lightID = nodebuffer.data[nodeID].LightIndexAndNext.x;
		nodeID = nodebuffer.data[nodeID].LightIndexAndNext.y;

		lightcolor = lightbuffer.data[lightID].Color;
		lightpos = mix(lightbuffer.data[lightID].Previous, lightbuffer.data[lightID].Current, alpha);
		radius = lightbuffer.data[lightID].VelocityAndRadius.w;

		l = lightpos.xyz - wpos.xyz;
		atten = Attenuate(l, radius);

		l = normalize(l);
		h = normalize(l + v);

		diff = max(dot(l, n), 0);
		spec = pow(max(dot(h, n), 0), 80.0);
		
		irrad = lightcolor.rgb * (base.rgb * diff + vec3(spec)) * atten;
		color.rgb += irrad;
	}

	my_FragColor0 = color;
}
