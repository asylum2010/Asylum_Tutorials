
#include <cassert>
#include <cmath>
#include <algorithm>

#include "geometryutils.h"

namespace GeometryUtils {

static uint32_t FindAdjacentIndex(uint32_t i1, uint32_t i2, uint32_t i3, uint32_t e1, uint32_t e2)
{
	// NOTE: can be simplified
	if (e1 == i1) {
		if (e2 == i2) {
			return i3;
		} else if (e2 == i3) {
			return i2;
		}
	} else if (e1 == i2) {
		if (e2 == i1) {
			return i3;
		} else if (e2 == i3) {
			return i1;
		}
	} else if (e1 == i3) {
		if (e2 == i2) {
			return i1;
		} else if (e2 == i1) {
			return i2;
		}
	}

	return UINT32_MAX;
}

// --- Structures impl --------------------------------------------------------

Edge::Edge()
{
	v1 = v2	= { 0, 0, 0 };
	n1 = n2	= { 0, 0, 0 };
	i1 = i2	= UINT32_MAX;
	other	= UINT32_MAX;
}

// --- Functions impl ---------------------------------------------------------

void AccumulateTangentFrame(TBNVertex* vdata, uint32_t i1, uint32_t i2, uint32_t i3)
{
	TBNVertex* v1 = (vdata + i1);
	TBNVertex* v2 = (vdata + i2);
	TBNVertex* v3 = (vdata + i3);

	float a[3], b[3], c[3], t[3];

	a[0] = v2->x - v1->x;
	a[1] = v2->y - v1->y;
	a[2] = v2->z - v1->z;

	c[0] = v3->x - v1->x;
	c[1] = v3->y - v1->y;
	c[2] = v3->z - v1->z;

	float s1 = v2->u - v1->u;
	float s2 = v3->u - v1->u;
	float t1 = v2->v - v1->v;
	float t2 = v3->v - v1->v;

	float invdet = 1.0f / ((s1 * t2 - s2 * t1) + 0.0001f);

	t[0] = (t2 * a[0] - t1 * c[0]) * invdet;
	t[1] = (t2 * a[1] - t1 * c[1]) * invdet;
	t[2] = (t2 * a[2] - t1 * c[2]) * invdet;

	b[0] = (s1 * c[0] - s2 * a[0]) * invdet;
	b[1] = (s1 * c[1] - s2 * a[1]) * invdet;
	b[2] = (s1 * c[2] - s2 * a[2]) * invdet;

	v1->tx += t[0];	v2->tx += t[0];	v3->tx += t[0];
	v1->ty += t[1];	v2->ty += t[1];	v3->ty += t[1];
	v1->tz += t[2];	v2->tz += t[2];	v3->tz += t[2];

	v1->bx += b[0];	v2->bx += b[0];	v3->bx += b[0];
	v1->by += b[1];	v2->by += b[1];	v3->by += b[1];
	v1->bz += b[2];	v2->bz += b[2];	v3->bz += b[2];
}

void OrthogonalizeTangentFrame(TBNVertex& vert)
{
	Math::Vector3 t, b, q;

	bool tangentinvalid = (Math::Vec3Dot(&vert.tx, &vert.tx) < 1e-6f);
	bool bitangentinvalid = (Math::Vec3Dot(&vert.bx, &vert.bx) < 1e-6f);

	if (tangentinvalid && bitangentinvalid) {
		// TODO:
		return;
	} else if (tangentinvalid) {
		Math::Vec3Cross((Math::Vector3&)vert.tx, &vert.bx, &vert.nx);
	} else if (bitangentinvalid) {
		Math::Vec3Cross((Math::Vector3&)vert.bx, &vert.nx, &vert.tx);
	}

	Math::Vec3Scale(t, &vert.nx, Math::Vec3Dot(&vert.nx, &vert.tx));
	Math::Vec3Subtract(t, &vert.tx, t);

	Math::Vec3Scale(q, t, (Math::Vec3Dot(t, &vert.bx) / Math::Vec3Dot(&vert.tx, &vert.tx)));
	Math::Vec3Scale(b, &vert.nx, Math::Vec3Dot(&vert.nx, &vert.bx));
	Math::Vec3Subtract(b, &vert.bx, b);
	Math::Vec3Subtract(b, b, q);

	if (Math::Vec3Dot(t, t) >= 1e-6f)
		Math::Vec3Normalize((Math::Vector3&)vert.tx, t);
	
	if (Math::Vec3Dot(b, b) >= 1e-6f)
		Math::Vec3Normalize((Math::Vector3&)vert.bx, b);
}

void CreatePlane(CommonVertex* outvdata, uint32_t* outidata, float width, float depth)
{
	outvdata[0] = { width * -0.5f, 0, depth * 0.5f,		0, 1, 0,	0, 0 };
	outvdata[1] = { width * 0.5f, 0, depth * 0.5f,		0, 1, 0,	1, 0 };
	outvdata[2] = { width * 0.5f, 0, depth * -0.5f,		0, 1, 0,	1, 1 };
	outvdata[3] = { width * -0.5f, 0, depth * -0.5f,	0, 1, 0,	0, 1 };

	outidata[0] = 0;
	outidata[1] = 1;
	outidata[2] = 2;

	outidata[3] = 0;
	outidata[4] = 2;
	outidata[5] = 3;
}

void CreateBox(CommonVertex* outvdata, uint32_t* outidata, float width, float height, float depth)
{
	outvdata[0] = { width * -0.5f, height * -0.5f, depth * 0.5f,	0, -1, 0,	0, 0 };
	outvdata[1] = { width * -0.5f, height * -0.5f, depth * -0.5f,	0, -1, 0,	0, 1 };
	outvdata[2] = { width * 0.5f, height * -0.5f, depth * -0.5f,	0, -1, 0,	1, 1 };
	outvdata[3] = { width * 0.5f, height * -0.5f, depth * 0.5f,		0, -1, 0,	1, 0 };

	outvdata[4] = { width * -0.5f, height * 0.5f, depth * 0.5f,		0, 1, 0,	1, 0 };
	outvdata[5] = { width * 0.5f, height * 0.5f, depth * 0.5f,		0, 1, 0,	0, 0 };
	outvdata[6] = { width * 0.5f, height * 0.5f, depth * -0.5f,		0, 1, 0,	0, 1 };
	outvdata[7] = { width * -0.5f, height * 0.5f, depth * -0.5f,	0, 1, 0,	1, 1 };

	outvdata[8] = { width * -0.5f, height * -0.5f, depth * 0.5f,	0, 0, 1,	1, 0,};
	outvdata[9] = { width * 0.5f, height * -0.5f, depth * 0.5f,		0, 0, 1,	0, 0,};
	outvdata[10] = { width * 0.5f, height * 0.5f, depth * 0.5f,		0, 0, 1,	0, 1,};
	outvdata[11] = { width * -0.5f, height * 0.5f, depth * 0.5f,	0, 0, 1,	1, 1,};

	outvdata[12] = { width * 0.5f, height * -0.5f, depth * 0.5f,	1, 0, 0,	1, 0 };
	outvdata[13] = { width * 0.5f, height * -0.5f, depth * -0.5f,	1, 0, 0,	0, 0 };
	outvdata[14] = { width * 0.5f, height * 0.5f, depth * -0.5f,	1, 0, 0,	0, 1 };
	outvdata[15] = { width * 0.5f, height * 0.5f, depth * 0.5f,		1, 0, 0,	1, 1 };

	outvdata[16] = { width * 0.5f, height * -0.5f, depth * -0.5f,	0, 0, -1,	1, 0 };
	outvdata[17] = { width * -0.5f, height * -0.5f, depth * -0.5f,	0, 0, -1,	0, 0 };
	outvdata[18] = { width * -0.5f, height * 0.5f, depth * -0.5f,	0, 0, -1,	0, 1 };
	outvdata[19] = { width * 0.5f, height * 0.5f, depth * -0.5f,	0, 0, -1,	1, 1 };

	outvdata[20] = { width * -0.5f, height * -0.5f, depth * -0.5f,	-1, 0, 0,	1, 0 };
	outvdata[21] = { width * -0.5f, height * -0.5f, depth * 0.5f,	-1, 0, 0,	0, 0 };
	outvdata[22] = { width * -0.5f, height * 0.5f, depth * 0.5f,	-1, 0, 0,	0, 1 };
	outvdata[23] = { width * -0.5f, height * 0.5f, depth * -0.5f,	-1, 0, 0,	1, 1 };

	uint32_t indices[36] = {
		0, 1, 2, 2, 3, 0, 
		4, 5, 6, 6, 7, 4,
		8, 9, 10, 10, 11, 8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20
	};

	memcpy(outidata, indices, 36 * sizeof(uint32_t));
}

void CreateSphere(CommonVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments)
{
	assert(radius > 0.0f);

	vsegments = std::max<uint16_t>(vsegments, 4);
	hsegments = std::max<uint16_t>(hsegments, 4);

	float theta, phi;

	// body vertices
	for (int j = 1; j < hsegments - 1; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			outvdata->u = ((float)i / (float)(vsegments - 1));
			outvdata->v = ((float)j / (float)(hsegments - 1));

			theta = outvdata->v * Math::PI;
			phi = outvdata->u * Math::TWO_PI;

			outvdata->nx = sinf(theta) * cosf(phi);
			outvdata->ny = cosf(theta);
			outvdata->nz = -sinf(theta) * sinf(phi);

			outvdata->x = outvdata->nx * radius;
			outvdata->y = outvdata->ny * radius;
			outvdata->z = outvdata->nz * radius;

			++outvdata;
		}
	}

