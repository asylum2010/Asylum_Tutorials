
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\basiccamera.h"

// TODO:
// - fix local probe (can't, D3D has different addressing)

#define POINT_LIGHT_RADIUS	3.5f
#define CAR_ELEVATION		-0.27f

// helper macros
#define TITLE				"Shader sample 40: Deferred lighting"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample enums
enum RenderPass : uint32_t
{
	RenderPassShadow = 1,
	RenderPassLocalProbe = 2,
	RenderPassGBuffer = 4,
	RenderPassForward = 8
};

// sample structures
struct RenderPassParams
{
	LPDIRECT3DSURFACE9	target;
	D3DVIEWPORT9		viewport;
	Math::Matrix		view;
	Math::Matrix		proj;
	Math::Vector4		eye;
};

// sample variables
Application*			app				= nullptr;
LPDIRECT3DDEVICE9		device			= nullptr;
LPDIRECT3DTEXTURE9		helptext		= nullptr;

LPDIRECT3DCUBETEXTURE9	environment		= nullptr;
LPDIRECT3DCUBETEXTURE9	localprobe		= nullptr;

LPDIRECT3DTEXTURE9		scenetarget		= nullptr;
LPDIRECT3DTEXTURE9		normals			= nullptr;
LPDIRECT3DTEXTURE9		depth			= nullptr;
LPDIRECT3DTEXTURE9		illumdiffuse	= nullptr;
LPDIRECT3DTEXTURE9		illumspecular	= nullptr;

LPDIRECT3DSURFACE9		scenesurface	= nullptr;
LPDIRECT3DSURFACE9		normalsurface	= nullptr;
LPDIRECT3DSURFACE9		depthsurface	= nullptr;
LPDIRECT3DSURFACE9		illumdiffsurf	= nullptr;
LPDIRECT3DSURFACE9		illumspecsurf	= nullptr;

LPD3DXMESH				skymesh			= nullptr;

LPD3DXEFFECT			skyeffect		= nullptr;
LPD3DXEFFECT			shadowmap		= nullptr;
LPD3DXEFFECT			blur			= nullptr;
LPD3DXEFFECT			gbuffer			= nullptr;
LPD3DXEFFECT			deferred		= nullptr;
LPD3DXEFFECT			forward			= nullptr;
LPD3DXEFFECT			tonemap			= nullptr;
LPD3DXEFFECT			screenquad		= nullptr;

BasicCamera				camera;
RECT					scissorrect;
D3DXVECTOR4				pixelsize(1, 1, 1, 1);
int						selectedlight	= 0;

DXObject*				bridge			= nullptr;	// NOTE: UVs and tangent frame is wrong
DXObject*				car				= nullptr;

DXLight*				pointlights[3]	= { nullptr };
DXLight*				dirlights[2]	= { nullptr };
DXLight*				spotlights[2]	= { nullptr };

