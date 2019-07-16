
#include "shadowtechniques.h"
#include "../Common/dx9ext.h"

UINT ShadowTechnique::ShadowMapSize = 512;

ShadowTechnique::ShadowTechnique()
{
	device = nullptr;
	effect = nullptr;
	shadowmap = nullptr;
}

ShadowTechnique::~ShadowTechnique()
{
	if (effect) {
		effect->Release();
		effect = nullptr;
	}

	if (shadowmap) {
		shadowmap->Release();
		shadowmap = nullptr;
	}
}

// --- PercentageCloser impl --------------------------------------------------

PercentageCloser::PercentageCloser()
{
	noise = nullptr;
}

PercentageCloser::~PercentageCloser()
{
	if (noise)
		noise->Release();
}

bool PercentageCloser::Initialize(LPDIRECT3DDEVICE9 device)
{
	this->device = device;

	if (FAILED(device->CreateTexture(ShadowMapSize, ShadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &shadowmap, NULL)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/pcfnoise.bmp", &noise)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/shadow_pcf.fx", &effect)))
		return false;

	effect->SetTechnique("pcf5x5");
	return true;
}

Math::Vector2 PercentageCloser::RenderShadowMap(const Math::Matrix& lightview, const Math::AABox& box, ShadowCallback callback)
{
	Math::Matrix proj, viewproj;
	Math::Vector2 clipplanes;

	Math::FitToBoxOrtho(proj, clipplanes, lightview, box);
	Math::MatrixMultiply(viewproj, lightview, proj);

	LPDIRECT3DSURFACE9 surface = nullptr;
	shadowmap->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, surface);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1, 0);

	D3DXHANDLE oldtech = effect->GetCurrentTechnique();
	effect->SetTechnique("shadowmap");

	effect->SetMatrix("lightViewProj", (D3DXMATRIX*)&viewproj);
	effect->SetVector("clipPlanes", (D3DXVECTOR4*)&clipplanes);
	{
		callback(proj, clipplanes);
	}
	effect->SetTechnique(oldtech);

	surface->Release();
	return clipplanes;
}

// --- VarianceShadow impl ----------------------------------------------------

VarianceShadow::VarianceShadow()
{
	othershadowmap = nullptr;
	blureffect = nullptr;
}

VarianceShadow::~VarianceShadow()
{
	if (othershadowmap)
		othershadowmap->Release();

	if (blureffect)
		blureffect->Release();
}

bool VarianceShadow::Initialize(LPDIRECT3DDEVICE9 device)
{
	this->device = device;

	if (FAILED(device->CreateTexture(ShadowMapSize, ShadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL)))
		return false;

	if (FAILED(device->CreateTexture(ShadowMapSize, ShadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_G32R32F, D3DPOOL_DEFAULT, &othershadowmap, NULL)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/shadow_variance.fx", &effect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/blur.fx", &blureffect)))
		return false;

	effect->SetTechnique("variance");
	blureffect->SetTechnique("boxblur5x5");

	return true;
}

