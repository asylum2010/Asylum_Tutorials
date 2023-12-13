
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\3Dmath.h"

// helper macros
#define TITLE				"Shader sample 37: Cel shading"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

#define CLIP_NEAR			0.5f
#define CLIP_FAR			3.0f

// sample variables
Application*		app				= nullptr;
LPDIRECT3DDEVICE9	device			= nullptr;
LPDIRECT3DTEXTURE9	colortarget		= nullptr;
LPDIRECT3DTEXTURE9	normaltarget	= nullptr;
LPDIRECT3DTEXTURE9	edgetarget		= nullptr;
LPDIRECT3DTEXTURE9	intensity		= nullptr;
LPDIRECT3DTEXTURE9	helptext		= nullptr;

LPDIRECT3DSURFACE9	colorsurface	= nullptr;
LPDIRECT3DSURFACE9	normalsurface	= nullptr;
LPDIRECT3DSURFACE9	edgesurface		= nullptr;

LPD3DXMESH			teapot			= nullptr;
LPD3DXMESH			knot			= nullptr;
LPD3DXMESH			mesh			= nullptr;

LPD3DXEFFECT		screenquad		= nullptr;
LPD3DXEFFECT		celshading		= nullptr;

D3DXVECTOR3			eye(0.5f, 0.5f, -1.5f);
D3DXMATRIX			world, view, proj;
bool				stencilcountours = false;

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/teapot.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &teapot)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/knot.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &knot)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/celshading.fx", &celshading)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device, "0 - Change object\n1 - Sobel Filter\n2 - Stencil countours", 512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
		return false;

	// create render targets
	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &colortarget, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &normaltarget, NULL)))
		return false;

	if (FAILED(device->CreateTexture(screenwidth, screenheight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &edgetarget, NULL)))
		return false;

	colortarget->GetSurfaceLevel(0, &colorsurface);
	normaltarget->GetSurfaceLevel(0, &normalsurface);
	edgetarget->GetSurfaceLevel(0, &edgesurface);

	// create intensity ramp
	if (FAILED(device->CreateTexture(8, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &intensity, NULL)))
		return false;

	D3DLOCKED_RECT rect;

	intensity->LockRect(0, &rect, NULL, 0);
	{
		uint32_t* data = (uint32_t*)rect.pBits;

		data[0] = 0xff7f7f7f;	// < 0.375 -> 0.5
		data[1] = 0xff7f7f7f;
		data[2] = 0xff7f7f7f;

		data[3] = 0xffafafaf;	// < 0.625 -> 0.68
		data[4] = 0xffafafaf;

		data[5] = 0xffffffff;	// >= 0.625 NextPow-> 1.0
		data[6] = 0xffffffff;
		data[7] = 0xffffffff;
	}
	intensity->UnlockRect(0);

	D3DXVECTOR3 look(0, 0, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)screenwidth / (float)screenheight, CLIP_NEAR, CLIP_FAR);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);

	mesh = knot;
	return true;
}

