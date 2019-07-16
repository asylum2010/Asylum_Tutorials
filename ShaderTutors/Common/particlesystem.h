
#ifndef _PARTICLESYSTEM_H_
#define _PARTICLESYSTEM_H_

#include <vector>

#include "3Dmath.h"
#include "orderedmultiarray.hpp"

// --- Interfaces -------------------------------------------------------------

class IParticleStorage
{
public:
	virtual ~IParticleStorage();

	virtual bool Initialize(size_t count, size_t stride) = 0;
	virtual void* LockVertexBuffer(uint32_t offset, uint32_t size) = 0;
	virtual void UnlockVertexBuffer() = 0;

	virtual size_t GetVertexStride() const = 0;
	virtual size_t GetNumVertices() const = 0;
};

// --- Classes ----------------------------------------------------------------

class ParticleSystem
{
	struct Particle
	{
		Math::Vector3 position;
		Math::Vector3 velocity;
		Math::Color color;
		uint32_t life;
	};

	struct ParticleCompare
	{
		Math::Matrix transform;

		bool operator ()(const Particle& a, const Particle& b) const {
			Math::Vector3 p1, p2;

			Math::Vec3TransformCoord(p1, a.position, transform);
			Math::Vec3TransformCoord(p2, b.position, transform);

			return (p1.z > p2.z);
		}
	};

	typedef std::vector<Particle> ParticleList;
	typedef OrderedMultiArray<Particle, ParticleCompare> ParticleTable;

private:
	ParticleList		particles;
	ParticleTable		orderedparticles;
	IParticleStorage*	storageimpl;	// GPU-side storage
	size_t				maxcount;

public:
	Math::Vector3		EmitterPosition;
	Math::Vector3		Force;			// net force (gravity, uplifting, etc.)
	float				EmitterRadius;
	float				ParticleSize;
	bool				NeedsSorting;

	ParticleSystem();
	~ParticleSystem();

	bool Initialize(IParticleStorage* impl, size_t maxnumparticles);

	void Update(float dt);
	void Draw(const Math::Matrix& world, const Math::Matrix& view, std::function<void(size_t)> callback);
};

#endif
