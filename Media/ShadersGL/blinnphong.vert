
#version 330

in vec3 my_Position;
in vec3 my_Normal;

#ifdef HARDWARE_INSTANCING
layout(location = 6) in vec4 world_row0;
layout(location = 7) in vec4 world_row1;
layout(location = 8) in vec4 world_row2;
layout(location = 9) in vec4 world_row3;
layout(location = 10) in vec4 instColor;
#endif

uniform mat4 matWorld;
uniform mat4 matViewProj;

uniform vec4 lightPos;
uniform vec4 eyePos;

out vec3 wnorm;
out vec3 vdir;
out vec3 ldir;

#ifdef HARDWARE_INSTANCING
out vec4 instcolor;
#endif

void main()
{
	mat4 world = matWorld;

#ifdef HARDWARE_INSTANCING
	mat4 instworld = mat4(world_row0, world_row1, world_row2, world_row3);

	world = world * instworld;
	instcolor = instColor;
#endif

	vec4 wpos = world * vec4(my_Position, 1.0);

	ldir = lightPos.xyz - wpos.xyz;
	vdir = eyePos.xyz - wpos.xyz;

	wnorm = (world * vec4(my_Normal, 0.0)).xyz;
	gl_Position = matViewProj * wpos;
}