	// top vertices
	for (int i = 0; i < vsegments - 1; ++i) {
		outvdata->x = outvdata->z = 0;
		outvdata->y = radius;

		outvdata->nx = outvdata->nz = 0;
		outvdata->ny = 1;

		outvdata->u = ((float)i / (float)(vsegments - 1));
		outvdata->v = 0;

		++outvdata;
	}

	// bottom vertices
	for (int i = 0; i < vsegments - 1; ++i) {
		outvdata->x = outvdata->z = 0;
		outvdata->y = -radius;

		outvdata->nx = outvdata->nz = 0;
		outvdata->ny = -1;

		outvdata->u = ((float)i / (float)(vsegments - 1));
		outvdata->v = 1;

		++outvdata;
	}

	// indices
	for (int j = 0; j < hsegments - 3; ++j) {
		for (int i = 0; i < vsegments - 1; ++i) {
			outidata[0] = j * vsegments + i;
			outidata[1] = (j + 1) * vsegments + i + 1;
			outidata[2] = j * vsegments + i + 1;

			outidata[3] = j * vsegments + i;
			outidata[4] = (j + 1) * vsegments + i;
			outidata[5] = (j + 1) * vsegments + i + 1;

			outidata += 6;
		}
	}

	for (int i = 0; i < vsegments - 1; ++i) {
		// top
		outidata[0] = (hsegments - 2) * vsegments + i;
		outidata[1] = i;
		outidata[2] = i + 1;

		// bottom
		outidata[3] = (hsegments - 2) * vsegments + (vsegments - 1) + i;
		outidata[4] = (hsegments - 3) * vsegments + i + 1;
		outidata[5] = (hsegments - 3) * vsegments + i;

		outidata += 6;
	}
}

