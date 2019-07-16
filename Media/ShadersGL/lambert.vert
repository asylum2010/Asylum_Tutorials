
#version 150

in vec3 my_Position;
in vec3 my_Normal;

#ifdef TEXTURED
in vec2 my_Texcoord0;
out vec2 tex;

uniform mat4 matTexture;
#endif

uniform mat4 matWorld;
uniform mat4 matWorldInv;
uniform mat4 matViewProj;

uniform vec4 lightPos;

out vec3 wnorm;
out vec3 ldir;

void main()
{
	vec4 wpos = matWorld * vec4(my_Position, 1.0);

	// supports both point and directional lights
	ldir = lightPos.xyz - wpos.xyz * lightPos.w;

#ifdef TEXTURED
	tex = (matTexture * vec4(my_Texcoord0, 0.0, 1.0)).xy;
#endif

	wnorm = (vec4(my_Normal, 0.0) * matWorldInv).xyz;
	gl_Position = matViewProj * wpos;
}
