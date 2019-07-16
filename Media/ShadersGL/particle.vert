
#version 440

layout(location = 0) in vec4 particlePos;
layout(location = 1) in vec4 particleVel;
layout(location = 2) in vec4 particleColor;

out vec4 vs_particlePos;
out vec4 vs_particleVel;
out vec4 vs_particleColor;

void main()
{
	vs_particlePos		= particlePos;
	vs_particleVel		= particleVel;
	vs_particleColor	= particleColor;
}
