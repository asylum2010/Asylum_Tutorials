
#include "lightning.h"
#include "geometryutils.h"

#define METRIC_CORRECTION	8e-2f

LightningParameters CoilLightning = {
	Math::Vector2(0.45f, 0.55f),
	METRIC_CORRECTION * Math::Vector2(-3.0f, 3.0f),	
	METRIC_CORRECTION * Math::Vector2(-3.0f, 3.0f),
	0.5f,

	Math::Vector2(0.45f, 0.55f),
	METRIC_CORRECTION * Math::Vector2(-1.0f, 1.0f),
	METRIC_CORRECTION * Math::Vector2(-1.0f, 1.0f),
	0.5f,

	METRIC_CORRECTION * Math::Vector2(-1.0f, 1.0f),
	METRIC_CORRECTION * Math::Vector2(-1.0f, 1.0f),
	METRIC_CORRECTION * Math::Vector2(0.0f, 1.0f),
	0.5f,
	METRIC_CORRECTION * Math::Vector2(1.0f, 2.0f),
	0.01f
};

// --- Linear Congruential Generator ------------------------------------------

static int random_x;

#define RANDOM_IA	16807
#define RANDOM_IM	2147483647
#define RANDOM_AM	(1.0f / RANDOM_IM)
#define RANDOM_IQ	127773
#define RANDOM_IR	2836
#define RANDOM_MASK	123459876

static float Random()
{
	// NOTE: http://www.library.cornell.edu/nr/bookcpdf/c7-1.pdf

	int k;
	float ans;
	
	random_x ^= RANDOM_MASK;
	k = random_x / RANDOM_IQ;
	random_x = RANDOM_IA * (random_x - k * RANDOM_IQ ) - RANDOM_IR * k;
	
	if (random_x < 0) 
		random_x += RANDOM_IM;
	
	ans = RANDOM_AM * random_x;
	random_x ^= RANDOM_MASK;
	
	return ans;
}

static void RandomSeed(int value)
{
	random_x = value;
	Random();
}

static float Random(float low, float high)
{
	float v = Random();
	return low * (1.0f - v) + high * v;
};

// --- Lightning impl ---------------------------------------------------------

Lightning::Lightning(const LightningParameters& params)
{
	parameters = params;
	storageimpl = nullptr;

	AnimationSpeed = 15;
}

Lightning::~Lightning()
{
	delete storageimpl;
}

size_t Lightning::CalculateNumVertices(size_t segments, int levels, int mask)
{
	size_t ret = 1;

	for (int i = 0; i < levels; ++i) {
		if (mask & (1 << i))
			ret *= 3;
		else
			ret *= 2;
	}

	return segments * 2 * ret;
}

bool Lightning::Initialize(IParticleStorage* impl, const SeedList& seeds, int subdivisionlevels, int subdivisionmask)
{
	Math::Vector3 jitter;
	size_t segments = seeds.size() / 2;
	size_t maxnumvertices = CalculateNumVertices(segments, subdivisionlevels, subdivisionmask);

	if (!impl->Initialize(maxnumvertices * 18, sizeof(GeometryUtils::BillboardVertex)))
		return false;

	storageimpl	= impl;
	levels		= subdivisionlevels;
	mask		= subdivisionmask;

	seedpoints.reserve(seeds.size());

	for (const Math::Vector3& p : seeds) {
		jitter.x = 1e-2f * (Math::RandomFloat() * 2.0f - 1.0f);
		jitter.z = 1e-2f * (Math::RandomFloat() * 2.0f - 1.0f);

		seedpoints.push_back(p + jitter);
	}

	subdivided0.reserve(maxnumvertices);
	subdivided1.reserve(maxnumvertices);

	return true;
}

void Lightning::Reset()
{
	seedpoints.clear();
}

