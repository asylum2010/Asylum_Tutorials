
#version 330

in vec3 my_Position;
in vec3 my_Normal;

uniform VertexUniformData {
	mat4 matWorld;
	mat4 matViewProj;
	vec4 lightPos;
	vec4 eyePos;
} vsuniforms;

out vec3 wnorm;
out vec3 vdir;
out vec3 ldir;

void main()
{
	vec4 wpos = vsuniforms.matWorld * vec4(my_Position, 1);

	ldir = vsuniforms.lightPos.xyz - wpos.xyz;
	vdir = vsuniforms.eyePos.xyz - wpos.xyz;

	wnorm = (vsuniforms.matWorld * vec4(my_Normal, 0)).xyz;
	gl_Position = vsuniforms.matViewProj * wpos;
}
