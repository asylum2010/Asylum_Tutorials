
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\basiccamera.h"

// NOTE: see GPU Gems 3, chapter 17
// NOTE: see http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.87.5002&rep=rep1&type=pdf

// helper macros
#define TITLE				"Shader sample 44: Fresnel equations"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*			app					= nullptr;
LPDIRECT3DDEVICE9		device				= nullptr;

LPD3DXMESH				skymesh				= nullptr;

LPDIRECT3DCUBETEXTURE9	environment			= nullptr;
LPDIRECT3DTEXTURE9		normals				= nullptr;
LPDIRECT3DTEXTURE9		depth				= nullptr;
LPDIRECT3DTEXTURE9		helptext			= nullptr;

LPDIRECT3DSURFACE9		normalsurface		= nullptr;
LPDIRECT3DSURFACE9		depthsurface		= nullptr;

LPD3DXEFFECT			skyeffect			= nullptr;
LPD3DXEFFECT			fresneleffects		= nullptr;
LPD3DXEFFECT			screenquad			= nullptr;

DXObject*				objects[3]			= { nullptr };
BasicCamera				camera;
BasicCamera				objectrotator;
int						currentobject		= 1;

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	objects[0] = new DXObject(device);
	objects[1] = new DXObject(device);
	objects[2] = new DXObject(device);

	if (!objects[0]->Load("../../Media/MeshesDX/teapot.x")) {
		MYERROR("Could not load 'teapot.x'");
		return false;
	}

	if (!objects[1]->Load("../../Media/MeshesDX/knot.x")) {
		MYERROR("Could not load 'knot.x'");
		return false;
	}

	if (!objects[2]->Load("../../Media/MeshesDX/skullocc3.x")) {
		MYERROR("Could not load 'skullocc3.x'");
		return false;
	}

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/sky.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh)))
		return false;

	if (FAILED(D3DXCreateCubeTextureFromFile(device, "../../Media/Textures/mountains_sunny.dds", &environment)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/sky.fx", &skyeffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/fresneleffects.fx", &fresneleffects)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device,
		"Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\nMouse right - Rotate object\n\n1 - Change object\n2 - Reflection\n3 - Single refraction\n4 - Double refraction",
		512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
	{
		return false;
	}

	// render targets
	if (FAILED((device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &normals, NULL))))
		return false;

	if (FAILED((device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_R32F, D3DPOOL_DEFAULT, &depth, NULL))))
		return false;

	normals->GetSurfaceLevel(0, &normalsurface);
	depth->GetSurfaceLevel(0, &depthsurface);

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 20);
	camera.SetZoomLimits(1.2f, 5);
	camera.SetDistance(2.5f);
	camera.SetPosition(0, 0, 0);
	camera.SetOrientation(Math::DegreesToRadians(-124), Math::DegreesToRadians(14), 0);

	objectrotator.SetDistance(0);
	fresneleffects->SetTechnique("doublerefraction");

	return true;
}

void UninitScene()
{
	delete objects[0];
	delete objects[1];
	delete objects[2];

	SAFE_RELEASE(normalsurface);
	SAFE_RELEASE(depthsurface);

	SAFE_RELEASE(environment);
	SAFE_RELEASE(normals);
	SAFE_RELEASE(depth);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(skymesh);

	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(fresneleffects);
	SAFE_RELEASE(screenquad);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		currentobject = (currentobject + 1) % 3;
		break;

	case KeyCode2:
		fresneleffects->SetTechnique("reflection");
		break;

	case KeyCode3:
		fresneleffects->SetTechnique("singlerefraction");
		break;

	case KeyCode4:
		fresneleffects->SetTechnique("doublerefraction");
		break;

	case KeyCode5:
		//fresneleffects->SetTechnique("dispersion");
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
	} else if (state & MouseButtonRight) {
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

void Render(float alpha, float elapsedtime)
{
	D3DXMATRIX		world, worldinv;
	D3DXMATRIX		rotation;

	Math::Matrix	identity(1, 1, 1, 1);
	Math::Matrix	view, proj;
	Math::Matrix	skyview, viewproj, viewprojinv;
	Math::Vector4	eye;
	Math::Vector3	orient;
	Math::Color		color = Math::Color::sRGBToLinear(0.5f, 0.75f, 0.5f, 1);

	objectrotator.Animate(alpha);
	objectrotator.GetOrientation(orient);

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);

	// setup world matrix
	if (currentobject == 0) {
		D3DXMatrixScaling(&world, 0.5f, 0.5f, 0.5f);
	} else if (currentobject == 1) {
		D3DXMatrixScaling(&world, 0.3f, 0.3f, 0.3f);
	} else if (currentobject == 2) {
		D3DXMatrixScaling(&world, 0.15f, 0.15f, 0.15f);
		world._42 = -0.5f;
	}

	D3DXMatrixRotationYawPitchRoll(&rotation, orient.x, orient.y, 0);
	D3DXMatrixMultiply(&world, &world, &rotation);
	D3DXMatrixInverse(&worldinv, NULL, &world);

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;

		device->GetViewport(&oldviewport);

		LPDIRECT3DSURFACE9 backbuffer = nullptr;
		device->GetRenderTarget(0, &backbuffer);

		// render g-buffer for refraction
		D3DXHANDLE oldtechnique = fresneleffects->GetCurrentTechnique();

		device->SetRenderTarget(0, normalsurface);
		device->SetRenderTarget(1, depthsurface);

		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		Math::MatrixMultiply(viewproj, view, proj);
		Math::MatrixInverse(viewprojinv, viewproj);

		fresneleffects->SetTechnique("gbuffer");
		fresneleffects->SetMatrix("matWorld", &world);
		fresneleffects->SetMatrix("matWorldInv", &worldinv);
		fresneleffects->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);

		fresneleffects->Begin(NULL, 0);
		fresneleffects->BeginPass(0);
		{
			objects[currentobject]->Draw(nullptr);
		}
		fresneleffects->EndPass();
		fresneleffects->End();

		fresneleffects->SetTechnique(oldtechnique);

		device->SetRenderTarget(0, backbuffer);
		device->SetRenderTarget(1, nullptr);

		backbuffer->Release();

		// render sky
		skyview = view;
		skyview._41 = skyview._42 = skyview._43 = 0;

		Math::MatrixMultiply(viewproj, skyview, proj);

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

		skyeffect->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		skyeffect->SetMatrix("matSkyRotation", (D3DXMATRIX*)&identity);

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
		fresneleffects->SetMatrix("matViewProjInv", (D3DXMATRIX*)&viewprojinv);
		fresneleffects->SetVector("eyePos", (D3DXVECTOR4*)&eye);
		fresneleffects->SetVector("baseColor", (D3DXVECTOR4*)&color);

		device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_NONE);
		device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_NONE);
		device->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		device->SetTexture(0, environment);
		device->SetTexture(1, normals);
		device->SetTexture(2, depth);

		fresneleffects->Begin(NULL, 0);
		fresneleffects->BeginPass(0);
		{
			objects[currentobject]->Draw(nullptr);
		}
		fresneleffects->EndPass();
		fresneleffects->End();

		// render text
		viewport = oldviewport;

		viewport.Width = 512;
		viewport.Height = 512;
		viewport.X = viewport.Y = 10;

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

		device->SetViewport(&viewport);
		device->SetTexture(0, helptext);

		device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);

		screenquad->Begin(NULL, 0);
		screenquad->BeginPass(0);
		{
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
