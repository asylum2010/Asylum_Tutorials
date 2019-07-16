
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\basiccamera.h"

// helper macros
#define TITLE				"Shader sample 39: HDR effects"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*			app						= nullptr;
LPDIRECT3DDEVICE9		device					= nullptr;
LPDIRECT3DTEXTURE9		helptext				= nullptr;

LPDIRECT3DCUBETEXTURE9	environment				= nullptr;		// HDR environment
LPDIRECT3DCUBETEXTURE9	irradiance1				= nullptr;		// preintegrated diffuse irradiance
LPDIRECT3DCUBETEXTURE9	irradiance2				= nullptr;		// preintegrated specular irradiance
LPDIRECT3DTEXTURE9		brdfLUT					= nullptr;		// preintegrated BRDF lookup texture

LPDIRECT3DTEXTURE9		scenetarget				= nullptr;
LPDIRECT3DTEXTURE9		avgluminance			= nullptr;
LPDIRECT3DTEXTURE9		avglumsystemmem			= nullptr;
LPDIRECT3DTEXTURE9		dsampletargets[5]		= { nullptr };
LPDIRECT3DTEXTURE9		blurtargets[5]			= { nullptr };
LPDIRECT3DTEXTURE9		startargets[4][2]		= { { nullptr } };
LPDIRECT3DTEXTURE9		lensflaretargets[2]		= { nullptr };
LPDIRECT3DTEXTURE9		afterimagetargets[2]	= { nullptr };
LPDIRECT3DTEXTURE9		bloomresult				= nullptr;
LPDIRECT3DTEXTURE9		starresult				= nullptr;

LPDIRECT3DSURFACE9		scenesurface			= nullptr;
LPDIRECT3DSURFACE9		avglumsurfaces[4]		= { nullptr };
LPDIRECT3DSURFACE9		avglumsysmemsurface		= nullptr;
LPDIRECT3DSURFACE9		dsamplesurfaces[5]		= { nullptr };
LPDIRECT3DSURFACE9		blursurfaces[5]			= { nullptr };
LPDIRECT3DSURFACE9		starsurfaces[4][2]		= { { nullptr } };
LPDIRECT3DSURFACE9		lensflaresurfaces[2]	= { nullptr };
LPDIRECT3DSURFACE9		afterimagesurfaces[2]	= { nullptr };
LPDIRECT3DSURFACE9		bloomsurface			= nullptr;
LPDIRECT3DSURFACE9		starsurface				= nullptr;

LPD3DXMESH				teapot					= nullptr;
LPD3DXMESH				skull					= nullptr;
LPD3DXMESH				knot					= nullptr;
LPD3DXMESH				skymesh					= nullptr;
LPD3DXMESH				mesh					= nullptr;

LPD3DXEFFECT			skyeffect				= nullptr;
LPD3DXEFFECT			metaleffect				= nullptr;
LPD3DXEFFECT			insulatoreffect			= nullptr;
LPD3DXEFFECT			measureeffect			= nullptr;
LPD3DXEFFECT			hdreffects				= nullptr;
LPD3DXEFFECT			screenquad				= nullptr;
LPD3DXEFFECT			effect					= nullptr;

BasicCamera				camera;
BasicCamera				objectrotator;
D3DVIEWPORT9			oldviewport;
D3DVIEWPORT9			viewport;
D3DXVECTOR4				pixelsize(0, 0, 0, 1);
D3DXVECTOR4				texelsize(0, 0, 0, 1);
float					averageluminance	= 0.1f;	// don't set this to zero!!!
float					adaptedluminance	= 0.1f;	// don't set this to zero!!!
float					exposure			= 0;
uint8_t					mousebuttons		= 0;
int						currentafterimage	= 0;
bool					drawafterimage		= false;
bool					drawtext			= true;

// forward declarations
void RenderScene(float alpha);
void MeasureLuminance();
void AdaptLuminance(float dt);
void BrightPass();
void DownSample();
void Stars();
void Bloom();
void LensFlare();
void ToneMap();

