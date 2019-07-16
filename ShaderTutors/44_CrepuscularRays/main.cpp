
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\basiccamera.h"

// helper macros
#define TITLE				"Shader sample 44: God rays & Fresnel water"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

enum RenderPass
{
	RenderPassOccluders = 0,
	RenderPassReflection,
	RenderPassScene,
	RenderPassWater
};

// sample variables
Application*			app					= nullptr;
LPDIRECT3DDEVICE9		device				= nullptr;

LPD3DXMESH				skymesh				= nullptr;

LPDIRECT3DCUBETEXTURE9	environment			= nullptr;
LPDIRECT3DTEXTURE9		sand				= nullptr;
LPDIRECT3DTEXTURE9		waves				= nullptr;
LPDIRECT3DTEXTURE9		occluders			= nullptr;
LPDIRECT3DTEXTURE9		godrays				= nullptr;
LPDIRECT3DTEXTURE9		scene				= nullptr;
LPDIRECT3DTEXTURE9		refraction			= nullptr;
LPDIRECT3DTEXTURE9		reflection			= nullptr;
LPDIRECT3DTEXTURE9		helptext			= nullptr;

LPDIRECT3DSURFACE9		occludersurface		= nullptr;
LPDIRECT3DSURFACE9		godraysurface		= nullptr;
LPDIRECT3DSURFACE9		scenesurface		= nullptr;
LPDIRECT3DSURFACE9		refractsurface		= nullptr;
LPDIRECT3DSURFACE9		reflectsurface		= nullptr;

LPD3DXEFFECT			skyeffect			= nullptr;
LPD3DXEFFECT			simplecolor			= nullptr;
LPD3DXEFFECT			ambient				= nullptr;
LPD3DXEFFECT			blinnphong			= nullptr;
LPD3DXEFFECT			water				= nullptr;
LPD3DXEFFECT			radialblur			= nullptr;
LPD3DXEFFECT			combine				= nullptr;
LPD3DXEFFECT			screenquad			= nullptr;

DXObject*				palm				= nullptr;
DXObject*				sandplane			= nullptr;
DXObject*				waterplane			= nullptr;
BasicCamera				camera;
bool					drawtext			= true;

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	palm = new DXObject(device);
	sandplane = new DXObject(device);
	waterplane = new DXObject(device);

	if (!palm->Load("../../Media/MeshesQM/palm.qm")) {
		MYERROR("Could not load 'palm.qm'");
		return false;
	}

	sandplane->Create([&](LPD3DXMESH* mesh) -> bool {
		return SUCCEEDED(DXCreatePlane(device, 50, 50, mesh));
	});

	sandplane->AddBaseTexture("../../Media/Textures/sand.jpg");

	waterplane->Create([&](LPD3DXMESH* mesh) -> bool {
		return SUCCEEDED(DXCreatePlane(device, 50, 50, mesh));
	});

	waterplane->GenerateTangentFrame();
	waterplane->AddNormalMap("../../Media/Textures/wave2.png");

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/sky.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh)))
		return false;

	if (FAILED(D3DXCreateCubeTextureFromFile(device, "../../Media/Textures/mountains_sunny.dds", &environment)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/sky.fx", &skyeffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/simplecolor.fx", &simplecolor)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/ambient.fx", &ambient)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/blinnphong.fx", &blinnphong)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/simplewater.fx", &water)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/radialblur.fx", &radialblur)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/godraycombine.fx", &combine)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device, "Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\n\nH - Toggle help text", 512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext))) {
		return false;
	}

	// render targets
	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8, D3DPOOL_DEFAULT, &occluders, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8, D3DPOOL_DEFAULT, &godrays, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &scene, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &refraction, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &reflection, NULL)))
		return false;

	occluders->GetSurfaceLevel(0, &occludersurface);
	godrays->GetSurfaceLevel(0, &godraysurface);
	scene->GetSurfaceLevel(0, &scenesurface);
	refraction->GetSurfaceLevel(0, &refractsurface);
	reflection->GetSurfaceLevel(0, &reflectsurface);

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(90));
	camera.SetClipPlanes(0.1f, 80);
	camera.SetZoomLimits(3, 8);
	camera.SetDistance(5);
	camera.SetPosition(0, 1.2f, 0);
	camera.SetOrientation(-2.81870556f, 0.506145358f, 0);
	camera.SetPitchLimits(-0.4f, Math::HALF_PI);

	return true;
}

void UninitScene()
{
	delete palm;
	delete sandplane;
	delete waterplane;

	SAFE_RELEASE(occludersurface);
	SAFE_RELEASE(godraysurface);
	SAFE_RELEASE(scenesurface);
	SAFE_RELEASE(refractsurface);
	SAFE_RELEASE(reflectsurface);

	SAFE_RELEASE(occluders);
	SAFE_RELEASE(godrays);
	SAFE_RELEASE(scene);
	SAFE_RELEASE(refraction);
	SAFE_RELEASE(reflection);
	SAFE_RELEASE(environment);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(skymesh);

	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(simplecolor);
	SAFE_RELEASE(ambient);
	SAFE_RELEASE(blinnphong);
	SAFE_RELEASE(water);
	SAFE_RELEASE(radialblur);
	SAFE_RELEASE(combine);
	SAFE_RELEASE(screenquad);
}

