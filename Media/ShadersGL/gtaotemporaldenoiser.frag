
#version 330

uniform sampler2D historyBuffer;
uniform sampler2D currIteration;
uniform sampler2D prevDepthBuffer;
uniform sampler2D currDepthBuffer;

in vec2 tex;
out vec2 my_FragColor0;

uniform mat4 matPrevViewInv;
uniform mat4 matCurrViewInv;
uniform mat4 matPrevView;
uniform mat4 matProj;

uniform vec4 projInfo;
uniform vec4 clipPlanes;

vec3 GetWorldPosition(float d, vec2 spos, mat4 viewinv)
{
	vec4 vpos = vec4(1.0);

	// NOTE: depth is linearized
	vpos.z = clipPlanes.x + d * (clipPlanes.y - clipPlanes.x);
	vpos.xy = (spos * projInfo.xy) * vpos.z;
	
	vpos.z = -vpos.z;	// by def

	vec4 wpos = (viewinv * vpos);
	return wpos.xyz;
}

vec3 GetPrevScreenPosition(vec3 wpos)
{
	vec4 vpos = matPrevView * vec4(wpos, 1.0);
	vec4 cpos = matProj * vpos;

	float d = (-vpos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);
	return vec3(cpos.xy / cpos.w, d);
}

void main()
{
	vec2 spos = tex * 2.0 - vec2(1.0);

	// unproject to current frame world space
	float currdepth = texture(currDepthBuffer, tex).r;
	vec3 currpos = GetWorldPosition(currdepth, spos, matCurrViewInv);

	// reproject to previous frame screen space
	vec3 tempspos = GetPrevScreenPosition(currpos);
	vec2 temptex = tempspos.xy * 0.5 + vec2(0.5);

	// unproject to previous frame world space
	float prevdepth = texture(prevDepthBuffer, temptex).r;
	vec3 prevpos = GetWorldPosition(prevdepth, tempspos.xy, matPrevViewInv);

	// detect disocclusion
	float dist2 = dot(currpos - prevpos, currpos - prevpos);
	
	// fetch values
	vec2 currAO = texture(currIteration, tex).rg;
	vec2 accumAO = texture(historyBuffer, temptex).rg;

	float currn = 0.0;
	float prevn = accumAO.y * 6.0;
	float ao = currAO.x;

	// NOTE: large value causes waving/ghosting, small value causes flickering
	if (dist2 < 1e-4) {
		// no disocclusion, continue convergence
		currn = min(prevn + 1.0, 6.0);
		ao = mix(accumAO.x, currAO.x, 1.0 / currn);
	}

	my_FragColor0 = vec2(ao, currn / 6.0);
}
