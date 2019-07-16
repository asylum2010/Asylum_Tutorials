
#version 440

layout(location = 0) in vec4 particlePos;
layout(location = 1) in vec4 particleVel;
layout(location = 2) in vec4 particleColor;

out vec4 vs_particlePos;
out vec4 vs_particleVel;
out vec4 vs_particleColor;

layout(location = 0) uniform float elapsedTime;
layout(location = 1) uniform float buoyancy;

void main()
{
	vec3 vel = particleVel.xyz;
	vel.y += buoyancy;

	vs_particlePos.xyz	= particlePos.xyz + vel * elapsedTime;
	vs_particlePos.w	= 1.0;
	vs_particleVel.xyz	= vel;
	vs_particleVel.w	= particleVel.w + elapsedTime;
	vs_particleColor	= particleColor;
}