// forward declarations
void RenderScene(LPD3DXEFFECT effect, const D3DXMATRIX& viewproj, uint32_t passes);
void RenderShadowMaps();
void RenderLocalProbe();
void RenderGBuffer(const RenderPassParams& params, uint32_t passes);
void DeferredLighting(const RenderPassParams& params, uint32_t passes);
void RenderSky(const RenderPassParams& params);
void RenderTransparents(const RenderPassParams& params);
void RenderScissorRect();

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	bridge = new DXObject(device);
	car = new DXObject(device);

	if (!bridge->Load("../../Media/MeshesQM/bridge/bridge.qm")) {
		MYERROR("Could not load 'bridge.qm'");
		return false;
	}

	if (!bridge->AddNormalMap("../../Media/MeshesQM/bridge/bridge_normal.tga")) {
		MYERROR("Could not load 'bridge_normal.tga'");
		return false;
	}

	// NOTE: too bright
	//if (!bridge->AddSpecularMap("../../Media/MeshesQM/bridge/bridge_spec.dds")) {
	//	MYERROR("Could not load 'bridge_spec.dds'");
	//	return false;
	//}

	if (!car->Load("../../Media/MeshesQM/reventon/reventon.qm")) {
		MYERROR("Could not load 'reventon.qm'");
		return false;
	}

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/sky.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh)))
		return false;

	if (FAILED(D3DXCreateCubeTextureFromFileA(device, "../../Media/Textures/evening_cloudy.dds", &environment)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/sky.fx", &skyeffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/shadowmap.fx", &shadowmap)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/blur.fx", &blur)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/gbuffer_dl.fx", &gbuffer)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/deferredlighting.fx", &deferred)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/forwardlighting.fx", &forward)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/tonemap.fx", &tonemap)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device, "View scissor rect with keys 1-3 (press again to revert)", 1024, 256, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
		return false;

	// create render targets
	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &scenetarget, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &normals, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &depth, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &illumdiffuse, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &illumspecular, NULL)))
		return false;

	if (FAILED(device->CreateCubeTexture(256, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &localprobe, NULL)))
		return false;

	scenetarget->GetSurfaceLevel(0, &scenesurface);
	normals->GetSurfaceLevel(0, &normalsurface);
	depth->GetSurfaceLevel(0, &depthsurface);
	illumdiffuse->GetSurfaceLevel(0, &illumdiffsurf);
	illumspecular->GetSurfaceLevel(0, &illumspecsurf);

	// setup camera (right-handed)
	// NOTE: car faces in -X direction, it's left side is +Z
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 50);
	camera.SetZoomLimits(1.5f, 8);
	camera.SetDistance(2.4f);
	camera.SetPosition(-1, 0.3f, -0.4f);
	camera.SetOrientation(Math::DegreesToRadians(135), Math::DegreesToRadians(25), 0);

	// setup lights
	dirlights[0]	= new DXLight(LightTypeDirectional, { 0.1f, 1.0f, 0.7f, 0 }, (const D3DXCOLOR&)Math::Color::sRGBToLinear(140, 140, 140));
	dirlights[1]	= new DXLight(LightTypeDirectional, { 0.5f, 0.3f, -1.0f, 0 }, (const D3DXCOLOR&)Math::Color::sRGBToLinear(75, 75, 75));

	dirlights[0]->CreateShadowMap(device, 512);
	dirlights[1]->CreateShadowMap(device, 512);

	dirlights[0]->SetProjectionParameters(7.1f, 7.1f, -5, 5);
	dirlights[1]->SetProjectionParameters(7.1f, 7.1f, -5, 5);

	pointlights[0]	= new DXLight(LightTypePoint, { 1.5f, 0.5f, 0.0f, 1 }, { 1, 0, 0, 1 });
	pointlights[1]	= new DXLight(LightTypePoint, { -0.7f, 0.5f, 1.2f, 1 }, { 0, 1, 0, 1 });
	pointlights[2]	= new DXLight(LightTypePoint, { 0.0f, 0.5f, 0.0f, 1 }, { 0, 0, 1, 1 });

	pointlights[0]->CreateShadowMap(device, 256);
	pointlights[1]->CreateShadowMap(device, 256);
	pointlights[2]->CreateShadowMap(device, 256);

	pointlights[0]->SetProjectionParameters(0, 0, 0.1f, 10.0f);
	pointlights[1]->SetProjectionParameters(0, 0, 0.1f, 10.0f);
	pointlights[2]->SetProjectionParameters(0, 0, 0.1f, 10.0f);

	spotlights[0] = new DXLight(LightTypeSpot, { -1, 0.1f, 0.3f, 1 }, D3DXCOLOR(1, 1, 1, 1));
	spotlights[1] = new DXLight(LightTypeSpot, { -1, 0.1f, -0.3f, 1 }, D3DXCOLOR(1, 1, 1, 1));

	spotlights[0]->SetSpotParameters({ -1, 0, 0 }, D3DX_PI / 6, D3DX_PI / 4);
	spotlights[1]->SetSpotParameters({ -1, 0, 0 }, D3DX_PI / 6, D3DX_PI / 4);

	return true;
}

