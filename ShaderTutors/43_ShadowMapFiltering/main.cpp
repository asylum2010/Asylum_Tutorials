
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\basiccamera.h"
#include "shadowtechniques.h"

// helper macros
#define TITLE				"Shader sample 43: Shadow map filtering methods"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample structures
struct ObjectInstance
{
	D3DXMATRIX			transform;
	D3DXVECTOR3			position;
	D3DXVECTOR3			scaling;
	D3DXVECTOR3			angles;
	LPD3DXMESH*			mesh;
	LPDIRECT3DTEXTURE9*	texture;
};

// sample variables
Application*		app					= nullptr;
LPDIRECT3DDEVICE9	device				= nullptr;

LPDIRECT3DTEXTURE9	wood				= nullptr;
LPDIRECT3DTEXTURE9	marble				= nullptr;
LPDIRECT3DTEXTURE9	crate				= nullptr;
LPDIRECT3DTEXTURE9	helptext			= nullptr;

LPD3DXMESH			box					= nullptr;
LPD3DXMESH			skull				= nullptr;

LPD3DXEFFECT		screenquad			= nullptr;

BasicCamera			camera;
BasicCamera			light;
Math::AABox			scenebox;

// shadow map filtering techniques
PercentageCloser*	percentagecloser	= new PercentageCloser();
VarianceShadow*		varianceshadow		= new VarianceShadow();
ExpVarianceShadow*	expvarianceshadow	= new ExpVarianceShadow();
ShadowTechnique*	currenttechnique	= percentagecloser;

// object instances
ObjectInstance instances[] =
{
	{ {}, { 0, 0, 0 },					{ 5, 0.1f, 5 },			{ 0, 0, 0 },				&box,	&wood },
	{ {}, { 0, 0.05f, -1.2f },			{ 0.2f, 0.2f, 0.2f },	{ Math::PI - 1.04f, 0, 0 },	&skull,	&marble },

	{ {}, { 0.5f, 0.38f, 1 },			{ 1.2f, 0.75f, 0.75f },	{ Math::PI + 0.3f, 0, 0 },	&box,	&crate },
	{ {}, { 0.55f, 1.005f, 0.85f },		{ 0.5f, 0.5f, 0.5f },	{ Math::PI + 0.3f, 0, 0 },	&box,	&crate },
	{ {}, { -1.75f, 0.38f, -0.4f },		{ 0.75f, 0.75f, 1.5f },	{ Math::PI + 0.15f, 0, 0 },	&box,	&crate }
};

const int NumInstances = sizeof(instances) / sizeof(instances[0]);

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/box.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &box)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/skullocc3.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skull)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/marble.dds", &marble)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/wood2.jpg", &wood)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/crate.jpg", &crate)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device,
		"Mouse left - Orbit camera\nMouse middle - Pan/zoom camera\nMouse right - Rotate light\n\n1 - Unfiltered\n2 - PCF 5x5\n3 - PCF irregular\n4 - PCF soft\n5 - Variance\n6 - Exponential variance",
		512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
	{
		return false;
	}

	if (!percentagecloser->Initialize(device))
		return false;

	if (!varianceshadow->Initialize(device))
		return false;

	if (!expvarianceshadow->Initialize(device))
		return false;

	// setup scene
	Math::AABox		boxbb;
	Math::AABox		skullbb;
	Math::AABox		trafbox;
	D3DXMATRIX		tmp1, tmp2, tmp3;
	D3DXVECTOR3*	vdata = nullptr;

	box->LockVertexBuffer(D3DLOCK_READONLY, (void**)&vdata);
	D3DXComputeBoundingBox(vdata, box->GetNumVertices(), box->GetNumBytesPerVertex(), (D3DXVECTOR3*)&boxbb.Min, (D3DXVECTOR3*)&boxbb.Max);
	box->UnlockVertexBuffer();

	skull->LockVertexBuffer(D3DLOCK_READONLY, (void**)&vdata);
	D3DXComputeBoundingBox(vdata, skull->GetNumVertices(), skull->GetNumBytesPerVertex(), (D3DXVECTOR3*)&skullbb.Min, (D3DXVECTOR3*)&skullbb.Max);
	skull->UnlockVertexBuffer();

	for (int i = 0; i < NumInstances; ++i) {
		ObjectInstance& ot = instances[i];

		D3DXMatrixScaling(&tmp1, ot.scaling.x, ot.scaling.y, ot.scaling.z);
		D3DXMatrixRotationYawPitchRoll(&tmp2, ot.angles.x, ot.angles.y, ot.angles.z);
		D3DXMatrixTranslation(&tmp3, ot.position.x, ot.position.y, ot.position.z);

		D3DXMatrixMultiply(&ot.transform, &tmp1, &tmp2);
		D3DXMatrixMultiply(&ot.transform, &ot.transform, &tmp3);

		if (ot.mesh == &skull)
			trafbox = skullbb;
		else
			trafbox = boxbb;

		trafbox.TransformAxisAligned((const Math::Matrix&)ot.transform);

		scenebox.Add(trafbox.Min);
		scenebox.Add(trafbox.Max);
	}

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 20);
	camera.SetZoomLimits(3, 8);
	camera.SetDistance(5);
	camera.SetPosition(0, 0.5f, 0);
	camera.SetOrientation(Math::DegreesToRadians(45), Math::DegreesToRadians(45), 0);

	// setup light
	light.SetDistance(10);
	light.SetPosition(0, 0, 0);
	light.SetOrientation(Math::DegreesToRadians(183), Math::DegreesToRadians(48.7f), 0);
	light.SetPitchLimits(0.1f, 1.45f);

	return true;
}

