
#version 440

uniform vec4 clipPlanes;

in vec4 vpos;
in vec3 vnorm;

out vec4 my_FragColor0;
out float my_FragColor1;

void main()
{
	vec3 n = normalize(vnorm);

	// view space is right-handed
	float d = (-vpos.z - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);

	my_FragColor0 = vec4(n, 1.0);
	my_FragColor1 = d;
}
