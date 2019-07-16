
#include "particlesystem.h"
#include "geometryutils.h"

IParticleStorage::~IParticleStorage()
{
}

// --- ParticleSystem impl ----------------------------------------------------

ParticleSystem::ParticleSystem()
{
	storageimpl		= nullptr;
	maxcount		= 0;

	Force			= Math::Vector3(0, -9.81f, 0);
	EmitterRadius	= 0.5f;
	ParticleSize	= 0.5f;
	NeedsSorting	= true;
}

ParticleSystem::~ParticleSystem()
{
	delete storageimpl;
}

bool ParticleSystem::Initialize(IParticleStorage* impl, size_t maxnumparticles)
{
	if (!impl->Initialize(maxnumparticles, sizeof(GeometryUtils::BillboardVertex)))
		return false;

	maxcount = maxnumparticles;
	storageimpl = impl;

	particles.reserve(maxnumparticles);
	orderedparticles.Reserve(maxnumparticles);

	return true;
}

void ParticleSystem::Update(float dt)
{
	size_t i = 0;

	while (i < particles.size()) {
		Particle& p = particles[i];
		
		if (p.life < 30)
			p.color.a = (float)p.life / 30.0f;

		if (p.life == 0) {
			// died
			std::swap(particles[i], particles[particles.size() - 1]);
			particles.pop_back();
		} else {
			p.position += p.velocity;
			p.velocity += Force * dt;

			--p.life;
			++i;
		}
	}

	while (particles.size() < maxcount) {
		float u = (float)(rand() % 100) / 100.0f;
		float v = (float)(rand() % 100) / 100.0f;

		u = 2 * u * Math::PI;
		v = (v - 0.5f) * Math::PI;

		Particle p;
		Math::Vector3 du, dv, n;

		p.position.x = EmitterRadius * sinf(u) * cosf(v);
		p.position.y = EmitterRadius * cosf(u) * cosf(v);
		p.position.z = EmitterRadius * sinf(v);

		du.x = p.position.y;
		du.y = -p.position.x;
		du.z = p.position.z;

		dv.x = -EmitterRadius * sinf(u) * sinf(v);
		dv.y = -EmitterRadius * cosf(u) * sinf(v);
		dv.z = EmitterRadius * cosf(v);

		Math::Vec3Cross(n, du, dv);
		Math::Vec3Normalize(n, n);

		p.velocity = n * 0.02f;			// InitialVelocity
		p.life = (rand() % 50) + 10;	// InitialLife
		p.color = Math::Color(1, 1, 1, 1);

		particles.push_back(p);
	}
}

void ParticleSystem::Draw(const Math::Matrix& world, const Math::Matrix& view, std::function<void(size_t)> callback)
{
	Math::Vector3	worldpos;
	Math::Vector3	right, up;
	Math::Vector3	tmp1, tmp2;
	size_t			count		= particles.size();
	float			halfsize	= ParticleSize * 0.5f;

	// NOTE: matrix is right-handed
	right.x = -view._11;
	right.y = -view._21;
	right.z = -view._31;

	up.x = view._12;
	up.y = view._22;
	up.z = view._32;

	GeometryUtils::BillboardVertex*	vdata = (GeometryUtils::BillboardVertex*)storageimpl->LockVertexBuffer(0, 0);

	if (vdata == nullptr)
		return;

	Math::MatrixMultiply(orderedparticles.comp.transform, world, view);

	tmp1 = (right - up) * halfsize;
	tmp2 = (right + up) * halfsize;
	
	if (NeedsSorting) {
		orderedparticles.Clear();

		for (size_t i = 0; i < particles.size(); ++i)
			orderedparticles.Insert(particles[i]);

		count = orderedparticles.Size();
	}

	for (size_t i = 0; i < count; ++i) {
		GeometryUtils::BillboardVertex* v1 = (vdata + i * 6 + 0);
		GeometryUtils::BillboardVertex* v2 = (vdata + i * 6 + 1);
		GeometryUtils::BillboardVertex* v3 = (vdata + i * 6 + 2);
		GeometryUtils::BillboardVertex* v4 = (vdata + i * 6 + 3);
		GeometryUtils::BillboardVertex* v5 = (vdata + i * 6 + 4);
		GeometryUtils::BillboardVertex* v6 = (vdata + i * 6 + 5);

		const Particle& p = (NeedsSorting ? orderedparticles[i] : particles[i]);
		Math::Vec3TransformCoord(worldpos, p.position, world);
			
		// top left
		v1->x = worldpos.x - tmp1.x;
		v1->y = worldpos.y - tmp1.y;
		v1->z = worldpos.z - tmp1.z;

		v1->u = 0;
		v1->v = 0;
		v1->color = p.color;

		// top right
		v2->x = v5->x = worldpos.x + tmp2.x;
		v2->y = v5->y = worldpos.y + tmp2.y;
		v2->z = v5->z = worldpos.z + tmp2.z;

		v2->u = v5->u = 1;
		v2->v = v5->v = 0;
		v2->color = v5->color = p.color;

		// bottom left
		v3->x = v4->x = worldpos.x - tmp2.x;
		v3->y = v4->y = worldpos.y - tmp2.y;
		v3->z = v4->z = worldpos.z - tmp2.z;

		v3->u = v4->u = 0;
		v3->v = v4->v = 1;
		v3->color = v4->color = p.color;

		// bottomright
		v6->x = worldpos.x + tmp1.x;
		v6->y = worldpos.y + tmp1.y;
		v6->z = worldpos.z + tmp1.z;

		v6->u = 1;
		v6->v = 1;
		v6->color = p.color;
	}

	storageimpl->UnlockVertexBuffer();

	// render
	callback(count);
}
