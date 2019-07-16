
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\basiccamera.h"

#define POINT_LIGHT_RADIUS	3.0f

// helper macros
#define TITLE				"Shader sample 40: Deferred shading"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*		app				= nullptr;
LPDIRECT3DDEVICE9	device			= nullptr;
LPDIRECT3DTEXTURE9	helptext		= nullptr;

LPDIRECT3DTEXTURE9	marble			= nullptr;
LPDIRECT3DTEXTURE9	wood			= nullptr;
LPDIRECT3DTEXTURE9	wood_normal		= nullptr;
LPDIRECT3DTEXTURE9	sky				= nullptr;

LPDIRECT3DTEXTURE9	scenetarget		= nullptr;
LPDIRECT3DTEXTURE9	albedo			= nullptr;
LPDIRECT3DTEXTURE9	normals			= nullptr;
LPDIRECT3DTEXTURE9	depth			= nullptr;

LPDIRECT3DSURFACE9	scenesurface	= nullptr;
LPDIRECT3DSURFACE9	albedosurface	= nullptr;
LPDIRECT3DSURFACE9	normalsurface	= nullptr;
LPDIRECT3DSURFACE9	depthsurface	= nullptr;

LPD3DXMESH			box				= nullptr;
LPD3DXMESH			skull			= nullptr;

LPD3DXEFFECT		shadowmap		= nullptr;
LPD3DXEFFECT		blur			= nullptr;
LPD3DXEFFECT		gbuffer			= nullptr;
LPD3DXEFFECT		deferred		= nullptr;
LPD3DXEFFECT		tonemap			= nullptr;
LPD3DXEFFECT		screenquad		= nullptr;

BasicCamera			camera;
D3DXVECTOR4			pixelsize(1, 1, 1, 1);
int					selectedlight	= 0;

DXLight*			moonlight		= nullptr;
DXLight*			pointlights[3]	= { nullptr };

// forward declarations
void RenderScene(LPD3DXEFFECT effect, const D3DXMATRIX& viewproj);
void RenderShadowMaps();
void RenderGBuffer(const Math::Matrix& viewproj);
void DeferredShading(const Math::Matrix& view, const Math::Matrix& proj, const Math::Matrix& viewprojinv, const Math::Vector4& eye);

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/box.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &box)))
		return false;

	if (FAILED(DXGenTangentFrame(device, box, &box)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/skullocc3.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skull)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/marble.dds", &marble)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/wood2.jpg", &wood)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/wood2_normal.tga", &wood_normal)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/static_sky.jpg", &sky)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/shadowmap.fx", &shadowmap)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/blur.fx", &blur)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/gbuffer_ds.fx", &gbuffer)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/deferredshading.fx", &deferred)))
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

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &albedo, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &normals, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &depth, NULL)))
		return false;

	scenetarget->GetSurfaceLevel(0, &scenesurface);
	albedo->GetSurfaceLevel(0, &albedosurface);
	normals->GetSurfaceLevel(0, &normalsurface);
	depth->GetSurfaceLevel(0, &depthsurface);

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 50);
	camera.SetZoomLimits(3, 10);
	camera.SetDistance(5);
	camera.SetPosition(0, 0.5f, 0);
	camera.SetOrientation(Math::DegreesToRadians(-135), Math::DegreesToRadians(30), 0);

	// setup lights
	moonlight		= new DXLight(LightTypeDirectional, { 0, 0, 0, 0 }, (const D3DXCOLOR&)Math::Color::sRGBToLinear(250, 250, 250));
	pointlights[0]	= new DXLight(LightTypePoint, { 1.5f, 0.5f, 0.0f, 1 }, { 1, 0, 0, 1 });
	pointlights[1]	= new DXLight(LightTypePoint, { -0.7f, 0.5f, 1.2f, 1 }, { 0, 1, 0, 1 });
	pointlights[2]	= new DXLight(LightTypePoint, { 0.0f, 0.5f, 0.0f, 1 }, { 0, 0, 1, 1 });

	moonlight->CreateShadowMap(device, 512);
	moonlight->SetProjectionParameters(7.1f, 7.1f, -5, 5);

	pointlights[0]->CreateShadowMap(device, 256);
	pointlights[1]->CreateShadowMap(device, 256);
	pointlights[2]->CreateShadowMap(device, 256);

	pointlights[0]->SetProjectionParameters(0, 0, 0.1f, 10.0f);
	pointlights[1]->SetProjectionParameters(0, 0, 0.1f, 10.0f);
	pointlights[2]->SetProjectionParameters(0, 0, 0.1f, 10.0f);

	return true;
}