void UninitScene()
{
	SAFE_RELEASE(colorsurface);
	SAFE_RELEASE(normalsurface);
	SAFE_RELEASE(edgesurface);

	SAFE_RELEASE(colortarget);
	SAFE_RELEASE(normaltarget);
	SAFE_RELEASE(edgetarget);

	SAFE_RELEASE(intensity);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(teapot);
	SAFE_RELEASE(knot);

	SAFE_RELEASE(screenquad);
	SAFE_RELEASE(celshading);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode0:
		mesh = ((mesh == knot) ? teapot : knot);
		break;

	case KeyCode1:
		stencilcountours = false;
		break;
	case KeyCode2:
		stencilcountours = true;
		break;

	default:
		break;
	}
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	D3DXMATRIX	worldinv;
	D3DXMATRIX	viewproj;
	D3DXVECTOR3	axis(0, 1, 0);
	D3DXVECTOR4	texelsize(1.0f / (float)app->GetClientWidth(), 1.0f / (float)app->GetClientHeight(), 0, 1);
	D3DXVECTOR4	clipplanes(CLIP_NEAR, CLIP_FAR, 0, 0);
	Math::Color	color;

	time += elapsedtime;

	if (mesh == knot) {
		D3DXMatrixScaling(&world, 0.3f, 0.3f, 0.3f);
		color = Math::Color::sRGBToLinear(0xff77ff70);
	} else {
		D3DXMatrixScaling(&world, 0.6f, 0.6f, 0.6f);
		color = Math::Color::sRGBToLinear(0xffff7700);
	}

	D3DXMatrixRotationAxis(&worldinv, &axis, time);
	D3DXMatrixMultiply(&world, &world, &worldinv);
	D3DXMatrixInverse(&worldinv, nullptr, &world);
	D3DXMatrixMultiply(&viewproj, &view, &proj);

	celshading->SetMatrix("matWorld", &world);
	celshading->SetMatrix("matWorldInv", &worldinv);
	celshading->SetMatrix("matViewProj", &viewproj);

	celshading->SetVector("baseColor", (D3DXVECTOR4*)&color);
	celshading->SetVector("texelSize", &texelsize);
	celshading->SetVector("clipPlanes", &clipplanes);

	LPDIRECT3DSURFACE9 backbuffer = nullptr;

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;

		device->GetViewport(&oldviewport);

		// render scene + normals/depth
		celshading->SetTechnique("celshading");

		if (!stencilcountours) {
			device->GetRenderTarget(0, &backbuffer);
			device->SetRenderTarget(0, colorsurface);
			device->SetRenderTarget(1, normalsurface);
		}

		device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
		device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

		celshading->Begin(NULL, 0);
		celshading->BeginPass(0);
		{
			device->SetTexture(0, intensity);
			mesh->DrawSubset(0);
		}
		celshading->EndPass();
		celshading->End();

		if (!stencilcountours) {
			// detect edges
			celshading->SetTechnique("edgedetect");

			device->SetRenderTarget(0, edgesurface);
			device->SetRenderTarget(1, nullptr);

			device->Clear(0, NULL, D3DCLEAR_TARGET, 0xffffffff, 1.0f, 0);
			device->SetRenderState(D3DRS_ZENABLE, FALSE);
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);

			celshading->Begin(NULL, 0);
			celshading->BeginPass(0);
			{
				device->SetTexture(0, normaltarget);
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
			celshading->EndPass();
			celshading->End();

			// compose
			celshading->SetTechnique("compose");

			device->SetRenderTarget(0, backbuffer);
			device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
			device->Clear(0, NULL, D3DCLEAR_TARGET, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

			backbuffer->Release();

			device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			device->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_NONE);

			celshading->Begin(NULL, 0);
			celshading->BeginPass(0);
			{
				device->SetTexture(0, colortarget);
				device->SetTexture(1, edgetarget);

				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVertices, 6 * sizeof(float));
			}
			celshading->EndPass();
			celshading->End();
		} else {
			D3DXMATRIX offproj;

			// use the stencil buffer
			device->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
			device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			device->SetRenderState(D3DRS_STENCILENABLE, TRUE);
			device->SetRenderState(D3DRS_ZENABLE, FALSE);

			device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
			device->SetRenderState(D3DRS_STENCILREF, 1);
			device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);

			device->SetTransform(D3DTS_WORLD, &world);
			device->SetTransform(D3DTS_VIEW, &view);

			float thickness = 3.5f;

			// render object 4 times with offseted frusta
			for (float i = -thickness; i < thickness + 1; i += 2 * thickness) {
				for (float j = -thickness; j < thickness + 1; j += 2 * thickness) {
					D3DXMatrixTranslation(&offproj, i / (float)app->GetClientWidth(), j / (float)app->GetClientHeight(), 0);
					D3DXMatrixMultiply(&offproj, &proj, &offproj);

					device->SetTransform(D3DTS_PROJECTION, &offproj);
					mesh->DrawSubset(0);
				}
			}

			// erase area in the center
			device->SetRenderState(D3DRS_STENCILREF, 0);
			device->SetTransform(D3DTS_PROJECTION, &proj);

			mesh->DrawSubset(0);

			// now render outlines
			device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED|D3DCOLORWRITEENABLE_GREEN|D3DCOLORWRITEENABLE_BLUE|D3DCOLORWRITEENABLE_ALPHA);
			device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
			device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
			device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);

			device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);

			DXScreenQuadVerticesFFP[6] = DXScreenQuadVerticesFFP[18]	= (float)app->GetClientWidth() - 0.5f;
			DXScreenQuadVerticesFFP[13] = DXScreenQuadVerticesFFP[19]	= (float)app->GetClientHeight() - 0.5f;

			device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CONSTANT);
			device->SetTextureStageState(0, D3DTSS_CONSTANT, 0);
			{
				device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, DXScreenQuadVerticesFFP, 6 * sizeof(float));
			}
			device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

			device->SetRenderState(D3DRS_ZENABLE, TRUE);
			device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		}

		// render text
		viewport = oldviewport;

		viewport.Width = 512;
		viewport.Height = 512;
		viewport.X = viewport.Y = 10;

		device->SetViewport(&viewport);
		device->SetTexture(0, helptext);

		device->SetRenderState(D3DRS_ZENABLE, FALSE);
		device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
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
	app->RenderCallback = Render;
	app->KeyUpCallback = KeyUp;

	app->Run();
	delete app;

	return 0;
}
