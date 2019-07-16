
#ifndef _LIGHTNING_H_
#define _LIGHTNING_H_

#include <vector>
#include "particlesystem.h"

struct LightningParameters
{
	// for zig-zagging
	Math::Vector2	ZigZagFraction;
	Math::Vector2	ZigZagDeviationRight;		
	Math::Vector2	ZigZagDeviationUp;
	float			ZigZagDeviationDecay;
		
	// for forking
	Math::Vector2	ForkFraction;
	Math::Vector2	ForkZigZagDeviationRight;
	Math::Vector2	ForkZigZagDeviationUp;
	float			ForkZigZagDeviationDecay;
		
	Math::Vector2	ForkDeviationRight;
	Math::Vector2	ForkDeviationUp;
	Math::Vector2	ForkDeviationForward;
	float			ForkDeviationDecay;
	Math::Vector2	ForkLength;
	float			ForkLengthDecay;
};

extern LightningParameters CoilLightning;

// --- Classes ----------------------------------------------------------------

class Lightning
{
	typedef std::vector<Math::Vector3> SeedList;

private:
	LightningParameters	parameters;
	SeedList			seedpoints;
	SeedList			subdivided0;
	SeedList			subdivided1;
	IParticleStorage*	storageimpl;	// GPU-side storage
	int					levels;			// how many times to subdivide
	int					mask;			// to know when to fork

	size_t CalculateNumVertices(size_t segments, int levels, int mask);

public:
	float AnimationSpeed;

	Lightning(const LightningParameters& params);
	~Lightning();

	bool Initialize(IParticleStorage* impl, const SeedList& seeds, int subdivisionlevels, int subdivisionmask);

	void Reset();
	void Subdivide(float time);
	void Generate(float thickness, const Math::Matrix& view);

	inline const std::vector<Math::Vector3>& GetSubdivision() const {
		return ((levels % 2) ? subdivided1 : subdivided0);
	}
};

#endif