void CreateCapsule(CommonVertex* outvdata, uint32_t* outidata, float length, float radius, uint16_t vsegments, uint16_t hsegments)
{
	assert(radius > 0.0f);

	vsegments = std::max<uint16_t>(vsegments, 4);
	hsegments = std::max<uint16_t>(hsegments, 4);

	// NOTE: [first vertex, last vertex]; for 16x16 total is [0, 253]
	float theta, phi;

	// top hemisphere vertices [0, (hs / 2 - 1) * vs - 1]
	for (int j = 1; j < hsegments / 2; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			outvdata->u = ((float)i / (float)(vsegments - 1));
			outvdata->v = ((float)j / (float)(hsegments - 1));

			theta = outvdata->v * Math::PI;
			phi = outvdata->u * Math::TWO_PI;

			outvdata->nx = sinf(theta) * cosf(phi);
			outvdata->ny = cosf(theta);
			outvdata->nz = -sinf(theta) * sinf(phi);

			outvdata->x = outvdata->nx * radius;
			outvdata->y = outvdata->ny * radius + length * 0.5f;
			outvdata->z = outvdata->nz * radius;

			++outvdata;
		}
	}

	// bottom hemisphere vertices [(hs / 2 - 1) * vs, ... + (hs / 2 - 1) * vs - 1]
	for (int j = hsegments / 2; j < hsegments - 1; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			outvdata->u = ((float)i / (float)(vsegments - 1));
			outvdata->v = ((float)j / (float)(hsegments - 1));

			theta = outvdata->v * Math::PI;
			phi = outvdata->u * Math::TWO_PI;

			outvdata->nx = sinf(theta) * cosf(phi);
			outvdata->ny = cosf(theta);
			outvdata->nz = -sinf(theta) * sinf(phi);

			outvdata->x = outvdata->nx * radius;
			outvdata->y = outvdata->ny * radius - length * 0.5f;
			outvdata->z = outvdata->nz * radius;

			++outvdata;
		}
	}

	// top vertices [2 * (hs / 2 - 1) * vs, ... + vs - 2]
	for (int i = 0; i < vsegments - 1; ++i) {
		outvdata->x = outvdata->z = 0;
		outvdata->y = radius + length * 0.5f;

		outvdata->nx = outvdata->nz = 0;
		outvdata->ny = 1;

		outvdata->u = ((float)i / (float)(vsegments - 1));
		outvdata->v = 0;

		++outvdata;
	}

	// bottom vertices [2 * (hs / 2 - 1) * vs + (vs - 1), ... + vs - 2]
	for (int i = 0; i < vsegments - 1; ++i) {
		outvdata->x = outvdata->z = 0;
		outvdata->y = -radius - length * 0.5f;

		outvdata->nx = outvdata->nz = 0;
		outvdata->ny = -1;

		outvdata->u = ((float)i / (float)(vsegments - 1));
		outvdata->v = 1;

		++outvdata;
	}

	// top hemisphere indices
	for (int j = 0; j < hsegments / 2 - 2; ++j) {
		for (int i = 0; i < vsegments - 1; ++i) {
			outidata[0] = j * vsegments + i;
			outidata[1] = (j + 1) * vsegments + i + 1;
			outidata[2] = j * vsegments + i + 1;

			outidata[3] = j * vsegments + i;
			outidata[4] = (j + 1) * vsegments + i;
			outidata[5] = (j + 1) * vsegments + i + 1;

			outidata += 6;
		}
	}

	// bottom hemisphere indices
	uint16_t start = (hsegments / 2 - 1);

	for (int j = start; j < start + hsegments / 2 - 2; ++j) {
		for (int i = 0; i < vsegments - 1; ++i) {
			outidata[0] = j * vsegments + i;
			outidata[1] = (j + 1) * vsegments + i + 1;
			outidata[2] = j * vsegments + i + 1;

			outidata[3] = j * vsegments + i;
			outidata[4] = (j + 1) * vsegments + i;
			outidata[5] = (j + 1) * vsegments + i + 1;

			outidata += 6;
		}
	}

	// top/bottom hemisphere caps
	for (int i = 0; i < vsegments - 1; ++i) {
		// top hemisphere vertices [0, (hs / 2 - 1) * vs - 1]
		// top vertices [2 * (hs / 2 - 1) * vs, ... + vs - 2]
		outidata[0] = (hsegments - 2) * vsegments + i;
		outidata[1] = i;
		outidata[2] = i + 1;

		// bottom hemisphere vertices [(hs / 2 - 1) * vs, ... + (hs / 2 - 1) * vs - 1]
		// bottom vertices [2 * (hs / 2 - 1) * vs + (vs - 1), ... + vs - 2]
		outidata[3] = (hsegments - 2) * vsegments + (vsegments - 1) + i;
		outidata[4] = (hsegments / 2 - 1) * vsegments + (hsegments / 2 - 2) * vsegments + i + 1;
		outidata[5] = (hsegments / 2 - 1) * vsegments + (hsegments / 2 - 2) * vsegments + i;

		outidata += 6;
	}

	// body indices
	for (int i = 0; i < vsegments - 1; ++i) {
		outidata[0] = (hsegments / 2 - 2) * vsegments + i + 1;
		outidata[1] = (hsegments / 2 - 2) * vsegments + i;
		outidata[2] = (hsegments / 2 - 1) * vsegments + i;

		outidata[3] = (hsegments / 2 - 2) * vsegments + i + 1;
		outidata[4] = (hsegments / 2 - 1) * vsegments + i;
		outidata[5] = (hsegments / 2 - 1) * vsegments + i + 1;

		outidata += 6;
	}
}