bool InitScene()
{
	// print Gaussian weights (kernel_length = 6*sigma - 1)
	//printf("static const float blurWeights[9] =\n{\n");
	//
	//for (int i = -4; i < 5; ++i) {
	//	printf("    %.6ff,\n", Math::Gaussian((float)i, (9.0f + 1.0f) / 6.0f));
	//}
	//
	//printf("};\n");

	// check for HDR support first
	D3DDISPLAYMODE mode;
	LPDIRECT3D9 direct3d = (LPDIRECT3D9)app->GetDriverInterface();

	if (FAILED(direct3d->GetAdapterDisplayMode(0, &mode))) {
		MYERROR("Could not get adapter mode");
		return false;
	}
	
	if (FAILED(direct3d->CheckDeviceFormat(0, D3DDEVTYPE_HAL, mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F))) {
		MYERROR("No floating point rendertarget support");
		return false;
	}
	
	if (FAILED(direct3d->CheckDepthStencilMatch(0, D3DDEVTYPE_HAL, mode.Format, D3DFMT_A16B16G16R16F, D3DFMT_D24S8))) {
		MYERROR("D3DFMT_A16B16G16R16F does not support D3DFMT_D24S8");
		return false;
	}

	// load resources
	if (FAILED(D3DXCreateCubeTextureFromFile(device, "../../Media/Textures/grace.dds", &environment)))
		return false;

	if (FAILED(D3DXCreateCubeTextureFromFile(device, "../../Media/Textures/grace_diff_irrad.dds", &irradiance1)))
		return false;

	if (FAILED(D3DXCreateCubeTextureFromFile(device, "../../Media/Textures/grace_spec_irrad.dds", &irradiance2)))
		return false;

	if (FAILED(D3DXCreateTextureFromFile(device, "../../Media/Textures/brdf.dds", &brdfLUT)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/knot.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &knot)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/skullocc3.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skull)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/teapot.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &teapot)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/sky.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/sky.fx", &skyeffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/metal.fx", &metaleffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/insulator.fx", &insulatoreffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/measureluminance.fx", &measureeffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/hdreffects.fx", &hdreffects)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(
		device,
		"Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\nMouse right - Rotate object\n\n1 - Change object\n2 - Change material\n3 - Toggle afterimage\n\nH - Toggle help text",
		512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
		return false;

	// create render targets
	UINT screenwidth = app->GetClientWidth();
	UINT screenheight = app->GetClientHeight();

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &scenetarget, NULL)))
		return false;

	if (FAILED(device->CreateTexture(64, 64, 0, D3DUSAGE_RENDERTARGET, D3DFMT_R16F, D3DPOOL_DEFAULT, &avgluminance, NULL)))
		return false;

	if (FAILED(device->CreateTexture(1, 1, 0, 0, D3DFMT_R16F, D3DPOOL_SYSTEMMEM, &avglumsystemmem, NULL)))
		return false;

	for (int i = 0; i < 4; ++i) {
		avgluminance->GetSurfaceLevel(i * 2, &avglumsurfaces[i]);
	}

	avglumsystemmem->GetSurfaceLevel(0, &avglumsysmemsurface);

	if (FAILED(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &bloomresult, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth / 4, screenheight / 4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &starresult, NULL)))
		return false;

	for (int i = 0; i < 5; ++i) {
		if (FAILED(device->CreateTexture(screenwidth / (2 << i), screenheight / (2 << i), 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &dsampletargets[i], NULL)))
			return false;

		if (FAILED(device->CreateTexture(screenwidth / (2 << i), screenheight / (2 << i), 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &blurtargets[i], NULL)))
			return false;

		dsampletargets[i]->GetSurfaceLevel(0, &dsamplesurfaces[i]);
		blurtargets[i]->GetSurfaceLevel(0, &blursurfaces[i]);
	}

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 2; ++j) {
			if (FAILED(device->CreateTexture(screenwidth / 4, screenheight / 4, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &startargets[i][j], NULL)))
				return false;

			startargets[i][j]->GetSurfaceLevel(0, &starsurfaces[i][j]);
		}
	}

	for (int i = 0; i < 2; ++i) {
		if (FAILED(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &lensflaretargets[i], NULL)))
			return false;

		if (FAILED(device->CreateTexture(screenwidth / 2, screenheight / 2, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &afterimagetargets[i], NULL)))
			return false;

		lensflaretargets[i]->GetSurfaceLevel(0, &lensflaresurfaces[i]);
		afterimagetargets[i]->GetSurfaceLevel(0, &afterimagesurfaces[i]);
	}

	scenetarget->GetSurfaceLevel(0, &scenesurface);
	bloomresult->GetSurfaceLevel(0, &bloomsurface);
	starresult->GetSurfaceLevel(0, &starsurface);

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetClipPlanes(0.1f, 50);
	camera.SetZoomLimits(3, 10);
	camera.SetDistance(5);
	camera.SetOrientation(Math::DegreesToRadians(146), 0.1f, 0);

	objectrotator.SetDistance(0);

	mesh = skull;
	effect = insulatoreffect;

	return true;
}