void UninitScene()
{
	delete moonlight;
	delete pointlights[0];
	delete pointlights[1];
	delete pointlights[2];

	SAFE_RELEASE(marble);
	SAFE_RELEASE(wood);
	SAFE_RELEASE(wood_normal);
	SAFE_RELEASE(sky);

	SAFE_RELEASE(scenesurface);
	SAFE_RELEASE(albedosurface);
	SAFE_RELEASE(normalsurface);
	SAFE_RELEASE(depthsurface);

	SAFE_RELEASE(scenetarget);
	SAFE_RELEASE(albedo);
	SAFE_RELEASE(normals);
	SAFE_RELEASE(depth);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(skull);
	SAFE_RELEASE(box);

	SAFE_RELEASE(shadowmap);
	SAFE_RELEASE(blur);
	SAFE_RELEASE(gbuffer);
	SAFE_RELEASE(deferred);
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

void RenderScene(LPD3DXEFFECT effect, const D3DXMATRIX& viewproj)
{
	D3DXMATRIX	inv;
	D3DXMATRIX	world[4];
	D3DXVECTOR4	uv(1, 1, 1, 1);

	D3DXHANDLE	oldtech	= effect->GetCurrentTechnique();
	D3DXHANDLE	tech	= effect->GetTechniqueByName("gbuffer_tbn");

	// setup world matrices
	D3DXMatrixScaling(&world[0], 0.15f, 0.15f, 0.15f);
	D3DXMatrixScaling(&world[1], 0.15f, 0.15f, 0.15f);
	D3DXMatrixScaling(&world[2], 0.15f, 0.15f, 0.15f);
	D3DXMatrixScaling(&world[3], 5, 0.1f, 5);

	world[0]._41 = -1.5;
	world[0]._43 = 1.5;

	world[1]._41 = 1.5;
	world[1]._43 = 1.5;

	world[2]._41 = 0;
	world[2]._43 = -1;

	world[3]._42 = -0.05f;

	// render
	D3DXMatrixInverse(&inv, NULL, &world[0]);

	effect->SetMatrix("matWorld", &world[0]);
	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matViewProj", &viewproj);
	effect->SetVector("uv", &uv);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		// skull 1
		device->SetTexture(0, marble);
		skull->DrawSubset(0);

		// skull 2
		D3DXMatrixInverse(&inv, NULL, &world[1]);

		effect->SetMatrix("matWorld", &world[1]);
		effect->SetMatrix("matWorldInv", &inv);
		effect->CommitChanges();

		skull->DrawSubset(0);

		// skull 3
		D3DXMatrixInverse(&inv, NULL, &world[2]);

		effect->SetMatrix("matWorld", &world[2]);
		effect->SetMatrix("matWorldInv", &inv);
		effect->CommitChanges();

		skull->DrawSubset(0);
	}
	effect->EndPass();
	effect->End();

	// floor
	if (tech)
		effect->SetTechnique(tech);

	D3DXMatrixInverse(&inv, NULL, &world[3]);
	uv = D3DXVECTOR4(2, 2, 0, 0);

	effect->SetMatrix("matWorldInv", &inv);
	effect->SetMatrix("matWorld", &world[3]);
	effect->SetVector("uv", &uv);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		device->SetTexture(0, wood);
		device->SetTexture(1, wood_normal);

		box->DrawSubset(0);
	}
	effect->EndPass();
	effect->End();

	if (tech)
		effect->SetTechnique(oldtech);
}

void RenderShadowMaps()
{
	// moonlight
	moonlight->RenderShadowMap(device, [&](DXLight* light) {
		D3DXMATRIX viewproj;
		D3DXVECTOR4 clipplanes(light->GetNearPlane(), light->GetFarPlane(), 0, 0);
		
		light->CalculateViewProjection(viewproj);

		shadowmap->SetTechnique("variance");
		shadowmap->SetVector("clipPlanes", &clipplanes);
		shadowmap->SetBool("isPerspective", FALSE);

		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		RenderScene(shadowmap, viewproj);
	});

	moonlight->BlurShadowMap(device, [&](DXLight* light) {
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
			RenderScene(shadowmap, viewproj);
		});
	}

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	// TODO: blur with Monte Carlo
	// NOTE: not really needed
}

void RenderGBuffer(const Math::Matrix& viewproj)
{
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
	device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

	device->SetRenderTarget(0, albedosurface);
	device->SetRenderTarget(1, normalsurface);
	device->SetRenderTarget(2, depthsurface);

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
		RenderScene(gbuffer, (const D3DXMATRIX&)viewproj);
	}
	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

	device->SetRenderTarget(1, NULL);
	device->SetRenderTarget(2, NULL);
}

