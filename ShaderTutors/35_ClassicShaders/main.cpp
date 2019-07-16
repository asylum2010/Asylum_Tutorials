
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\3Dmath.h"

// helper macros
#define TITLE				"Shader sample 35: Classic shaders"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*		app				= nullptr;
LPDIRECT3DDEVICE9	device			= nullptr;
LPDIRECT3DTEXTURE9	checker1		= nullptr;
LPDIRECT3DTEXTURE9	checker2		= nullptr;
LPDIRECT3DTEXTURE9	brick			= nullptr;
LPDIRECT3DTEXTURE9	bricknormal		= nullptr;
LPDIRECT3DTEXTURE9	wavenormal		= nullptr;
LPDIRECT3DTEXTURE9	helptext		= nullptr;

LPD3DXMESH			mesh			= nullptr;
LPD3DXMESH			meshtbn			= nullptr;

LPD3DXEFFECT		screenquad		= nullptr;
LPD3DXEFFECT		basiceffect		= nullptr;
LPD3DXEFFECT		blinnphong		= nullptr;
LPD3DXEFFECT		normalmapping	= nullptr;
LPD3DXEFFECT		water			= nullptr;
LPD3DXEFFECT		effect			= nullptr;

D3DXVECTOR3			eye(0, 0, -1.5f);
D3DXMATRIX			world, view, proj;

bool InitScene()
{
	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/sphere.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &mesh)))
		return false;

	if (FAILED(DXGenTangentFrame(device, mesh, &meshtbn)))
		return false;

	if (FAILED(DXCreateChecker(device, 10, 10, 0xffb200ff, 0xffff6a00, &checker1)))
		return false;

	if (FAILED(DXCreateChecker(device, 10, 10, 0xff8cacff, 0xffe2e2e2, &checker2)))
		return false;

	if (FAILED(D3DXCreateTextureFromFile(device, "../../Media/Textures/brick.jpg", &brick)))
		return false;

	if (FAILED(D3DXCreateTextureFromFile(device, "../../Media/Textures/brick_normal.jpg", &bricknormal)))
		return false;

	if (FAILED(D3DXCreateTextureFromFile(device, "../../Media/Textures/wave.jpg", &wavenormal)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/basic.fx", &basiceffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/blinnphong.fx", &blinnphong)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/normalmapping.fx", &normalmapping)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/projectedwater.fx", &water)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device, "1 - Basic\n2 - Blinn-Phong\n3 - Normal mapping\n4 - Water-like effect", 512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
		return false;

	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)app->GetClientWidth() / (float)app->GetClientHeight(), 1, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);

	effect = basiceffect;
	return true;
}

void UninitScene()
{
	SAFE_RELEASE(checker1);
	SAFE_RELEASE(checker2);
	SAFE_RELEASE(brick);
	SAFE_RELEASE(bricknormal);
	SAFE_RELEASE(wavenormal);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(mesh);
	SAFE_RELEASE(meshtbn);

	SAFE_RELEASE(screenquad);
	SAFE_RELEASE(basiceffect);
	SAFE_RELEASE(blinnphong);
	SAFE_RELEASE(normalmapping);
	SAFE_RELEASE(water);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:	effect = basiceffect;	break;
	case KeyCode2:	effect = blinnphong;	break;
	case KeyCode3:	effect = normalmapping;	break;
	case KeyCode4:	effect = water;			break;
	default:		break;
	}
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	D3DXMATRIX worldviewproj;
	D3DXMATRIX viewproj;
	D3DXMATRIX worldinv;
	D3DXVECTOR3 axis(0, 1, 0);

	time += elapsedtime;

	if (effect == water)
		D3DXMatrixIdentity(&world);
	else
		D3DXMatrixRotationAxis(&world, &axis, time);

	D3DXMatrixInverse(&worldinv, nullptr, &world);

	D3DXMatrixMultiply(&worldviewproj, &world, &view);
	D3DXMatrixMultiply(&worldviewproj, &worldviewproj, &proj);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	effect->SetMatrix("matWVP", &worldviewproj);
	effect->SetMatrix("matWorld", &world);
	effect->SetMatrix("matWorldInv", &worldinv);
	effect->SetMatrix("matViewProj", &viewproj);

	effect->SetVector("eyePos", (D3DXVECTOR4*)&eye);

	device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
	device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

	if (effect == blinnphong) {
		D3DXVECTOR4 uv =  { 1, 1, 0, 0 };
		effect->SetVector("uv", (D3DXVECTOR4*)&uv);

		device->SetTexture(0, checker1);
	} else if (effect == normalmapping) {
		D3DXVECTOR4 uv =  { 2, 1, 0, 0 };
		effect->SetVector("uv", (D3DXVECTOR4*)&uv);

		device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		device->SetTexture(0, brick);
		device->SetTexture(1, bricknormal);
	} else if (effect == water) {
		effect->SetFloat("time", time);

		device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		device->SetTexture(0, checker2);
		device->SetTexture(1, wavenormal);
	}

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;

		device->GetViewport(&oldviewport);

		// render scene
		effect->Begin(NULL, 0);
		effect->BeginPass(0);
		{
			if (effect == normalmapping || effect == water)
				meshtbn->DrawSubset(0);
			else
				mesh->DrawSubset(0);
		}
		effect->EndPass();
		effect->End();

		// render text
		viewport = oldviewport;

		viewport.Width = 512;
		viewport.Height = 512;
		viewport.X = viewport.Y = 10;

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
	app->RenderCallback = Render;
	app->KeyUpCallback = KeyUp;

	app->Run();
	delete app;

	return 0;
}