void CreateLShape(CommonVertex* outvdata, uint32_t* outidata, float basewidth, float innerwidth, float height, float basedepth, float innerdepth)
{
	// half sizes
	float hbwidth = 0.5f * basewidth;
	float hbdepth = 0.5f * basedepth;
	float hheight = 0.5f * height;

	// relative origins
	float rzorig = hbdepth - innerdepth;
	float rxorig = hbwidth - innerwidth;

	// Z- face
	outvdata[0].x = -hbwidth;	outvdata[1].x = -hbwidth;	outvdata[2].x = hbwidth;	outvdata[3].x = hbwidth;
	outvdata[0].y = -hheight;	outvdata[1].y = hheight;	outvdata[2].y = hheight;	outvdata[3].y = -hheight;
	outvdata[0].z = -hbdepth;	outvdata[1].z = -hbdepth;	outvdata[2].z = -hbdepth;	outvdata[3].z = -hbdepth;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = 0;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = 0;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = -1;

	outvdata[0].u = 0;			outvdata[1].u = 0;			outvdata[2].u = 1;			outvdata[3].u = 1;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 1;

	outvdata += 4;

	// X- face
	outvdata[0].x = -hbwidth;	outvdata[1].x = -hbwidth;	outvdata[2].x = -hbwidth;	outvdata[3].x = -hbwidth;
	outvdata[0].y = -hheight;	outvdata[1].y = hheight;	outvdata[2].y = hheight;	outvdata[3].y = -hheight;
	outvdata[0].z = hbdepth;	outvdata[1].z = hbdepth;	outvdata[2].z = -hbdepth;	outvdata[3].z = -hbdepth;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = -1;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = 0;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = 0;

	outvdata[0].u = 0;			outvdata[1].u = 0;			outvdata[2].u = 1;			outvdata[3].u = 1;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 1;

	outvdata += 4;

	// X+ faces
	outvdata[0].x = rxorig;		outvdata[1].x = rxorig;		outvdata[2].x = rxorig;		outvdata[3].x = rxorig;
	outvdata[0].y = -hheight;	outvdata[1].y = hheight;	outvdata[2].y = hheight;	outvdata[3].y = -hheight;
	outvdata[0].z = rzorig;		outvdata[1].z = rzorig;		outvdata[2].z = hbdepth;	outvdata[3].z = hbdepth;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = 1;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = 0;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = 0;

	outvdata[0].u = 0.5f;		outvdata[1].u = 0.5f;		outvdata[2].u = 1;			outvdata[3].u = 1;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 1;

	outvdata += 4;

	outvdata[0].x = hbwidth;	outvdata[1].x = hbwidth;	outvdata[2].x = hbwidth;	outvdata[3].x = hbwidth;
	outvdata[0].y = -hheight;	outvdata[1].y = hheight;	outvdata[2].y = hheight;	outvdata[3].y = -hheight;
	outvdata[0].z = -hbdepth;	outvdata[1].z = -hbdepth;	outvdata[2].z = rzorig;		outvdata[3].z = rzorig;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = 1;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = 0;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = 0;

	outvdata[0].u = 0;			outvdata[1].u = 0;			outvdata[2].u = 0.5f;		outvdata[3].u = 0.5f;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 1;

	outvdata += 4;

	// Z+ faces
	outvdata[0].x = rxorig;		outvdata[1].x = rxorig;		outvdata[2].x = -hbwidth;	outvdata[3].x = -hbwidth;
	outvdata[0].y = -hheight;	outvdata[1].y = hheight;	outvdata[2].y = hheight;	outvdata[3].y = -hheight;
	outvdata[0].z = hbdepth;	outvdata[1].z = hbdepth;	outvdata[2].z = hbdepth;	outvdata[3].z = hbdepth;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = 0;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = 0;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = 1;

	outvdata[0].u = 0.5f;		outvdata[1].u = 0.5f;		outvdata[2].u = 1;			outvdata[3].u = 1;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 1;

	outvdata += 4;

	outvdata[0].x = hbwidth;	outvdata[1].x = hbwidth;	outvdata[2].x = rxorig;		outvdata[3].x = rxorig;
	outvdata[0].y = -hheight;	outvdata[1].y = hheight;	outvdata[2].y = hheight;	outvdata[3].y = -hheight;
	outvdata[0].z = rzorig;		outvdata[1].z = rzorig;		outvdata[2].z = rzorig;		outvdata[3].z = rzorig;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = 0;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = 0;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = 1;

	outvdata[0].u = 0;			outvdata[1].u = 0;			outvdata[2].u = 0.5f;		outvdata[3].u = 0.5f;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 1;

	outvdata += 4;

	// Y+ face
	outvdata[0].x = -hbwidth;	outvdata[1].x = -hbwidth;	outvdata[2].x = rxorig;		outvdata[3].x = rxorig;		outvdata[4].x = hbwidth;	outvdata[5].x = hbwidth;
	outvdata[0].y = hheight;	outvdata[1].y = hheight;	outvdata[2].y = hheight;	outvdata[3].y = hheight;	outvdata[4].y = hheight;	outvdata[5].y = hheight;
	outvdata[0].z = -hbdepth;	outvdata[1].z = hbdepth;	outvdata[2].z = hbdepth;	outvdata[3].z = rzorig;		outvdata[4].z = rzorig;		outvdata[5].z = -hbdepth;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = outvdata[4].nx = outvdata[5].nx = 0;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = outvdata[4].ny = outvdata[5].ny = 1;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = outvdata[4].nz = outvdata[5].nz = 0;

	outvdata[0].u = 0;			outvdata[1].u = 0;			outvdata[2].u = 0.5f;		outvdata[3].u = 0.5f;		outvdata[4].u = 1;			outvdata[5].u = 1;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 0.5f;		outvdata[4].v = 0.5f;		outvdata[5].v = 1;

	outvdata += 6;

	// Y- face
	outvdata[0].x = -hbwidth;	outvdata[1].x = -hbwidth;	outvdata[2].x = hbwidth;	outvdata[3].x = hbwidth;	outvdata[4].x = rxorig;		outvdata[5].x = rxorig;
	outvdata[0].y = -hheight;	outvdata[1].y = -hheight;	outvdata[2].y = -hheight;	outvdata[3].y = -hheight;	outvdata[4].y = -hheight;	outvdata[5].y = -hheight;
	outvdata[0].z = hbdepth;	outvdata[1].z = -hbdepth;	outvdata[2].z = -hbdepth;	outvdata[3].z = rzorig;		outvdata[4].z = rzorig;		outvdata[5].z = hbdepth;

	outvdata[0].nx = outvdata[1].nx = outvdata[2].nx = outvdata[3].nx = outvdata[4].nx = outvdata[5].nx = 0;
	outvdata[0].ny = outvdata[1].ny = outvdata[2].ny = outvdata[3].ny = outvdata[4].ny = outvdata[5].ny = -1;
	outvdata[0].nz = outvdata[1].nz = outvdata[2].nz = outvdata[3].nz = outvdata[4].nz = outvdata[5].nz = 0;

	outvdata[0].u = 0;			outvdata[1].u = 0;			outvdata[2].u = 1;			outvdata[3].u = 1;			outvdata[4].u = 0.5f;		outvdata[5].u = 0.5f;
	outvdata[0].v = 1;			outvdata[1].v = 0;			outvdata[2].v = 0;			outvdata[3].v = 0.5f;		outvdata[4].v = 0.5f;		outvdata[5].v = 1;

	for (int i = 0; i < 6; ++i) {
		outidata[0] = outidata[3]	= i * 4;
		outidata[1]					= i * 4 + 1;
		outidata[2] = outidata[4]	= i * 4 + 2;
		outidata[5]					= i * 4 + 3;

		outidata += 6;
	}

	outidata[0] = outidata[3] = outidata[6] = outidata[9] = 24;
	outidata[1] = 25;
	outidata[2] = outidata[4] = 26;
	outidata[5] = outidata[7] = 27;
	outidata[8] = outidata[10] = 28;
	outidata[11] = 29;

	outidata += 12;

	outidata[0] = outidata[3] = outidata[6] = outidata[9] = 31;
	outidata[1] = 32;
	outidata[2] = outidata[4] = 33;
	outidata[5] = outidata[7] = 34;
	outidata[8] = outidata[10] = 35;
	outidata[11] = 30;
}