void UninitScene()
{
	delete percentagecloser;
	delete varianceshadow;
	delete expvarianceshadow;

	SAFE_RELEASE(wood);
	SAFE_RELEASE(marble);
	SAFE_RELEASE(crate);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(box);
	SAFE_RELEASE(skull);

	SAFE_RELEASE(screenquad);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		currenttechnique = percentagecloser;
		percentagecloser->GetEffect()->SetTechnique("unfiltered");
		break;

	case KeyCode2:
		currenttechnique = percentagecloser;
		percentagecloser->GetEffect()->SetTechnique("pcf5x5");
		break;

	case KeyCode3:
		currenttechnique = percentagecloser;
		percentagecloser->GetEffect()->SetTechnique("pcfirregular");
		break;

	case KeyCode4:
		currenttechnique = percentagecloser;
		percentagecloser->GetEffect()->SetTechnique("pcfsoft");
		break;

	case KeyCode5:
		currenttechnique = varianceshadow;
		break;

	case KeyCode6:
		currenttechnique = expvarianceshadow;
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
		light.OrbitRight(Math::DegreesToRadians(dx));
		light.OrbitUp(Math::DegreesToRadians(dy));
	}
}

void MouseScroll(int32_t x, int32_t y, int16_t dz)
{
	camera.Zoom(dz * 2.0f);
}

void Update(float dt)
{
	camera.Update(dt);
	light.Update(dt);
}

void RenderScene(LPD3DXEFFECT effect)
{
	D3DXMATRIX worldinv;
	D3DXVECTOR4 uv(3, 3, 0, 0);

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	effect->Begin(NULL, 0);
	effect->BeginPass(0);
	{
		for (int i = 0; i < NumInstances; ++i) {
			const ObjectInstance& ot = instances[i];
			D3DXMatrixInverse(&worldinv, NULL, &ot.transform);

			effect->SetMatrix("matWorld", &ot.transform);
			effect->SetMatrix("matWorldInv", &worldinv);
			effect->SetVector("uv", &uv);
			effect->CommitChanges();

			device->SetTexture(0, *ot.texture);
			(*ot.mesh)->DrawSubset(0);

			uv = { -1, 1, 0, 0 };
		}
	}
	effect->EndPass();
	effect->End();

	device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix	view, proj;
	Math::Matrix	viewproj;
	Math::Matrix	lightview;
	Math::Vector4	eye;
	Math::Vector4	lightpos(0, 0, 0, 1);

	// NOTE: camera and light are right-handed
	light.Animate(alpha);
	light.GetViewMatrix(lightview);
	light.GetEyePosition((Math::Vector3&)lightpos);

	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eye);

	Math::MatrixMultiply(viewproj, view, proj);

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;

		device->GetViewport(&oldviewport);

		// render shadow map
		Math::Matrix lightproj;
		Math::Matrix lightviewproj;
		Math::Vector2 lightclip;

		LPD3DXEFFECT currenteffect = currenttechnique->GetEffect();
		LPDIRECT3DSURFACE9 backbuffer = nullptr;

		device->GetRenderTarget(0, &backbuffer);

		currenttechnique->RenderShadowMap(lightview, scenebox, [&](const Math::Matrix& proj, const Math::Vector2& clipplanes) {
			RenderScene(currenteffect);

			lightproj = proj;
			lightclip = clipplanes;
		});

		Math::MatrixMultiply(lightviewproj, lightview, lightproj);

		device->SetRenderTarget(0, backbuffer);
		backbuffer->Release();

		// render scene
		Math::Vector4 black(0, 0, 0, 1);
		Math::Vector4 texelsize(1.0f / ShadowTechnique::ShadowMapSize, 1.0f / ShadowTechnique::ShadowMapSize, 0, 0);
		Math::Vector4 invfrustumsize(0.5f * lightproj._11, 0.5f * lightproj._22, 1, 1);
		Math::Vector4 lightsize = 0.6f * invfrustumsize;

		currenteffect->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		currenteffect->SetMatrix("lightViewProj", (D3DXMATRIX*)&lightviewproj);
		currenteffect->SetVector("eyePos", (D3DXVECTOR4*)&eye);
		currenteffect->SetVector("lightPos", (D3DXVECTOR4*)&lightpos);
		//currenteffect->SetVector("lightAmbient", (D3DXVECTOR4*)&black);
		currenteffect->SetVector("clipPlanes", (D3DXVECTOR4*)&lightclip);
		currenteffect->SetVector("texelSize", (D3DXVECTOR4*)&texelsize);
		currenteffect->SetVector("lightSize", (D3DXVECTOR4*)&lightsize);

		device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

		//device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		//device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);	// as suggested in Shader X5
		device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

		if (currenttechnique == percentagecloser) {
			device->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			device->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
			device->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			device->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

			device->SetTexture(2, percentagecloser->GetNoiseTexture());
		}

		device->SetTexture(1, currenttechnique->GetShadowMap());
		{
			RenderScene(currenteffect);
		}
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
		
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

		// render text
		viewport = oldviewport;

		viewport.Width = 512;
		viewport.Height = 512;
		viewport.X = viewport.Y = 10;

		device->SetViewport(&viewport);
		device->SetTexture(0, helptext);

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
