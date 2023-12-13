
#ifndef _SHADOWTECHNIQUES_H_
#define _SHADOWTECHNIQUES_H_

#include <functional>
#include <d3dx9.h>

#include "../Common/3Dmath.h"

class ShadowTechnique
{
protected:
	typedef std::function<void (const Math::Matrix&, const Math::Vector2&)> ShadowCallback;

	LPDIRECT3DDEVICE9	device;
	LPDIRECT3DTEXTURE9	shadowmap;
	LPD3DXEFFECT		effect;

public:
	static UINT ShadowMapSize;

	ShadowTechnique();
	virtual ~ShadowTechnique();

	virtual bool Initialize(LPDIRECT3DDEVICE9 device) = 0;
	virtual Math::Vector2 RenderShadowMap(const Math::Matrix& lightview, const Math::AABox& box, ShadowCallback callback) = 0;

	inline LPD3DXEFFECT GetEffect()				{ return effect; }
	inline LPDIRECT3DTEXTURE9 GetShadowMap()	{ return shadowmap; }
};

class PercentageCloser : public ShadowTechnique
{
private:
	LPDIRECT3DTEXTURE9 noise;

public:
	PercentageCloser();
	~PercentageCloser();

	bool Initialize(LPDIRECT3DDEVICE9 device) override;
	Math::Vector2 RenderShadowMap(const Math::Matrix& lightview, const Math::AABox& box, ShadowCallback callback) override;

	inline LPDIRECT3DTEXTURE9 GetNoiseTexture()	{ return noise; }
};

class VarianceShadow : public ShadowTechnique
{
protected:
	LPDIRECT3DTEXTURE9	othershadowmap;
	LPD3DXEFFECT		blureffect;

public:
	VarianceShadow();
	~VarianceShadow();

	bool Initialize(LPDIRECT3DDEVICE9 device) override;
	Math::Vector2 RenderShadowMap(const Math::Matrix& lightview, const Math::AABox& box, ShadowCallback callback) override;
};

class ExpVarianceShadow : public VarianceShadow
{
public:
	ExpVarianceShadow();
	~ExpVarianceShadow();

	bool Initialize(LPDIRECT3DDEVICE9 device) override;
	Math::Vector2 RenderShadowMap(const Math::Matrix& lightview, const Math::AABox& box, ShadowCallback callback) override;
};

#endif