void Create2MBox(PositionVertex* outvdata, uint32_t* outidata, float width, float height, float depth)
{
	outvdata[0] = { width * -0.5f, height * -0.5f, depth * -0.5f };
	outvdata[1] = { width * -0.5f, height * -0.5f, depth * 0.5f };
	outvdata[2] = { width * -0.5f, height * 0.5f, depth * -0.5f };
	outvdata[3] = { width * -0.5f, height * 0.5f, depth * 0.5f };

	outvdata[4] = { width * 0.5f, height * -0.5f, depth * -0.5f };
	outvdata[5] = { width * 0.5f, height * -0.5f, depth * 0.5f };
	outvdata[6] = { width * 0.5f, height * 0.5f, depth * -0.5f };
	outvdata[7] = { width * 0.5f, height * 0.5f, depth * 0.5f };

	outidata[0] = outidata[3] = outidata[11] = outidata[31]						= 0;
	outidata[1] = outidata[8] = outidata[10] = outidata[24] = outidata[27]		= 2;
	outidata[2] = outidata[4] = outidata[13] = outidata[29]						= 6;
	outidata[5] = outidata[12] = outidata[15] = outidata[32] = outidata[34]		= 4;
	outidata[6] = outidata[9] = outidata[23] = outidata[30] = outidata[33]		= 1;
	outidata[7] = outidata[20] = outidata[22] = outidata[25]					= 3;
	outidata[14] = outidata[16] = outidata[19] = outidata[26] = outidata[28]	= 7;
	outidata[17] = outidata[18] = outidata[21] = outidata[35]					= 5;
}