void UninitScene()
{
	SAFE_RELEASE(helptext);
	SAFE_RELEASE(environment);
	SAFE_RELEASE(irradiance1);
	SAFE_RELEASE(irradiance2);
	SAFE_RELEASE(brdfLUT);

	SAFE_RELEASE(scenesurface);
	SAFE_RELEASE(bloomsurface);
	SAFE_RELEASE(starsurface);

	SAFE_RELEASE(scenetarget);
	SAFE_RELEASE(bloomresult);
	SAFE_RELEASE(starresult);

	for (int i = 0; i < 4; ++i) {
		SAFE_RELEASE(avglumsurfaces[i]);
	}

	SAFE_RELEASE(avglumsysmemsurface);

	SAFE_RELEASE(avgluminance);
	SAFE_RELEASE(avglumsystemmem);

	for (int i = 0; i < 5; ++i) {
		SAFE_RELEASE(dsamplesurfaces[i]);
		SAFE_RELEASE(blursurfaces[i]);

		SAFE_RELEASE(dsampletargets[i]);
		SAFE_RELEASE(blurtargets[i]);
	}

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 2; ++j) {
			SAFE_RELEASE(starsurfaces[i][j]);
			SAFE_RELEASE(startargets[i][j]);
		}
	}

	for (int i = 0; i < 2; ++i) {
		SAFE_RELEASE(lensflaresurfaces[i]);
		SAFE_RELEASE(afterimagesurfaces[i]);

		SAFE_RELEASE(lensflaretargets[i]);
		SAFE_RELEASE(afterimagetargets[i]);
	}

	SAFE_RELEASE(teapot);
	SAFE_RELEASE(skull);
	SAFE_RELEASE(knot);
	SAFE_RELEASE(skymesh);

	SAFE_RELEASE(metaleffect);
	SAFE_RELEASE(insulatoreffect);
	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(measureeffect);
	SAFE_RELEASE(hdreffects);
	SAFE_RELEASE(screenquad);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		if (mesh == teapot)
			mesh = knot;
		else if (mesh == knot)
			mesh = skull;
		else
			mesh = teapot;

		break;

	case KeyCode2:
		if (effect == metaleffect)
			effect = insulatoreffect;
		else
			effect = metaleffect;

		break;

	case KeyCode3:
		drawafterimage = !drawafterimage;
		break;

	case KeyCodeH:
		drawtext = !drawtext;
		break;

	default:
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonLeft) {
		camera.OrbitRight(Math::DegreesToRadians(dx));
		camera.OrbitUp(Math::DegreesToRadians(dy));
	} else if (state & MouseButtonMiddle) {
		float scale = camera.GetDistance() / 10.0f;
		float amount = 1e-3f + scale * (0.1f - 1e-3f);

		camera.PanRight(dx * -amount);
		camera.PanUp(dy * amount);
	}

	if (state & MouseButtonRight) {
		objectrotator.OrbitRight(Math::DegreesToRadians(dx));
		objectrotator.OrbitUp(-Math::DegreesToRadians(dy));
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float dt)
{
	camera.Update(dt);
	objectrotator.Update(dt);
}

