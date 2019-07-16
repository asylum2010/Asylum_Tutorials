
#version 330

uniform vec2 clipPlanes;
uniform int isPerspective;

in vec4 ltov;
out vec4 my_FragColor0;

void main()
{
	// linearized depth (handles ortho projection too)
	float d01 = (ltov.z * 0.5 + 0.5);
	float d = (ltov.w - clipPlanes.x) / (clipPlanes.y - clipPlanes.x);

	d = ((isPerspective == 1) ? d : d01);

	my_FragColor0 = vec4(d, d * d, 0.0, 0.0);
}