void UninitScene()
{
	delete bridge;
	delete car;

	delete pointlights[0];
	delete pointlights[1];
	delete pointlights[2];

	delete dirlights[0];
	delete dirlights[1];

	delete spotlights[0];
	delete spotlights[1];

	SAFE_RELEASE(skymesh);

	SAFE_RELEASE(scenesurface);
	SAFE_RELEASE(normalsurface);
	SAFE_RELEASE(depthsurface);
	SAFE_RELEASE(illumdiffsurf);
	SAFE_RELEASE(illumspecsurf);

	SAFE_RELEASE(scenetarget);
	SAFE_RELEASE(normals);
	SAFE_RELEASE(depth);
	SAFE_RELEASE(illumdiffuse);
	SAFE_RELEASE(illumspecular);

	SAFE_RELEASE(environment);
	SAFE_RELEASE(localprobe);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(shadowmap);
	SAFE_RELEASE(blur);
	SAFE_RELEASE(gbuffer);
	SAFE_RELEASE(deferred);
	SAFE_RELEASE(forward);
	SAFE_RELEASE(tonemap);
	SAFE_RELEASE(screenquad);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		selectedlight = (selectedlight == 1 ? 0 : 1);
		break;

	case KeyCode2:
		selectedlight = (selectedlight == 2 ? 0 : 2);
		break;

	case KeyCode3:
		selectedlight = (selectedlight == 3 ? 0 : 3);
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
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float dt)
{
	camera.Update(dt);
}