void RenderScene(float alpha)
{
	D3DXMATRIX		identity;
	D3DXMATRIX		world, worldinv;
	D3DXMATRIX		rotation;
	D3DXVECTOR4		silver = { 0.972f, 0.96f, 0.915f, 1.0f };
	Math::Color		insulator = Math::Color::sRGBToLinear(255, 255, 255);

	Math::Matrix	view, proj;
	Math::Matrix	skyview, viewproj;
	Math::Vector4	eye;
	Math::Vector3	orient;

	D3DXMatrixIdentity(&identity);

	objectrotator.Animate(alpha);
	objectrotator.GetOrientation(orient);

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);

	device->SetRenderTarget(0, scenesurface);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xff000000, 1.0f, 0);
	{
		// render sky
		skyview = view;
		skyview._41 = skyview._42 = skyview._43 = 0;

		Math::MatrixMultiply(viewproj, skyview, proj);

		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		skyeffect->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		skyeffect->SetMatrix("matSkyRotation", &identity);

		skyeffect->Begin(NULL, 0);
		skyeffect->BeginPass(0);
		{
			device->SetTexture(0, environment);
			skymesh->DrawSubset(0);
		}
		skyeffect->EndPass();
		skyeffect->End();

		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		// render object
		if (mesh == teapot) {
			D3DXMatrixScaling(&world, 1.5f, 1.5f, 1.5f);
		} else if (mesh == knot) {
			D3DXMatrixScaling(&world, 0.8f, 0.8f, 0.8f);
		} else if (mesh == skull) {
			D3DXMatrixScaling(&world, 0.4f, 0.4f, 0.4f);
			world._42 = -1.5f;
		}

		D3DXMatrixRotationYawPitchRoll(&rotation, orient.x, orient.y, 0);
		D3DXMatrixMultiply(&world, &world, &rotation);
		D3DXMatrixInverse(&worldinv, NULL, &world);

		Math::MatrixMultiply(viewproj, view, proj);

		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		effect->SetMatrix("matWorld", &world);
		effect->SetMatrix("matWorldInv", &worldinv);
		effect->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		effect->SetVector("eyePos", (D3DXVECTOR4*)&eye);

		if (effect == metaleffect) {
			effect->SetVector("baseColor", &silver);
			effect->SetFloat("roughness", 0.0f);

			device->SetTexture(0, irradiance2);
			device->SetTexture(1, brdfLUT);
		} else {
			effect->SetVector("baseColor", (D3DXVECTOR4*)&insulator);
			effect->SetFloat("roughness", 0.2f);

			device->SetTexture(0, irradiance1);
			device->SetTexture(1, irradiance2);
			device->SetTexture(2, brdfLUT);
		}

		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	}
}