Math::Vector2 VarianceShadow::RenderShadowMap(const Math::Matrix& lightview, const Math::AABox& box, ShadowCallback callback)
{
	D3DVIEWPORT9 oldviewport;
	D3DVIEWPORT9 viewport;

	Math::Matrix proj, viewproj;
	Math::Vector2 clipplanes;

	Math::FitToBoxOrtho(proj, clipplanes, lightview, box);
	Math::MatrixMultiply(viewproj, lightview, proj);

	device->GetViewport(&oldviewport);

	viewport.X = viewport.Y = 0;
	viewport.Width = viewport.Height = ShadowMapSize;
	viewport.MinZ = 0;
	viewport.MaxZ = 1;

	// must render everything (receivers too)
	LPDIRECT3DSURFACE9 surface = nullptr;
	othershadowmap->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, surface);
	device->SetViewport(&viewport);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1, 0);

	D3DXHANDLE oldtech = effect->GetCurrentTechnique();
	effect->SetTechnique("shadowmap");

	effect->SetMatrix("lightViewProj", (D3DXMATRIX*)&viewproj);
	effect->SetVector("clipPlanes", (D3DXVECTOR4*)&clipplanes);
	{
		callback(proj, clipplanes);
	}
	effect->SetTechnique(oldtech);

	surface->Release();

	// now blur it
	D3DXVECTOR4 pixelsize(1.0f / ShadowMapSize, 1.0f / ShadowMapSize, 0, 0);
	D3DXVECTOR4 texelsize = pixelsize;

	shadowmap->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, surface);
	device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);

	blureffect->SetVector("pixelSize", &pixelsize);
	blureffect->SetVector("texelSize", &texelsize);
		
	blureffect->Begin(NULL, 0);
	blureffect->BeginPass(0);
	{
		device->SetTexture(0, othershadowmap);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	blureffect->EndPass();
	blureffect->End();

	device->SetRenderState(D3DRS_ZENABLE, TRUE);
	device->SetViewport(&oldviewport);

	surface->Release();
	return clipplanes;
}

// --- ExpVarianceShadow impl -------------------------------------------------

ExpVarianceShadow::ExpVarianceShadow()
{
	othershadowmap = nullptr;
	blureffect = nullptr;
}

ExpVarianceShadow::~ExpVarianceShadow()
{
}

bool ExpVarianceShadow::Initialize(LPDIRECT3DDEVICE9 device)
{
	this->device = device;

	if (FAILED(device->CreateTexture(ShadowMapSize, ShadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &shadowmap, NULL)))
		return false;

	if (FAILED(device->CreateTexture(ShadowMapSize, ShadowMapSize, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &othershadowmap, NULL)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/shadow_expvariance.fx", &effect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/blur.fx", &blureffect)))
		return false;

	effect->SetTechnique("expvariance");
	blureffect->SetTechnique("boxblur5x5");

	return true;
}

Math::Vector2 ExpVarianceShadow::RenderShadowMap(const Math::Matrix& lightview, const Math::AABox& box, ShadowCallback callback)
{
	D3DVIEWPORT9 oldviewport;
	D3DVIEWPORT9 viewport;

	Math::Matrix proj, viewproj;
	Math::Vector2 clipplanes;

	Math::FitToBoxOrtho(proj, clipplanes, lightview, box);
	Math::MatrixMultiply(viewproj, lightview, proj);

	device->GetViewport(&oldviewport);

	viewport.X = viewport.Y = 0;
	viewport.Width = viewport.Height = ShadowMapSize;
	viewport.MinZ = 0;
	viewport.MaxZ = 1;

	// must render everything (receivers too)
	LPDIRECT3DSURFACE9 surface = nullptr;
	othershadowmap->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, surface);
	device->SetViewport(&viewport);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1, 0);

	D3DXHANDLE oldtech = effect->GetCurrentTechnique();
	effect->SetTechnique("shadowmap");

	effect->SetMatrix("lightViewProj", (D3DXMATRIX*)&viewproj);
	effect->SetVector("clipPlanes", (D3DXVECTOR4*)&clipplanes);
	{
		callback(proj, clipplanes);
	}
	effect->SetTechnique(oldtech);

	surface->Release();

	// now blur it
	D3DXVECTOR4 pixelsize(1.0f / ShadowMapSize, 1.0f / ShadowMapSize, 0, 0);
	D3DXVECTOR4 texelsize = pixelsize;

	shadowmap->GetSurfaceLevel(0, &surface);

	device->SetRenderTarget(0, surface);
	device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);

	blureffect->SetVector("pixelSize", &pixelsize);
	blureffect->SetVector("texelSize", &texelsize);
		
	blureffect->Begin(NULL, 0);
	blureffect->BeginPass(0);
	{
		device->SetTexture(0, othershadowmap);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	blureffect->EndPass();
	blureffect->End();

	device->SetRenderState(D3DRS_ZENABLE, TRUE);
	device->SetViewport(&oldviewport);

	surface->Release();
	return clipplanes;
}