void RenderScene(LPD3DXEFFECT effect, const D3DXMATRIX& viewproj, uint32_t passes)
{
	D3DXMATRIX	inv;
	D3DXMATRIX	world[2];
	D3DXVECTOR4	uv(1, 1, 1, 1);

	D3DXHANDLE	oldtech	= effect->GetCurrentTechnique();
	D3DXHANDLE	tech	= nullptr;

	if (passes & RenderPassGBuffer)
		tech = effect->GetTechniqueByName("gbuffer_tbn");
	else if (passes & RenderPassForward)
		tech = effect->GetTechniqueByName("postreflective");

	// setup world matrices
	D3DXMatrixScaling(&world[0], 0.5f, 0.5f, 0.5f);
	D3DXMatrixScaling(&world[1], 1, 1, 1);

	world[0]._42 = CAR_ELEVATION;
	world[1]._42 = -1;

	effect->SetMatrix("matViewProj", &viewproj);
	effect->SetVector("uv", &uv);
	effect->SetFloat("hasTexture", 0);

	// car (NOTE: subset 6 is chassis)
	if (!(passes & RenderPassLocalProbe)) {
		D3DXMatrixInverse(&inv, NULL, &world[0]);

		effect->SetMatrix("matWorld", &world[0]);
		effect->SetMatrix("matWorldInv", &inv);

		// render every part normally
		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			car->Draw([&](DXObject* object, DWORD subset) -> bool {
				if (passes & RenderPassForward) {
					// except chassis
					if (subset == 6)
						return false;

					LPDIRECT3DTEXTURE9 texture = object->GetBaseTexture(subset);
					const D3DCOLORVALUE& color = object->GetBaseColor(subset);
					Math::Color linearcolor = Math::Color::sRGBToLinear(color.r, color.g, color.b, color.a);

					if (color.a < 1.0f)
						return false;

					effect->SetFloat("hasTexture", (texture != nullptr));
					effect->SetVector("baseColor", (D3DXVECTOR4*)&linearcolor);
					effect->CommitChanges();

					device->SetTexture(0, texture);
				} else if (passes & (RenderPassGBuffer|RenderPassShadow)) {
					const D3DCOLORVALUE& color = object->GetBaseColor(subset);

					if (color.a < 1.0f)
						return false;
				}

				return true;
			});
		}
		effect->EndPass();
		effect->End();

		// render chassis with reflection
		if (tech && (passes & RenderPassForward)) {
			effect->SetTechnique(tech);

			effect->Begin(NULL, 0);
			effect->BeginPass(0);
			{
				car->Draw([&](DXObject* object, DWORD subset) -> bool {
					if (subset != 6)
						return false;

					// make it silverish
					LPDIRECT3DTEXTURE9 texture = object->GetBaseTexture(subset);
					Math::Color linearcolor = Math::Color(0.972f, 0.96f, 0.915f, 1.0f);

					effect->SetFloat("hasTexture", (texture != nullptr));
					effect->SetVector("baseColor", (D3DXVECTOR4*)&linearcolor);
					effect->SetFloat("metalness", 0.75f);
					effect->CommitChanges();

					device->SetTexture(0, texture);
					return true;
				});
			}
			effect->EndPass();
			effect->End();

			effect->SetTechnique(oldtech);
		}
	}

	// bridge
	if (tech && (passes & RenderPassGBuffer))
		effect->SetTechnique(tech);

	D3DXMatrixInverse(&inv, NULL, &world[1]);

	effect->SetMatrix("matWorld", &world[1]);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetFloat("handedness", 1.0f);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		bridge->Draw([&](DXObject* object, DWORD subset) -> bool {
			if (passes & RenderPassForward) {
				LPDIRECT3DTEXTURE9 texture = object->GetBaseTexture(subset);
				const D3DCOLORVALUE& color = object->GetBaseColor(subset);
				Math::Color linearcolor = Math::Color::sRGBToLinear(color.r, color.g, color.b, color.a);

				effect->SetFloat("hasTexture", (texture != nullptr));
				effect->SetVector("baseColor", (D3DXVECTOR4*)&linearcolor);
				effect->CommitChanges();

				device->SetTexture(0, texture);
			} else if (passes & RenderPassGBuffer) {
				LPDIRECT3DTEXTURE9 normalmap = object->GetNormalMap(subset);
				LPDIRECT3DTEXTURE9 specularmap = object->GetSpecularMap(subset);

				device->SetTexture(0, specularmap);
				device->SetTexture(1, normalmap);
			}

			return true;
		});
	}
	effect->EndPass();
	effect->End();

	if (tech)
		effect->SetTechnique(oldtech);
}