void MeasureLuminance()
{
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	for (int i = 0; i < 4; ++i) {
		viewport.Width = 64 >> (i * 2);
		viewport.Height = 64 >> (i * 2);

		pixelsize.x = 1.0f / (float)viewport.Width;
		pixelsize.y = -1.0f / (float)viewport.Height;

		if (i == 0) {
			texelsize.x = 1.0f / (float)oldviewport.Width;
			texelsize.y = 1.0f / (float)oldviewport.Height;

			measureeffect->SetTechnique("avgluminital");
			device->SetTexture(0, scenetarget);
		} else if (i == 3) {
			texelsize.x = 1.0f / (float)(64 >> (2 * (i - 1)));
			texelsize.y = 1.0f / (float)(64 >> (2 * (i - 1)));

			measureeffect->SetTechnique("avglumfinal");
			device->SetTexture(0, avgluminance);
		} else {
			texelsize.x = 1.0f / (float)(64 >> (2 * (i - 1)));
			texelsize.y = 1.0f / (float)(64 >> (2 * (i - 1)));

			measureeffect->SetTechnique("avglumiterative");
			device->SetTexture(0, avgluminance);
		}

		measureeffect->SetInt("prevLevel", (i - 1) * 2);
		measureeffect->SetVector("pixelSize", &pixelsize);
		measureeffect->SetVector("texelSize", &texelsize);

		device->SetRenderTarget(0, avglumsurfaces[i]);
		device->SetViewport(&viewport);

		measureeffect->Begin(NULL, 0);
		measureeffect->BeginPass(0);
		{
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		measureeffect->EndPass();
		measureeffect->End();
	}

	D3DLOCKED_RECT rect;

	device->GetRenderTargetData(avglumsurfaces[3], avglumsysmemsurface);
	avglumsystemmem->LockRect(0, &rect, NULL, D3DLOCK_READONLY);
	{
		averageluminance = ((Math::Float16*)rect.pBits)->operator float();
	}
	avglumsystemmem->UnlockRect(0);
}

void AdaptLuminance(float dt)
{
	adaptedluminance = adaptedluminance + (averageluminance - adaptedluminance) * (1.0f - powf(0.98f, 50.0f * dt));

	// DICE's suggestion
	float two_ad_EV = adaptedluminance * (100.0f / 12.5f);
	exposure = 1.0f / (1.2f * two_ad_EV);
}

void BrightPass()
{
	viewport.Width = oldviewport.Width / 2;
	viewport.Height = oldviewport.Height / 2;

	pixelsize.x = 1.0f / (float)viewport.Width;
	pixelsize.y = -1.0f / (float)viewport.Height;

	device->SetRenderTarget(0, dsamplesurfaces[0]);
	device->SetViewport(&viewport);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	hdreffects->SetTechnique("brightpass");
	hdreffects->SetVector("pixelSize", &pixelsize);
	hdreffects->SetFloat("exposure", exposure);

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		device->SetTexture(0, scenetarget);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	hdreffects->EndPass();
	hdreffects->End();

	// generate afterimage here
	device->SetRenderTarget(0, afterimagesurfaces[currentafterimage]);

	if (drawafterimage) {
		hdreffects->SetTechnique("afterimage");
		hdreffects->SetVector("pixelSize", &pixelsize);

		hdreffects->Begin(NULL, 0);
		hdreffects->BeginPass(0);
		{
			device->SetTexture(0, afterimagetargets[1 - currentafterimage]);
			device->SetTexture(1, dsampletargets[0]);

			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		hdreffects->EndPass();
		hdreffects->End();
	} else {
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1, 0);
	}

	currentafterimage = 1 - currentafterimage;
}

void DownSample()
{
	viewport.Width = oldviewport.Width / 2;
	viewport.Height = oldviewport.Height / 2;

	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	hdreffects->SetTechnique("downsample");

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		for (int i = 1; i < 5; ++i) {
			texelsize.x = 1.0f / (float)viewport.Width;
			texelsize.y = 1.0f / (float)viewport.Height;

			viewport.Width /= 2;
			viewport.Height /= 2;

			pixelsize.x = 1.0f / (float)viewport.Width;
			pixelsize.y = -1.0f / (float)viewport.Height;

			hdreffects->SetVector("pixelSize", &pixelsize);
			hdreffects->SetVector("texelSize", &texelsize);
			hdreffects->CommitChanges();

			device->SetRenderTarget(0, dsamplesurfaces[i]);
			device->SetTexture(0, dsampletargets[i - 1]);
			device->SetViewport(&viewport);

			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
	}
	hdreffects->EndPass();
	hdreffects->End();
}

void Stars()
{
	viewport.Width = oldviewport.Width / 4;
	viewport.Height = oldviewport.Height / 4;

	pixelsize.x = 1.0f / (float)viewport.Width;
	pixelsize.y = -1.0f / (float)viewport.Height;

	texelsize.x = 1.0f / (float)viewport.Width;
	texelsize.y = 1.0f / (float)viewport.Height;

	device->SetViewport(&viewport);
	
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

	hdreffects->SetTechnique("star");
	hdreffects->SetVector("pixelSize", &pixelsize);
	hdreffects->SetVector("texelSize", &texelsize);

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		for (int i = 0; i < 4; ++i) {
			hdreffects->SetInt("starDirection", i);

			for (int j = 0; j < 3; ++j) {
				hdreffects->SetInt("starPass", j);
				hdreffects->CommitChanges();

				device->SetRenderTarget(0, starsurfaces[i][j % 2]);
				device->SetTexture(0, (j == 0 ? dsampletargets[1] : startargets[i][1 - j % 2]));

				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
		}
	}
	hdreffects->EndPass();
	hdreffects->End();

	// combine
	for (int i = 1; i < 4; ++i) {
		device->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	hdreffects->SetTechnique("starcombine");

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		device->SetRenderTarget(0, starsurface);

		device->SetTexture(0, startargets[0][0]);
		device->SetTexture(1, startargets[1][0]);
		device->SetTexture(2, startargets[2][0]);
		device->SetTexture(3, startargets[3][0]);

		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	hdreffects->EndPass();
	hdreffects->End();
}

void Bloom()
{
	viewport.Width = oldviewport.Width / 2;
	viewport.Height = oldviewport.Height / 2;

	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

	hdreffects->SetTechnique("blur");

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		for (int i = 0; i < 5; ++i) {
			device->SetViewport(&viewport);

			pixelsize.x = 1.0f / (float)viewport.Width;
			pixelsize.y = -1.0f / (float)viewport.Height;

			texelsize.x = 1.0f / (float)viewport.Width;
			texelsize.y = 1.0f / (float)viewport.Height;

			hdreffects->SetVector("pixelSize", &pixelsize);
			hdreffects->SetVector("texelSize", &texelsize);

			// horizontal
			hdreffects->SetInt("blurDirection", 0);
			hdreffects->CommitChanges();

			device->SetRenderTarget(0, blursurfaces[i]);
			device->SetTexture(0, dsampletargets[i]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));

			// vertical
			hdreffects->SetInt("blurDirection", 1);
			hdreffects->CommitChanges();

			device->SetRenderTarget(0, dsamplesurfaces[i]);
			device->SetTexture(0, blurtargets[i]);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));

			viewport.Width /= 2;
			viewport.Height /= 2;
		}
	}
	hdreffects->EndPass();
	hdreffects->End();

	// combine
	viewport.Width = oldviewport.Width / 2;
	viewport.Height = oldviewport.Height / 2;

	pixelsize.x = 1.0f / (float)viewport.Width;
	pixelsize.y = -1.0f / (float)viewport.Height;

	for (int i = 1; i < 5; ++i) {
		device->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	hdreffects->SetTechnique("blurcombine");
	hdreffects->SetVector("pixelSize", &pixelsize);

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		device->SetRenderTarget(0, bloomsurface);
		device->SetViewport(&viewport);

		device->SetTexture(0, dsampletargets[0]);
		device->SetTexture(1, dsampletargets[1]);
		device->SetTexture(2, dsampletargets[2]);
		device->SetTexture(3, dsampletargets[3]);
		device->SetTexture(4, dsampletargets[4]);

		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	hdreffects->EndPass();
	hdreffects->End();
}