void Create2MSphere(PositionVertex* outvdata, uint32_t* outidata, float radius, uint16_t vsegments, uint16_t hsegments)
{
	assert(radius > 0.0f);

	vsegments = std::max<uint16_t>(vsegments, 4);
	hsegments = std::max<uint16_t>(hsegments, 4);

	float theta, phi;

	// body vertices
	for (int j = 1; j < hsegments - 1; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			theta = ((float)j / (float)(hsegments - 1)) * Math::PI;
			phi = ((float)i / (float)vsegments) * Math::TWO_PI;

			outvdata->x = sinf(theta) * cosf(phi) * radius;
			outvdata->y = cosf(theta) * radius;
			outvdata->z = -sinf(theta) * sinf(phi) * radius;

			++outvdata;
		}
	}

	// top vertex
	outvdata->x = outvdata->z = 0;
	outvdata->y = radius;

	++outvdata;

	// bottom vertex
	outvdata->x = outvdata->z = 0;
	outvdata->y = -radius;

	// indices
	for (int j = 0; j < hsegments - 3; ++j) {
		for (int i = 0; i < vsegments; ++i) {
			outidata[0] = j * vsegments + i;
			outidata[1] = (j + 1) * vsegments + (i + 1) % vsegments;
			outidata[2] = j * vsegments + (i + 1) % vsegments;

			outidata[3] = j * vsegments + i;
			outidata[4] = (j + 1) * vsegments + i;
			outidata[5] = (j + 1) * vsegments + (i + 1) % vsegments;

			outidata += 6;
		}
	}

	for (int i = 0; i < vsegments; ++i) {
		// top
		outidata[0] = (hsegments - 2) * vsegments;
		outidata[1] = i;
		outidata[2] = (i + 1) % vsegments;

		// bottom
		outidata[3] = (hsegments - 2) * vsegments + 1;
		outidata[4] = (hsegments - 3) * vsegments + (i + 1) % vsegments;
		outidata[5] = (hsegments - 3) * vsegments + i;

		outidata += 6;
	}
}