void DeferredShading(const Math::Matrix& view, const Math::Matrix& proj, const Math::Matrix& viewprojinv, const Math::Vector4& eye)
{
	RECT scissorrect;

	device->SetRenderTarget(0, scenesurface);
	device->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);

	for (int i = 1; i < 5; ++i) {
		device->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	}

	device->SetTexture(0, albedo);
	device->SetTexture(1, normals);
	device->SetTexture(2, depth);

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
	device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	deferred->SetTechnique("deferred");
	deferred->SetMatrix("matViewProjInv", (D3DXMATRIX*)&viewprojinv);
	deferred->SetVector("pixelSize", &pixelsize);
	deferred->SetVector("eyePos", (D3DXVECTOR4*)&eye);

	deferred->Begin(NULL, 0);
	deferred->BeginPass(0);
	{
		D3DXMATRIX lightviewproj;
		D3DXVECTOR4 clipplanes(0, 0, 0, 0);

		moonlight->CalculateViewProjection(lightviewproj);
		
		if (selectedlight == 0) {
			// directional light
			deferred->SetMatrix("lightViewProj", &lightviewproj);
			deferred->SetVector("lightColor", (D3DXVECTOR4*)&moonlight->GetColor());
			deferred->SetVector("lightPos", &moonlight->GetPosition());
			deferred->SetFloat("specularPower", 200.0f);
			deferred->CommitChanges();

			device->SetTexture(3, moonlight->GetShadowMap());
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}

		// point lights
		device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);

		for (int i = 0; i < 3; ++i) {
			clipplanes.x = pointlights[i]->GetNearPlane();
			clipplanes.y = pointlights[i]->GetFarPlane();

			if (selectedlight == 0 || selectedlight - 1 == i) {
				pointlights[i]->CalculateScissorRect(
					scissorrect, (const D3DXMATRIX&)view, (const D3DXMATRIX&)proj,
					POINT_LIGHT_RADIUS, app->GetClientWidth(), app->GetClientHeight());

				device->SetScissorRect(&scissorrect);

				deferred->SetVector("clipPlanes", &clipplanes);
				deferred->SetVector("lightColor", (D3DXVECTOR4*)&pointlights[i]->GetColor());
				deferred->SetVector("lightPos", &pointlights[i]->GetPosition());
				deferred->SetFloat("specularPower", 80.0f);
				deferred->SetFloat("lightRadius", POINT_LIGHT_RADIUS);
				deferred->CommitChanges();

				device->SetTexture(4, pointlights[i]->GetCubeShadowMap());
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
		}

		device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	}
	deferred->EndPass();
	deferred->End();

	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

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

	Math::Matrix	view, viewinv, proj;
	Math::Matrix	viewproj, viewprojinv;
	Math::Vector4	eye;

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);

	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixInverse(viewprojinv, viewproj);

	// calculate moonlight direction (let y stay in world space, so we can see shadow)
	Math::Vector4 moondir = { -0.25f, 0.65f, -1, 0 };

	Math::MatrixInverse(viewinv, view);
	Math::Vec3TransformNormal((Math::Vector3&)moondir, (const Math::Vector3&)moondir, viewinv);
	Math::Vec3Normalize((Math::Vector3&)moondir, (const Math::Vector3&)moondir);

	moondir.y = 0.65f;
	moonlight->SetPosition((const D3DXVECTOR4&)moondir);

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

		// STEP 0: render shadow maps
		device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		{
			RenderShadowMaps();
		}

		// STEP 1: g-buffer pass
		{
			RenderGBuffer(viewproj);
		}
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

		// STEP 2: deferred shading
		pixelsize.x = 1.0f / (float)oldviewport.Width;
		pixelsize.y = -1.0f / (float)oldviewport.Height;

		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);

		DeferredShading(view, proj, viewprojinv, eye);

		// STEP 3: render sky
		device->SetRenderTarget(0, backbuffer);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

		backbuffer->Release();

		screenquad->SetTechnique("screenquad");
		screenquad->Begin(NULL, 0);
		screenquad->BeginPass(0);
		{
			device->SetTexture(0, sky);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		screenquad->EndPass();
		screenquad->End();

		// STEP 4: tone mapping
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

		// render text
		viewport = oldviewport;

		viewport.Width = 1024;
		viewport.Height = 256;
		viewport.X = viewport.Y = 10;

		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
		device->SetViewport(&viewport);

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