void KeyUp(KeyCode key)
{
	switch (key) {
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
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float dt)
{
	camera.Update(dt);
}

void RenderScene(RenderPass pass)
{
	Math::Matrix world, worldinv;
	LPD3DXEFFECT effect = blinnphong;

	if (pass != RenderPassWater) {
		if (pass != RenderPassOccluders) {
			// render sky
			device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
			device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
			device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

			device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
			device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

			skyeffect->Begin(NULL, 0);
			skyeffect->BeginPass(0);
			{
				device->SetTexture(0, environment);
				skymesh->DrawSubset(0);
			}
			skyeffect->EndPass();
			skyeffect->End();

			device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		} else {
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			effect = simplecolor;
		}

		if (pass == RenderPassReflection)
			device->SetRenderState(D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);

		// palm
		Math::MatrixScaling(world, 6, 6, 6);
		Math::MatrixInverse(worldinv, world);

		world._42 = -2;

		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

		effect->SetMatrix("matWorld", (D3DXMATRIX*)&world);
		effect->SetMatrix("matWorldInv", (D3DXMATRIX*)&worldinv);

		effect->Begin(0, 0);
		effect->BeginPass(0);
		{
			palm->Draw([&](DXObject* object, DWORD subset) -> bool {
				device->SetTexture(0, object->GetBaseTexture(subset));
				return true;
			});
		}
		effect->EndPass();
		effect->End();

		if (pass != RenderPassOccluders) {
			// sand
			D3DXVECTOR4 uv(10, 10, 0, 0);
			Math::MatrixTranslation(world, 0, -2, 0);

			ambient->SetMatrix("matWorld", (D3DXMATRIX*)&world);
			ambient->SetVector("uv", &uv);

			ambient->Begin(0, 0);
			ambient->BeginPass(0);
			{
				sandplane->Draw([&](DXObject* object, DWORD subset) -> bool {
					device->SetTexture(0, object->GetBaseTexture(subset));
					return true;
				});
			}
			ambient->EndPass();
			ambient->End();
		}

		if (pass == RenderPassScene) {
			// render water surface into alpha channel (to mask out refraction)
			Math::MatrixTranslation(world, 0, -1, 0);

			device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA);

			simplecolor->SetMatrix("matWorld", (D3DXMATRIX*)&world);
			simplecolor->Begin(0, 0);
			simplecolor->BeginPass(0);
			{
				waterplane->Draw(nullptr);
			}
			simplecolor->EndPass();
			simplecolor->End();

			device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
		}

		if (pass == RenderPassReflection)
			device->SetRenderState(D3DRS_CLIPPLANEENABLE, 0);
	} else {
		// final water rendering
		D3DXVECTOR4 uv(6, 6, 0, 0);

		Math::MatrixTranslation(world, 0, -1, 0);
		Math::MatrixIdentity(worldinv);

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

		device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, TRUE);
		device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		device->SetSamplerState(2, D3DSAMP_SRGBTEXTURE, TRUE);
		device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		water->SetMatrix("matWorld", (D3DXMATRIX*)&world);
		water->SetMatrix("matWorldInv", (D3DXMATRIX*)&worldinv);
		water->SetVector("uv", &uv);

		water->Begin(0, 0);
		water->BeginPass(0);
		{
			waterplane->Draw([&](DXObject* object, DWORD subset) -> bool {
				device->SetTexture(0, object->GetNormalMap(subset));
				device->SetTexture(1, refraction);
				device->SetTexture(2, reflection);

				return true;
			});
		}
		water->EndPass();
		water->End();

		device->SetSamplerState(1, D3DSAMP_SRGBTEXTURE, FALSE);
	}
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	Math::Matrix	identity(1, 1, 1, 1);
	Math::Matrix	view, proj;
	Math::Matrix	skyview, viewproj, viewprojinv;
	Math::Vector4	eye;
	Math::Vector4	lightpos(-60, 35, 100, 1);
	Math::Color		lightcolor = Math::Color::sRGBToLinear(1, 1, 0.85f, 1);

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;
		D3DXVECTOR4 black(0, 0, 0, 0);

		device->GetViewport(&oldviewport);

		LPDIRECT3DSURFACE9 backbuffer = nullptr;
		device->GetRenderTarget(0, &backbuffer);

		// common uniforms
		skyeffect->SetMatrix("matSkyRotation", (D3DXMATRIX*)&identity);
		simplecolor->SetVector("matColor", &black);

		blinnphong->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);
		blinnphong->SetVector("lightColor", (D3DXVECTOR4*)&lightcolor);

		water->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);
		water->SetVector("lightColor", (D3DXVECTOR4*)&lightcolor);
		water->SetFloat("time", time);

		// calculate flipped view matrix
		Math::Matrix flippedview;
		Math::Vector4 plane(0, 1, 0, 1);
		Math::Vector3 look;
		Math::Vector3 refleye, refllook;
		Math::Vector3 scalednorm;

		camera.GetPosition(look);

		scalednorm = (Math::Vector3&)plane;
		scalednorm *= (2 * Math::PlaneDotCoord(plane, (Math::Vector3&)eye));
		
		Math::Vec3Subtract(refleye, (Math::Vector3&)eye, scalednorm);

		scalednorm = (Math::Vector3&)plane;
		scalednorm *= (2 * Math::PlaneDotCoord(plane, (Math::Vector3&)look));

		Math::Vec3Subtract(refllook, (Math::Vector3&)look, scalednorm);

		// transform plane to clip space
		Math::MatrixLookAtRH(flippedview, refleye, refllook, Math::Vector3(0, 1, 0));
		Math::MatrixMultiply(viewproj, flippedview, proj);
		
		Math::MatrixInverse(viewprojinv, viewproj);
		Math::MatrixTranspose(viewprojinv, viewprojinv);
		Math::PlaneTransform(plane, plane, viewprojinv);

		// render reflection texture
		blinnphong->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		blinnphong->SetVector("eyePos", (D3DXVECTOR4*)&refleye);

		water->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		water->SetVector("eyePos", (D3DXVECTOR4*)&refleye);

		simplecolor->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		ambient->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);

		skyview = flippedview;
		skyview._41 = skyview._42 = skyview._43 = 0;

		Math::MatrixMultiply(viewproj, skyview, proj);
		skyeffect->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);

		device->SetRenderTarget(0, reflectsurface);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);
		device->SetClipPlane(0, &plane.x);

		RenderScene(RenderPassReflection);

		// reset original view transform
		skyview = view;
		skyview._41 = skyview._42 = skyview._43 = 0;

		Math::MatrixMultiply(viewproj, skyview, proj);
		skyeffect->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		
		Math::MatrixMultiply(viewproj, view, proj);

		blinnphong->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		blinnphong->SetVector("eyePos", (D3DXVECTOR4*)&eye);

		water->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		water->SetVector("eyePos", (D3DXVECTOR4*)&eye);

		simplecolor->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		ambient->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);

		// render occluder texture
		device->SetRenderTarget(0, occludersurface);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);

		RenderScene(RenderPassOccluders);

		// render scene
		device->SetRenderTarget(0, scenesurface);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

		RenderScene(RenderPassScene);

		// copy rendered scene to another target
		device->StretchRect(scenesurface, NULL, refractsurface, NULL, D3DTEXF_POINT);

		// render water surface
		RenderScene(RenderPassWater);

		// radial blur
		D3DXVECTOR4 pixelsize(1.0f / app->GetClientWidth(), -1.0f / app->GetClientHeight(), 0, 0);
		D3DXVECTOR4 texelsize(1.0f / app->GetClientWidth(), 1.0f / app->GetClientHeight(), 0, 0);

		Math::Vector4 lightposNDC;
		Math::Vector3 lightdir, viewdir(-view._13, -view._23, -view._33);

		Math::Vec3Normalize(lightdir, (Math::Vector3&)lightpos);
		float exposure = Math::Min<float>(Math::Max<float>(Math::Vec3Dot(viewdir, lightdir), 0), 1);

		Math::Vec4Transform(lightposNDC, lightpos, viewproj);

		lightposNDC.x = (1.0f + lightposNDC.x / lightposNDC.w) * 0.5f;
		lightposNDC.y = (1.0f - lightposNDC.y / lightposNDC.w) * 0.5f;

		device->SetRenderTarget(0, godraysurface);

		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		radialblur->SetVector("pixelSize", &pixelsize);
		radialblur->SetVector("texelSize", &texelsize);
		radialblur->SetVector("center", (D3DXVECTOR4*)&lightposNDC);
		radialblur->SetFloat("exposure", exposure);

		radialblur->Begin(NULL, 0);
		radialblur->BeginPass(0);
		{
			device->SetTexture(0, occluders);
			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		radialblur->EndPass();
		radialblur->End();

		// combine
		device->SetRenderTarget(0, backbuffer);
		backbuffer->Release();

		device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);

		device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		combine->SetVector("pixelSize", &pixelsize);
		combine->SetVector("lightColor", (D3DXVECTOR4*)&lightcolor);

		combine->Begin(NULL, 0);
		combine->BeginPass(0);
		{
			device->SetTexture(0, scene);
			device->SetTexture(1, godrays);

			device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
		}
		combine->EndPass();
		combine->End();

		if (drawtext) {
			// render text
			viewport = oldviewport;

			viewport.Width = 512;
			viewport.Height = 512;
			viewport.X = viewport.Y = 10;

			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

			device->SetViewport(&viewport);
			device->SetTexture(0, helptext);

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