void Create2MLShape(PositionVertex* outvdata, uint32_t* outidata, float basewidth, float innerwidth, float height, float basedepth, float innerdepth)
{
	// half sizes
	float hbwidth = 0.5f * basewidth;
	float hbdepth = 0.5f * basedepth;
	float hheight = 0.5f * height;

	// relative origins
	float rzorig = hbdepth - innerdepth;
	float rxorig = hbwidth - innerwidth;

	outvdata[0].x = -hbwidth;	outvdata[1].x = -hbwidth;	outvdata[2].x = -hbwidth;	outvdata[3].x = -hbwidth;
	outvdata[0].y = -hheight;	outvdata[1].y = -hheight;	outvdata[2].y = hheight;	outvdata[3].y = hheight;
	outvdata[0].z = -hbdepth;	outvdata[1].z = hbdepth;	outvdata[2].z = -hbdepth;	outvdata[3].z = hbdepth;

	outvdata[4].x = hbwidth;	outvdata[5].x = hbwidth;	outvdata[6].x = hbwidth;	outvdata[7].x = hbwidth;
	outvdata[4].y = -hheight;	outvdata[5].y = -hheight;	outvdata[6].y = hheight;	outvdata[7].y = hheight;
	outvdata[4].z = -hbdepth;	outvdata[5].z = rzorig;		outvdata[6].z = -hbdepth;	outvdata[7].z = rzorig;

	outvdata[8].x = rxorig;		outvdata[9].x = rxorig;		outvdata[10].x = rxorig;	outvdata[11].x = rxorig;
	outvdata[8].y = -hheight;	outvdata[9].y = -hheight;	outvdata[10].y = hheight;	outvdata[11].y = hheight;
	outvdata[8].z = rzorig;		outvdata[9].z = hbdepth;	outvdata[10].z = rzorig;	outvdata[11].z = hbdepth;

	outidata[0] = outidata[3] = outidata[11] = outidata[48] = outidata[51] = outidata[54] = outidata[57]	= 0;
	outidata[1] = outidata[8] = outidata[10] = outidata[36] = outidata[39] = outidata[42] = outidata[45]	= 2;
	outidata[2] = outidata[4] = outidata[13] = outidata[47]													= 6;
	outidata[5] = outidata[12] = outidata[15] = outidata[49]												= 4;
	outidata[6] = outidata[9] = outidata[35] = outidata[59]													= 1;
	outidata[7] = outidata[32] = outidata[34] = outidata[37]												= 3;
	outidata[14] = outidata[16] = outidata[19] = outidata[44] = outidata[46]								= 7;
	outidata[17] = outidata[18] = outidata[21] = outidata[50] = outidata[52]								= 5;
	outidata[20] = outidata[22] = outidata[25] = outidata[41] = outidata[43]								= 10;
	outidata[23] = outidata[24] = outidata[27] = outidata[53] = outidata[55]								= 8;
	outidata[26] = outidata[28] = outidata[31] = outidata[38] = outidata[40]								= 11;
	outidata[29] = outidata[30] = outidata[33] = outidata[56] = outidata[58]								= 9;
}

void GenerateEdges(EdgeSet& out, const PositionVertex* vertices, const uint32_t* indices, uint32_t numindices)
{
	Math::Vector3	a, b, n;
	Edge			edge;
	size_t			index;
	uint32_t		i1, i2, i3;

	out.Clear();
	out.Reserve(32);

	// find edges and first triangle
	for (uint32_t i = 0; i < numindices; i += 3) {
		if (out.Capacity() <= out.Size())
			out.Reserve(out.Size() * 2);

		i1 = indices[i + 0];
		i2 = indices[i + 1];
		i3 = indices[i + 2];

		const PositionVertex& p1 = vertices[i1];
		const PositionVertex& p2 = vertices[i2];
		const PositionVertex& p3 = vertices[i3];

		a.x = p2.x - p1.x;
		a.y = p2.y - p1.y;
		a.z = p2.z - p1.z;

		b.x = p3.x - p1.x;
		b.y = p3.y - p1.y;
		b.z = p3.z - p1.z;

		Math::Vec3Cross(n, a, b);
		Math::Vec3Normalize(n, n);

		if (i1 < i2) {
			edge.i1 = i1;
			edge.i2 = i2;
			edge.v1 = (const Math::Vector3&)p1;
			edge.v2 = (const Math::Vector3&)p2;
			edge.n1 = n;

			if (!out.Insert(edge).second) {
				// crack in mesh (first triangle)
				assert(false);
				return;
			}
		}

		if (i2 < i3) {
			edge.i1 = i2;
			edge.i2 = i3;
			edge.v1 = (const Math::Vector3&)p2;
			edge.v2 = (const Math::Vector3&)p3;
			edge.n1 = n;

			if (!out.Insert(edge).second) {
				// crack in mesh (first triangle)
				assert(false);
				return;
			}
		}

		if (i3 < i1) {
			edge.i1 = i3;
			edge.i2 = i1;
			edge.v1 = (const Math::Vector3&)p3;
			edge.v2 = (const Math::Vector3&)p1;
			edge.n1 = n;

			if (!out.Insert(edge).second) {
				// crack in mesh (first triangle)
				assert(false);
				return;
			}
		}
	}

	// find second triangle
	for (uint32_t i = 0; i < numindices; i += 3) {
		i1 = indices[i + 0];
		i2 = indices[i + 1];
		i3 = indices[i + 2];

		const PositionVertex& p1 = vertices[i1];
		const PositionVertex& p2 = vertices[i2];
		const PositionVertex& p3 = vertices[i3];

		a.x = p2.x - p1.x;
		a.y = p2.y - p1.y;
		a.z = p2.z - p1.z;

		b.x = p3.x - p1.x;
		b.y = p3.y - p1.y;
		b.z = p3.z - p1.z;

		Math::Vec3Cross(n, a, b);
		Math::Vec3Normalize(n, n);

		if (i1 > i2) {
			edge.i1 = i2;
			edge.i2 = i1;

			index = out.Find(edge);

			if (index == EdgeSet::npos) {
				// lone triangle
				continue;
			}

			if (out[index].other == UINT32_MAX) {
				out[index].other = i / 3;
				out[index].n2 = n;
			} else {
				// crack in mesh (second triangle)
				assert(false);
				return;
			}
		}

		if (i2 > i3) {
			edge.i1 = i3;
			edge.i2 = i2;

			index = out.Find(edge);

			if (index == EdgeSet::npos) {
				// lone triangle
				continue;
			}

			if (out[index].other == UINT32_MAX) {
				out[index].other = i / 3;
				out[index].n2 = n;
			} else {
				// crack in mesh (second triangle)
				assert(false);
				return;
			}
		}

		if (i3 > i1) {
			edge.i1 = i1;
			edge.i2 = i3;

			index = out.Find(edge);

			if (index == EdgeSet::npos) {
				// lone triangle
				continue;
			}

			if (out[index].other == UINT32_MAX) {
				out[index].other = i / 3;
				out[index].n2 = n;
			} else {
				// crack in mesh (second triangle)
				assert(false);
				return;
			}
		}
	}
}