void Lightning::Subdivide(float time)
{
	subdivided0.clear();
	subdivided1.clear();

	// fill first buffer
	for (const Math::Vector3& p : seedpoints) {
		subdivided0.push_back(p);
	}

	// subdivide
	SeedList* buffers[] = { &subdivided0, &subdivided1 };

	Math::Vector3 forward, right, center;
	Math::Vector3 up(0, 1, 0);
	Math::Vector3 splitpos;
	float decay;
	int current = 0;

	for (int i = 0; i < levels; ++i) {
		const std::vector<Math::Vector3>& source = *buffers[current];
		std::vector<Math::Vector3>& target = *buffers[1 - current];

		target.clear();

		for (size_t j = 0; j < source.size(); j += 2) {
			const Math::Vector3& p1 = source[j];
			const Math::Vector3& p2 = source[j + 1];
			
			RandomSeed((int)(j / 2 + 1.0f + time * AnimationSpeed));

			Math::Vec3Normalize(forward, p2 - p1);

			Math::Vec3Cross(right, forward, up);
			Math::Vec3Normalize(right, right);

			Math::Vec3Cross(up, right, forward);
			Math::Vec3Normalize(up, up);

			// TODO: up vector and level should be calculated and stored

			if (mask & (1 << i)) {
				// fork
				Math::Vec3Lerp(splitpos, p1, p2, Random(parameters.ForkFraction.x, parameters.ForkFraction.y));

				decay = expf(-parameters.ForkZigZagDeviationDecay * i);

				Math::Vector2 delta = decay * Math::Vector2(
					Random(parameters.ForkZigZagDeviationRight.x, parameters.ForkZigZagDeviationRight.y),
					Random(parameters.ForkZigZagDeviationUp.x, parameters.ForkZigZagDeviationUp.y));

				splitpos += right * delta.x;
				splitpos += up * delta.y;

				target.push_back(p1);
				target.push_back(splitpos);
				target.push_back(splitpos);
				target.push_back(p2);

				decay = expf(-parameters.ForkDeviationDecay * i);

				Math::Vector3 forkdelta = decay * Math::Vector3(
					Random(parameters.ForkDeviationRight.x,parameters.ForkDeviationRight.y),
					Random(parameters.ForkDeviationUp.x, parameters.ForkDeviationUp.y),
					Random(parameters.ForkDeviationForward.x, parameters.ForkDeviationForward.y));

				decay = expf(-parameters.ForkLengthDecay * i);
				
				Math::Vector3 forkdir;
				Math::Vec3Normalize(forkdir, forkdelta.x * right + forkdelta.y * up + forkdelta.z * forward);
				
				float forklength = decay * Random(parameters.ForkLength.x, parameters.ForkLength.y);
				Math::Vector3 forkpos = splitpos + forklength * forkdir;
	
				target.push_back(splitpos);
				target.push_back(forkpos);
			} else {
				// zig-zag
				Math::Vec3Lerp(splitpos, p1, p2, Random(parameters.ZigZagFraction.x, parameters.ZigZagFraction.y));

				float decay = expf(-parameters.ZigZagDeviationDecay * i);
				Math::Vector2 delta = decay * Math::Vector2(
					Random(parameters.ZigZagDeviationRight.x, parameters.ZigZagDeviationRight.y),
					Random(parameters.ZigZagDeviationUp.x, parameters.ZigZagDeviationUp.y));

				splitpos += right * delta.x;
				splitpos += up * delta.y;

				target.push_back(p1);
				target.push_back(splitpos);
				target.push_back(splitpos);
				target.push_back(p2);
			}
		}

		current = 1 - current;
	}
}