void RenderShadowMaps()
{
	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

	// directional lights
	for (int i = 0; i < 2; ++i) {
		dirlights[i]->RenderShadowMap(device, [&](DXLight* light) {
			D3DXMATRIX viewproj;
			D3DXVECTOR4 clipplanes(light->GetNearPlane(), light->GetFarPlane(), 0, 0);
		
			light->CalculateViewProjection(viewproj);

			shadowmap->SetTechnique("variance");
			shadowmap->SetVector("clipPlanes", &clipplanes);
			shadowmap->SetBool("isPerspective", FALSE);

			device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
			RenderScene(shadowmap, viewproj, RenderPassShadow);
		});

		dirlights[i]->BlurShadowMap(device, [&](DXLight* light) {
			D3DXVECTOR4 pixelsize(1.0f / light->GetShadowMapSize(), 1.0f / light->GetShadowMapSize(), 0, 0);
			D3DXVECTOR4 texelsize = 4.0f * pixelsize;	// make it more blurry

			device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
			device->SetRenderState(D3DRS_ZENABLE, FALSE);

			blur->SetTechnique("boxblur3x3");
			blur->SetVector("pixelSize", &pixelsize);
			blur->SetVector("texelSize", &texelsize);
		
			blur->Begin(NULL, 0);
			blur->BeginPass(0);
			{
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
			blur->EndPass();
			blur->End();

			device->SetRenderState(D3DRS_ZENABLE, TRUE);
		});
	}

	// point lights
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	shadowmap->SetBool("isPerspective", TRUE);

	for (int i = 0; i < 3; ++i) {
		pointlights[i]->RenderShadowMap(device, [&](DXLight* light) {
			D3DXMATRIX viewproj;
			D3DXVECTOR4 clipplanes(light->GetNearPlane(), light->GetFarPlane(), 0, 0);
		
			light->CalculateViewProjection(viewproj);

			shadowmap->SetTechnique("variance");
			shadowmap->SetVector("lightPos", &light->GetPosition());
			shadowmap->SetVector("clipPlanes", &clipplanes);

			device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
			RenderScene(shadowmap, viewproj, RenderPassShadow);
		});
	}

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

void RenderLocalProbe()
{
	RenderPassParams params;
	Math::Vector3 look;

	params.eye = { 0, 0.3f, 0, 1 };

	params.viewport.X		= 0;
	params.viewport.Y		= 0;
	params.viewport.Width	= 256;
	params.viewport.Height	= 256;
	params.viewport.MinZ	= 0;
	params.viewport.MaxZ	= 1;

	Math::MatrixPerspectiveFovRH(params.proj, D3DX_PI / 2, 1, 0.1f, 5.0f);

	pixelsize.x = 1.0f / 256.0f;
	pixelsize.y = -1.0f / 256.0f;

	// NOTE: have to swap X directions
	// NOTE: individual faces are flipped right now
	int orderRH[] = { 1, 0, 2, 3, 4, 5 };

	for (int i = 0; i < 6; ++i) {
		int j = orderRH[i];

		Math::Vec3Add(look, (const Math::Vector3&)params.eye, (const Math::Vector3&)DXCubeForward[j]);
		Math::MatrixLookAtRH(params.view, (const Math::Vector3&)params.eye, look, (const Math::Vector3&)DXCubeUp[j]);

		LPDIRECT3DSURFACE9 surface = nullptr;
		localprobe->GetCubeMapSurface((D3DCUBEMAP_FACES)i, 0, &surface);

		params.target = surface;

		DeferredLighting(params, RenderPassLocalProbe);
		RenderSky(params);

		surface->Release();
	}
}

void RenderGBuffer(const RenderPassParams& params, uint32_t passes)
{
	Math::Matrix viewproj;
	Math::MatrixMultiply(viewproj, params.view, params.proj);

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->SetRenderTarget(0, normalsurface);
	device->SetRenderTarget(1, depthsurface);
	device->SetViewport(&params.viewport);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

	device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
	{
		gbuffer->SetTechnique("gbuffer");
		RenderScene(gbuffer, (const D3DXMATRIX&)viewproj, passes);
	}
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	device->SetRenderTarget(1, NULL);
}

void DeferredLighting(const RenderPassParams& params, uint32_t passes)
{
	Math::Matrix viewproj, viewprojinv;
	D3DXVECTOR4 texelsize(1.0f / app->GetClientWidth(), 1.0f / app->GetClientHeight(), 0, 0);

	Math::MatrixMultiply(viewproj, params.view, params.proj);
	Math::MatrixInverse(viewprojinv, viewproj);

	// geometry pass
	device->SetRenderState(D3DRS_ZENABLE, TRUE);
	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	{
		RenderGBuffer(params, passes|RenderPassGBuffer);
	}
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, FALSE);

	// accumulate illuminance
	device->SetRenderTarget(0, illumdiffsurf);
	device->SetRenderTarget(1, illumspecsurf);
	device->SetViewport(&params.viewport);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
	device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);

	for (int i = 0; i < 4; ++i) {
		device->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	device->SetTexture(0, normals);
	device->SetTexture(1, depth);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	deferred->SetTechnique("deferred");
	deferred->SetMatrix("matViewProjInv", (D3DXMATRIX*)&viewprojinv);
	deferred->SetVector("pixelSize", &pixelsize);
	deferred->SetVector("texelSize", &texelsize);
	deferred->SetVector("eyePos", (D3DXVECTOR4*)&params.eye);

	deferred->Begin(NULL, 0);
	deferred->BeginPass(0);
	{
		D3DXMATRIX lightviewproj;
		D3DXVECTOR4 clipplanes(0, 0, 0, 0);
	
		if (selectedlight == 0) {
			// directional lights
			for (int i = 0; i < 2; ++i) {
				dirlights[i]->CalculateViewProjection(lightviewproj);

				deferred->SetMatrix("lightViewProj", &lightviewproj);
				deferred->SetVector("lightColor", (D3DXVECTOR4*)&dirlights[i]->GetColor());
				deferred->SetVector("lightPos", &dirlights[i]->GetPosition());
				deferred->CommitChanges();

				device->SetTexture(2, dirlights[i]->GetShadowMap());
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}

			// spot lights
			for (int i = 0; i < 2; ++i) {
				deferred->SetVector("lightColor", (D3DXVECTOR4*)&spotlights[i]->GetColor());
				deferred->SetVector("lightPos", &spotlights[i]->GetPosition());
				deferred->SetVector("spotDirection", (D3DXVECTOR4*)&spotlights[i]->GetSpotDirection());
				deferred->SetVector("spotParams", (D3DXVECTOR4*)&spotlights[i]->GetSpotParameters());
				deferred->SetFloat("lightFlux", 50.0f);
				deferred->CommitChanges();

				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
		}

		// point lights
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

		for (int i = 0; i < 3; ++i) {
			clipplanes.x = pointlights[i]->GetNearPlane();
			clipplanes.y = pointlights[i]->GetFarPlane();

			if (selectedlight == 0 || selectedlight - 1 == i) {
				pointlights[i]->CalculateScissorRect(
					scissorrect, (const D3DXMATRIX&)params.view, (const D3DXMATRIX&)params.proj,
					POINT_LIGHT_RADIUS, app->GetClientWidth(), app->GetClientHeight());

				device->SetScissorRect(&scissorrect);

				deferred->SetVector("clipPlanes", &clipplanes);
				deferred->SetVector("lightColor", (D3DXVECTOR4*)&pointlights[i]->GetColor());
				deferred->SetVector("lightPos", &pointlights[i]->GetPosition());
				deferred->SetFloat("lightRadius", POINT_LIGHT_RADIUS);
				deferred->SetFloat("lightFlux", 15.0f);
				deferred->CommitChanges();

				device->SetTexture(3, pointlights[i]->GetCubeShadowMap());
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
		}

		device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	}
	deferred->EndPass();
	deferred->End();

	device->SetRenderTarget(1, NULL);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	// forward pass
	device->SetRenderTarget(0, params.target);
	device->SetViewport(&params.viewport);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);

	device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	device->SetTexture(1, illumdiffuse);
	device->SetTexture(2, illumspecular);
	device->SetTexture(3, localprobe);

	deferred->SetTechnique("postforward");
	RenderScene(deferred, (const D3DXMATRIX&)viewproj, passes|RenderPassForward);

	device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

void RenderSky(const RenderPassParams& params)
{
	Math::Matrix skyworld;
	Math::Matrix skyview = params.view;
	Math::Matrix viewproj;

	Math::MatrixRotationAxis(skyworld, Math::DegreesToRadians(-30), 1, 0, 0);

	skyview._41 = skyview._42 = skyview._43 = 0;
	Math::MatrixMultiply(viewproj, skyview, params.proj);
	
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);

	skyeffect->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
	skyeffect->SetMatrix("matSkyRotation", (D3DXMATRIX*)&skyworld);

	skyeffect->Begin(NULL, 0);
	skyeffect->BeginPass(0);
	{
		device->SetTexture(0, environment);
		skymesh->DrawSubset(0);
	}
	skyeffect->EndPass();
	skyeffect->End();

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

void RenderTransparents(const RenderPassParams& params)
{
	D3DXMATRIX		world, inv;
	D3DXMATRIX		lightviewproj;
	D3DXVECTOR4		clipplanes(0, 0, 0, 0);
	Math::Matrix	viewproj;

	Math::MatrixMultiply(viewproj, params.view, params.proj);

	D3DXMatrixScaling(&world, 0.5f, 0.5f, 0.5f);
	world._42 = CAR_ELEVATION;

	D3DXMatrixInverse(&inv, NULL, &world);

	forward->SetTechnique("forwardreflective");
	forward->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
	forward->SetMatrix("matWorld", &world);
	forward->SetMatrix("matWorldInv", &inv);
	forward->SetVector("eyePos", (D3DXVECTOR4*)&params.eye);

	device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	device->SetRenderState(D3DRS_ZENABLE, TRUE);
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	device->SetTexture(4, localprobe);

	auto RenderCar = [&](DXObject* object, DWORD subset) -> bool {
		LPDIRECT3DTEXTURE9 basetex = object->GetBaseTexture(subset);
		const D3DCOLORVALUE& color = object->GetBaseColor(subset);
		Math::Color linearcolor = Math::Color::sRGBToLinear(color.r, color.g, color.b, color.a);

		if (color.a >= 1.0f)
			return false;

		if (subset == 10) {
			// add the glass some tint
			linearcolor = Math::Color::sRGBToLinear(0.8f, 0.1f, 1.0f, color.a);
		}

		forward->SetFloat("hasTexture", (basetex != nullptr));
		forward->SetVector("baseColor", (D3DXVECTOR4*)&linearcolor);
		forward->CommitChanges();

		device->SetTexture(0, basetex);
		device->SetTexture(1, nullptr);

		return true;
	};

	forward->Begin(NULL, 0);
	forward->BeginPass(0);
	{
		if (selectedlight == 0) {
			// directional lights
			for (int i = 0; i < 2; ++i) {
				dirlights[i]->CalculateViewProjection(lightviewproj);

				forward->SetMatrix("lightViewProj", &lightviewproj);
				forward->SetVector("lightColor", (D3DXVECTOR4*)&dirlights[i]->GetColor());
				forward->SetVector("lightPos", &dirlights[i]->GetPosition());
				forward->CommitChanges();

				device->SetTexture(2, dirlights[i]->GetShadowMap());
				car->Draw(RenderCar);
			}
		}

		// point lights
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

		for (int i = 0; i < 3; ++i) {
			clipplanes.x = pointlights[i]->GetNearPlane();
			clipplanes.y = pointlights[i]->GetFarPlane();

			if (selectedlight == 0 || selectedlight - 1 == i) {
				pointlights[i]->CalculateScissorRect(
					scissorrect, (const D3DXMATRIX&)params.view, (const D3DXMATRIX&)params.proj,
					POINT_LIGHT_RADIUS, app->GetClientWidth(), app->GetClientHeight());

				device->SetScissorRect(&scissorrect);

				forward->SetVector("clipPlanes", &clipplanes);
				forward->SetVector("lightColor", (D3DXVECTOR4*)&pointlights[i]->GetColor());
				forward->SetVector("lightPos", &pointlights[i]->GetPosition());
				forward->SetFloat("lightRadius", POINT_LIGHT_RADIUS);
				forward->SetFloat("lightFlux", 15.0f);
				forward->CommitChanges();

				device->SetTexture(3, pointlights[i]->GetCubeShadowMap());
				car->Draw(RenderCar);
			}
		}

		device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	}
	forward->EndPass();
	forward->End();

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}

void RenderScissorRect()
{
	if (selectedlight != 0) {
		// draw some outline around scissor rect
		float rectvertices[24];
		uint16_t rectindices[5] = { 0, 1, 2, 3, 0 };

		memcpy(rectvertices, DXScreenQuadVerticesFFP, 24 * sizeof(float));

		rectvertices[0]		+= scissorrect.left;
		rectvertices[1]		+= scissorrect.top;
		rectvertices[6]		+= scissorrect.left;
		rectvertices[7]		+= scissorrect.bottom;
		rectvertices[12]	+= scissorrect.right;
		rectvertices[13]	+= scissorrect.bottom;
		rectvertices[18]	+= scissorrect.right;
		rectvertices[19]	+= scissorrect.top;

		device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);

		screenquad->SetTechnique("rect");
		screenquad->Begin(NULL, 0);
		screenquad->BeginPass(0);
		{
			device->DrawIndexedPrimitiveUP(D3DPT_LINESTRIP, 0, 4, 4, rectindices, D3DFMT_INDEX16, rectvertices, 6 * sizeof(float));
		}
		screenquad->EndPass();
		screenquad->End();

		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
	}
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	RenderPassParams	params;
	Math::Matrix		viewinv;
	Math::Matrix		viewproj;

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(params.view);
	camera.GetProjectionMatrix(params.proj);
	camera.GetEyePosition((Math::Vector3&)params.eye);

	Math::MatrixMultiply(viewproj, params.view, params.proj);

	// setup lights
	pointlights[0]->GetPosition().x = cosf(time * 0.5f) * 2;
	pointlights[0]->GetPosition().z = sinf(time * 0.5f) * cosf(time * 0.5f) * 2;

	pointlights[1]->GetPosition().x = cosf(1.5f * time) * 2;
	pointlights[1]->GetPosition().z = sinf(1 * time) * 2;

	pointlights[2]->GetPosition().x = cosf(0.75f * time) * 1.5f;
	pointlights[2]->GetPosition().z = sinf(1.5f * time) * 1.5f;

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;
		LPDIRECT3DSURFACE9 backbuffer = nullptr;

		device->GetRenderTarget(0, &backbuffer);
		device->GetViewport(&oldviewport);

		// STEP 0: render shadow maps & local probe
		RenderShadowMaps();
		RenderLocalProbe();

		// STEP 1: deferred lighting
		pixelsize.x = 1.0f / (float)oldviewport.Width;
		pixelsize.y = -1.0f / (float)oldviewport.Height;

		params.viewport = oldviewport;
		params.target = scenesurface;

		DeferredLighting(params, 0);
		RenderScissorRect();

		// STEP 2: render sky
		device->SetRenderTarget(0, backbuffer);

		RenderSky(params);

		// STEP 3: tone mapping
		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		tonemap->SetTechnique("tonemap");
		tonemap->SetVector("pixelSize", &pixelsize);

		tonemap->Begin(NULL, 0);
		tonemap->BeginPass(0);
		{
			device->SetTexture(0, scenetarget);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		tonemap->EndPass();
		tonemap->End();

		// STEP 4: now render transparent parts
		device->SetRenderTarget(0, scenesurface);
		device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1, 0);

		RenderTransparents(params);

		// STEP 5: tone map transparents & combine
		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

		device->SetRenderTarget(0, backbuffer);
		backbuffer->Release();

		tonemap->Begin(NULL, 0);
		tonemap->BeginPass(0);
		{
			device->SetTexture(0, scenetarget);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		tonemap->EndPass();
		tonemap->End();

		// render text
		viewport = oldviewport;

		viewport.Width = 1024;
		viewport.Height = 256;
		viewport.X = viewport.Y = 10;

		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->SetViewport(&viewport);

		screenquad->SetTechnique("screenquad");
		screenquad->Begin(NULL, 0);
		screenquad->BeginPass(0);
		{
			device->SetTexture(0, helptext);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		screenquad->EndPass();
		screenquad->End();

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