void GenerateGSAdjacency(uint32_t* outidata, const uint32_t* idata, uint32_t numindices)
{
	// NOTE: assumes that input is 2-manifold triangle list
	assert((numindices % 3) == 0);
	assert(outidata != idata);

	uint32_t numtriangles = numindices / 3;

	for (uint32_t i = 0; i < numtriangles; ++i) {
		// indices of base triangle
		uint32_t i1 = idata[i * 3 + 0];	// 0
		uint32_t i2 = idata[i * 3 + 1];	// 2
		uint32_t i3 = idata[i * 3 + 2];	// 4

		uint32_t adj1 = UINT32_MAX;		// 1
		uint32_t adj3 = UINT32_MAX;		// 3
		uint32_t adj5 = UINT32_MAX;		// 5

		// find adjacent indices
		for (uint32_t j = 0; j < numtriangles; ++j) {
			if (i == j)
				continue;

			uint32_t j1 = idata[j * 3 + 0];
			uint32_t j2 = idata[j * 3 + 1];
			uint32_t j3 = idata[j * 3 + 2];

			if (adj1 == UINT32_MAX)
				adj1 = FindAdjacentIndex(j1, j2, j3, i1, i2);

			if (adj3 == UINT32_MAX)
				adj3 = FindAdjacentIndex(j1, j2, j3, i2, i3);

			if (adj5 == UINT32_MAX)
				adj5 = FindAdjacentIndex(j1, j2, j3, i3, i1);
			
			if (adj1 < UINT32_MAX && adj3 < UINT32_MAX && adj5 < UINT32_MAX)
				break;
		}

		// write adjacency data
		outidata[0] = i1;
		outidata[1] = adj1;
		outidata[2] = i2;
		outidata[3] = adj3;
		outidata[4] = i3;
		outidata[5] = adj5;

		outidata += 6;
	}
}

void GenerateTangentFrame(TBNVertex* outvdata, const CommonVertex* vdata, uint32_t numvertices, const uint32_t* indices, uint32_t numindices)
{
	// NOTE: assumes triangle list
	assert((numindices % 3) == 0);

	for (uint32_t j = 0; j < numvertices; ++j) {
		const CommonVertex& oldvert = vdata[j];
		TBNVertex& newvert = outvdata[j];

		newvert.x = oldvert.x;
		newvert.y = oldvert.y;
		newvert.z = oldvert.z;

		newvert.nx = oldvert.nx;
		newvert.ny = oldvert.ny;
		newvert.nz = oldvert.nz;
			
		newvert.u = oldvert.u;
		newvert.v = oldvert.v;

		newvert.tx = newvert.ty = newvert.tz = 0;
		newvert.bx = newvert.by = newvert.bz = 0;
	}

	for (uint32_t j = 0; j < numindices; j += 3) {
		uint32_t i1 = *((uint32_t*)indices + j + 0);
		uint32_t i2 = *((uint32_t*)indices + j + 1);
		uint32_t i3 = *((uint32_t*)indices + j + 2);

		AccumulateTangentFrame(outvdata, i1, i2, i3);
	}

	for (uint32_t j = 0; j < numvertices; ++j) {
		OrthogonalizeTangentFrame(outvdata[j]);
	}
}

void NumVerticesIndicesSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments)
{
	outnumverts = (hsegments - 2) * vsegments + 2 * (vsegments - 1);
	outnuminds = (hsegments - 2) * (vsegments - 1) * 6;
}

void NumVerticesIndicesCapsule(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments)
{
	outnumverts = (hsegments - 2) * vsegments + 2 * (vsegments - 1);
	outnuminds = ((hsegments - 4) * (vsegments - 1) + 2 * (vsegments - 1)) * 6;
}

void NumVerticesIndices2MSphere(uint32_t& outnumverts, uint32_t& outnuminds, uint16_t vsegments, uint16_t hsegments)
{
	outnumverts = (hsegments - 2) * vsegments + 2;
	outnuminds = (hsegments - 2) * vsegments * 6;
}

}
