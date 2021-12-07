
#version 330

#include "pbr_common.head"

uniform sampler2D baseColorSamp;

uniform vec3 lightPos;
uniform vec3 lightRight;
uniform vec3 lightUp;
uniform float luminance;
uniform float radius;

in vec4 wpos;
in vec3 wnorm;
in vec3 vdir;
in vec2 tex;

out vec4 my_FragColor0;

vec3 ClosestPointOnLine(vec3 p, vec3 a, vec3 b)
{
	vec3 bma = b - a;
	float t = dot(p - a, bma) / dot(bma, bma);

	return a + t * bma;
}

vec3 ClosestPointOnSegment(vec3 p, vec3 a, vec3 b)
{
	vec3 bma = b - a;
	float t = clamp(dot(p - a, bma) / dot(bma, bma), 0.0, 1.0);

	return a + t * bma;
}

float RectangleSolidAngle(vec3 p, vec3 p0, vec3 p1, vec3 p2, vec3 p3)
{
	vec3 v0 = p0 - p;
	vec3 v1 = p1 - p;
	vec3 v2 = p2 - p;
	vec3 v3 = p3 - p;

	vec3 n0 = normalize(cross(v0, v1));
	vec3 n1 = normalize(cross(v1, v2));
	vec3 n2 = normalize(cross(v2, v3));
	vec3 n3 = normalize(cross(v3, v0));

	float g0 = acos(dot(-n0, n1));
	float g1 = acos(dot(-n1, n2));
	float g2 = acos(dot(-n2, n3));
	float g3 = acos(dot(-n3, n0));

	return max(0.0, g0 + g1 + g2 + g3 - TWO_PI);
}

float IntegrateDiffuse(vec3 p, vec3 n)
{
	float illum = 0.0;

	if (radius > 1e-3) {
		// tube light
		vec3 A = lightPos - lightRight;
		vec3 B = lightPos + lightRight;

		// contribution from oriented rectangle
		vec3 forward = normalize(ClosestPointOnLine(p, A, B) - p);
		vec3 up = normalize(cross(lightRight, forward));

		vec3 p0 = A + radius * up;
		vec3 p1 = A - radius * up;
		vec3 p2 = B - radius * up;
		vec3 p3 = B + radius * up;

		float dw = RectangleSolidAngle(p, p0, p1, p2, p3);

		illum = dw * 0.2 * (
			clamp(dot(normalize(p0 - p), n), 0.0, 1.0) +
			clamp(dot(normalize(p1 - p), n), 0.0, 1.0) +
			clamp(dot(normalize(p2 - p), n), 0.0, 1.0) +
			clamp(dot(normalize(p3 - p), n), 0.0, 1.0) +
			clamp(dot(normalize(lightPos - p), n), 0.0, 1.0));

		// contribution from sphere at closest point
		vec3 C = ClosestPointOnSegment(p, A, B);
		vec3 dlu = C - p;

		float dist2 = dot(dlu, dlu);
		vec3 dl = dlu * inversesqrt(dist2);

		illum += PI * clamp(dot(dl, n), 0.0, 1.0) * ((radius * radius) / dist2);
	} else {
		// contribution from oriented rectangle
		vec3 p0 = lightPos - lightRight + lightUp;
		vec3 p1 = lightPos - lightRight - lightUp;
		vec3 p2 = lightPos + lightRight - lightUp;
		vec3 p3 = lightPos + lightRight + lightUp;

		vec3 ln = normalize(cross(lightRight, lightUp));
		float dw = RectangleSolidAngle(p, p0, p1, p2, p3);

		if (dot(ln, p - lightPos) > 0.0) {
			illum = dw * 0.2 * (
				clamp(dot(normalize(p0 - p), n), 0.0, 1.0) +
				clamp(dot(normalize(p1 - p), n), 0.0, 1.0) +
				clamp(dot(normalize(p2 - p), n), 0.0, 1.0) +
				clamp(dot(normalize(p3 - p), n), 0.0, 1.0) +
				clamp(dot(normalize(lightPos - p), n), 0.0, 1.0));
		}
	}

	return illum * luminance;
}

float IntegrateSpecular(out vec3 l, vec3 p, vec3 r)
{
	float illum = 0.0;

	if (radius > 1e-3) {
		// tube light
		vec3 A = lightPos - lightRight;
		vec3 B = lightPos + lightRight;

		// contribution from segment
		vec3 L0 = A - p;
		vec3 L1 = B - p;
		vec3 Ld = L1 - L0;

		float a = dot(Ld, Ld);
		float b = dot(r, Ld);
		float t = clamp(dot(L0, b * r - Ld) / (a - b * b), 0.0, 1.0);

		l = L0 + t * Ld;

		// contribution from sphere at closest point to reflection ray
		vec3 toray = dot(l, r) * r - l;

		l = normalize(l + toray * clamp(radius * inversesqrt(dot(toray, toray)), 0.0, 1.0));

		// TODO:
	} else {
		// oriented rectangle
		vec4 planeeq;
		
		// calculate plane equation
		planeeq.xyz = cross(lightRight, lightUp);
		planeeq.w = -dot(planeeq.xyz, lightPos);

		float rdotn = dot(r, planeeq.xyz);

		if (rdotn < 0.0) {
			float t = -dot(planeeq, vec4(p, 1.0)) / rdotn;
			
			// test if intersection point is inside the rect
			vec3 hitpoint = p + t * r;
			vec3 offset = hitpoint - lightPos;

			mat3 basis = mat3(lightUp, lightRight, planeeq.xyz);
			vec3 ndc = basis * offset;

			bool intersects = (ndc.x >= -1.0 && ndc.x <= 1.0) && (ndc.y >= -1.0 && ndc.y <= 1.0);

			if (intersects) {
				l = normalize(hitpoint - p);

				// TODO: energy conservation
				illum = 1.0;
			}
		}
	}

	return illum * luminance;
}

void main()
{
	vec3 n = normalize(wnorm);
	vec3 v = normalize(vdir);
	vec3 p = wpos.xyz;
	vec3 r = 2 * dot(v, n) * n - v;
	vec3 l;

	float diff_lum = IntegrateDiffuse(p, n);
	float spec_lum = IntegrateSpecular(l, p, r);

	//float ndotl = clamp(dot(n, l), 0.0, 1.0);

	// accumulate
	vec4 fd = BRDF_Lambertian(baseColorSamp, tex);

	//vec3 F0 = mix(vec3(0.04), baseColor.rgb, matParams.y);
	//vec3 fs = BRDF_CookTorrance(l, v, n, F0, matParams.x);

	// NOTE: specular won't work like this; have to use the LTC method
	vec3 final_color = (fd.rgb * fd.a * diff_lum); // + (fs * spec_lum * ndotl);

	my_FragColor0.rgb = final_color;
	my_FragColor0.a = fd.a;
}