void LensFlare()
{
	viewport.Width = oldviewport.Width / 2;
	viewport.Height = oldviewport.Height / 2;

	pixelsize.x = 1.0f / (float)viewport.Width;
	pixelsize.y = -1.0f / (float)viewport.Height;

	for (int i = 0; i < 3; ++i) {
		device->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
		device->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
	}

	hdreffects->SetTechnique("lensflare");
	hdreffects->SetVector("pixelSize", &pixelsize);

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		device->SetRenderTarget(0, lensflaresurfaces[0]);
		device->SetViewport(&viewport);

		device->SetTexture(0, dsampletargets[0]);
		device->SetTexture(1, dsampletargets[1]);
		device->SetTexture(2, dsampletargets[2]);

		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	hdreffects->EndPass();
	hdreffects->End();

	// expand lens flares
	hdreffects->SetTechnique("lensflare_expand");

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		device->SetRenderTarget(0, lensflaresurfaces[1]);
		device->SetViewport(&viewport);

		device->SetTexture(0, lensflaretargets[0]);
		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	hdreffects->EndPass();
	hdreffects->End();
}

void ToneMap()
{
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	pixelsize.x = 1.0f / (float)oldviewport.Width;
	pixelsize.y = -1.0f / (float)oldviewport.Height;

	device->SetViewport(&oldviewport);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	hdreffects->SetTechnique("tonemap");
	hdreffects->SetVector("pixelSize", &pixelsize);
	hdreffects->SetFloat("exposure", exposure);

	hdreffects->Begin(NULL, 0);
	hdreffects->BeginPass(0);
	{
		device->SetTexture(0, scenetarget);
		device->SetTexture(1, bloomresult);
		device->SetTexture(2, starresult);
		device->SetTexture(3, lensflaretargets[1]);
		device->SetTexture(4, afterimagetargets[1 - currentafterimage]);

		device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
	}
	hdreffects->EndPass();
	hdreffects->End();
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	if (SUCCEEDED(device->BeginScene())) {
		LPDIRECT3DSURFACE9 backbuffer = nullptr;

		device->GetRenderTarget(0, &backbuffer);
		device->GetViewport(&oldviewport);

		viewport = oldviewport;
	
		// STEP 1: render scene
		RenderScene(alpha);
		
		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);

		// STEP 2: measure average luminance
		MeasureLuminance();
		AdaptLuminance(elapsedtime);

		// STEP 3: extract high-luminance regions
		BrightPass();

		// STEP 4: downsample before blur
		DownSample();

		// STEP 5: render stars
		Stars();

		// STEP 6: render bloom
		Bloom();

		// STEP 7: lens flare
		LensFlare();

		// STEP 9: tonemap & final combine
		device->SetRenderTarget(0, backbuffer);
		backbuffer->Release();

		ToneMap();

		// render text
		viewport.Width = 512;
		viewport.Height = 512;
		viewport.X = viewport.Y = 10;

		if (drawtext) {
			device->SetViewport(&viewport);
			device->SetTexture(0, helptext);

			device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device->SetRenderState(D3DRS_ZENABLE, FALSE);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			screenquad->Begin(NULL, 0);
			screenquad->BeginPass(0);
			{
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
			screenquad->EndPass();
			screenquad->End();
		}

		// reset states
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, TRUE);
		device->SetViewport(&oldviewport);

		device->EndScene();
	}

	time += elapsedtime;
	device->Present(NULL, NULL, NULL, NULL);
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIDirect3D9)) {
		delete app;
		return 1;
	}

	device = (LPDIRECT3DDEVICE9)app->GetLogicalDevice();

	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->UpdateCallback = Update;
	app->RenderCallback = Render;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseScrollCallback = MouseScroll;

	app->Run();
	delete app;

	return 0;
}