void Lightning::Generate(float thickness, const Math::Matrix& view)
{
	// NOTE: can't use triangle strip because there's no primitive restart

	Math::Vector3	viewpos1, viewpos2;
	Math::Vector3	forward, right;
	float			halfthickness = thickness * 0.5f;

	// use texcoord for gradient
	GeometryUtils::BillboardVertex*	vdata = (GeometryUtils::BillboardVertex*)storageimpl->LockVertexBuffer(0, 0);
	const auto& vertices = GetSubdivision();

	if (vdata == nullptr)
		return;

	for (size_t i = 0; i < vertices.size(); i += 2) {
		const Math::Vector3& start = vertices[i];
		const Math::Vector3& end = vertices[i + 1];

		GeometryUtils::BillboardVertex* t1 = (vdata + 0);
		GeometryUtils::BillboardVertex* t2 = (vdata + 1);
		GeometryUtils::BillboardVertex* t3 = (vdata + 2);
		GeometryUtils::BillboardVertex* t4 = (vdata + 3);
		GeometryUtils::BillboardVertex* t5 = (vdata + 4);
		GeometryUtils::BillboardVertex* t6 = (vdata + 5);

		GeometryUtils::BillboardVertex* v1 = (vdata + 6);
		GeometryUtils::BillboardVertex* v2 = (vdata + 7);
		GeometryUtils::BillboardVertex* v3 = (vdata + 8);
		GeometryUtils::BillboardVertex* v4 = (vdata + 9);
		GeometryUtils::BillboardVertex* v5 = (vdata + 10);
		GeometryUtils::BillboardVertex* v6 = (vdata + 11);

		GeometryUtils::BillboardVertex* b1 = (vdata + 12);
		GeometryUtils::BillboardVertex* b2 = (vdata + 13);
		GeometryUtils::BillboardVertex* b3 = (vdata + 14);
		GeometryUtils::BillboardVertex* b4 = (vdata + 15);
		GeometryUtils::BillboardVertex* b5 = (vdata + 16);
		GeometryUtils::BillboardVertex* b6 = (vdata + 17);

		Math::Vec3TransformCoord(viewpos1, start, view);
		Math::Vec3TransformCoord(viewpos2, end, view);

		Math::Vec3Subtract(forward, viewpos2, viewpos1);
		Math::Vec3Cross(right, forward, Math::Vector3(0, 0, 1));

		Math::Vec3Normalize(forward, forward);
		Math::Vec3Normalize(right, right);

		// top cap
		t1->x = viewpos1.x - right.x * halfthickness - forward.x * halfthickness;
		t1->y = viewpos1.y - right.y * halfthickness - forward.y * halfthickness;
		t1->z = viewpos1.z - right.z * halfthickness - forward.z * halfthickness;
		t1->u = -1;
		t1->v = 1;

		t2->x = t5->x = viewpos1.x + right.x * halfthickness - forward.x * halfthickness;
		t2->y = t5->y = viewpos1.y + right.y * halfthickness - forward.y * halfthickness;
		t2->z = t5->z = viewpos1.z + right.z * halfthickness - forward.z * halfthickness;
		t2->u = t5->u = 1;
		t2->v = t5->v = 1;

		t3->x = t4->x = viewpos1.x - right.x * halfthickness;
		t3->y = t4->y = viewpos1.y - right.y * halfthickness;
		t3->z = t4->z = viewpos1.z - right.z * halfthickness;
		t3->u = t4->u = -1;
		t3->v = t4->v = 0;

		t6->x = viewpos1.x + right.x * halfthickness;
		t6->y = viewpos1.y + right.y * halfthickness;
		t6->z = viewpos1.z + right.z * halfthickness;
		t6->u = 1;
		t6->v = 0;

		// inner quad
		v1->x = viewpos1.x - right.x * halfthickness;
		v1->y = viewpos1.y - right.y * halfthickness;
		v1->z = viewpos1.z - right.z * halfthickness;
		v1->u = -1;
		v1->v = 0;

		v2->x = v5->x = viewpos1.x + right.x * halfthickness;
		v2->y = v5->y = viewpos1.y + right.y * halfthickness;
		v2->z = v5->z = viewpos1.z + right.z * halfthickness;
		v2->u = v5->u = 1;
		v2->v = v5->v = 0;

		v3->x = v4->x = viewpos2.x - right.x * halfthickness;
		v3->y = v4->y = viewpos2.y - right.y * halfthickness;
		v3->z = v4->z = viewpos2.z - right.z * halfthickness;
		v3->u = v4->u = -1;
		v3->v = v4->v = 0;

		v6->x = viewpos2.x + right.x * halfthickness;
		v6->y = viewpos2.y + right.y * halfthickness;
		v6->z = viewpos2.z + right.z * halfthickness;
		v6->u = 1;
		v6->v = 0;

		// bottom cap
		b1->x = viewpos2.x - right.x * halfthickness;
		b1->y = viewpos2.y - right.y * halfthickness;
		b1->z = viewpos2.z - right.z * halfthickness;
		b1->u = -1;
		b1->v = 0;

		b2->x = b5->x = viewpos2.x + right.x * halfthickness;
		b2->y = b5->y = viewpos2.y + right.y * halfthickness;
		b2->z = b5->z = viewpos2.z + right.z * halfthickness;
		b2->u = b5->u = 1;
		b2->v = b5->v = 0;

		b3->x = b4->x = viewpos2.x - right.x * halfthickness + forward.x * halfthickness;
		b3->y = b4->y = viewpos2.y - right.y * halfthickness + forward.y * halfthickness;
		b3->z = b4->z = viewpos2.z - right.z * halfthickness + forward.z * halfthickness;
		b3->u = b4->u = -1;
		b3->v = b4->v = 1;

		b6->x = viewpos2.x + right.x * halfthickness + forward.x * halfthickness;
		b6->y = viewpos2.y + right.y * halfthickness + forward.y * halfthickness;
		b6->z = viewpos2.z + right.z * halfthickness + forward.z * halfthickness;
		b6->u = 1;
		b6->v = 1;

		vdata += 18;
	}

	storageimpl->UnlockVertexBuffer();
}
