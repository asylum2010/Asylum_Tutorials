
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\3Dmath.h"

// helper macros
#define TITLE				"Shader sample 38: Skeletal animation"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*		app				= nullptr;
DXAnimatedMesh*		mesh			= nullptr;
LPDIRECT3DDEVICE9	device			= nullptr;
LPDIRECT3DTEXTURE9	helptext		= nullptr;

LPD3DXEFFECT		screenquad		= nullptr;
LPD3DXEFFECT		effect			= nullptr;

D3DXVECTOR3			eye(0.5f, 1.0f, -1.5f);
D3DXMATRIX			world, view, proj;

bool InitScene()
{
	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/skinning.fx", &effect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device, "Press space for next animation", 512, 512, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
		return false;

	// load animated model
	mesh = new DXAnimatedMesh();
	mesh->SetSkinningMethod(SkinningMethodShader, effect);
	
	if (FAILED(mesh->Load(device, "../../Media/MeshesDX/dwarf/dwarf.x"))) {
		MYERROR("Could not load 'dwarf.x'");
		return false;
	}

	// all the possible skins are contained in this file => turn off some
	mesh->SetAnimation(6);
	mesh->EnableFrame("LOD0_attachment_weapon3", false);
	mesh->EnableFrame("LOD0_attachment_weapon2", false);
	mesh->EnableFrame("LOD0_attachment_shield3", false);
	mesh->EnableFrame("LOD0_attachment_shield2", false);
	mesh->EnableFrame("LOD0_attachment_head2", false);
	mesh->EnableFrame("LOD0_attachment_head1", false);
	mesh->EnableFrame("LOD0_attachment_torso2", false);
	mesh->EnableFrame("LOD0_attachment_torso3", false);
	mesh->EnableFrame("LOD0_attachment_legs2", false);
	mesh->EnableFrame("LOD0_attachment_legs3", false);
	mesh->EnableFrame("LOD0_attachment_Lpads1", false);
	mesh->EnableFrame("LOD0_attachment_Lpads2", false);
	mesh->EnableFrame("Rshoulder", false);

	D3DXVECTOR3 look(0, 0.7f, 0);
	D3DXVECTOR3 up(0, 1, 0);

	D3DXMatrixPerspectiveFovLH(&proj, D3DX_PI / 3, (float)app->GetClientWidth() / (float)app->GetClientHeight(), 0.1f, 10);
	D3DXMatrixLookAtLH(&view, &eye, &look, &up);

	return true;
}

void UninitScene()
{
	if (mesh != nullptr)
		delete mesh;

	SAFE_RELEASE(helptext);

	SAFE_RELEASE(effect);
	SAFE_RELEASE(screenquad);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCodeSpace:
		mesh->NextAnimation();
		break;

	default:
		break;
	}
}

void Render(float alpha, float elapsedtime)
{
	D3DXMATRIX viewproj;

	D3DXMatrixMultiply(&viewproj, &view, &proj);
	D3DXMatrixScaling(&world, 0.1f, 0.1f, 0.1f);

	effect->SetMatrix("matViewProj", &viewproj);

	device->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);
	device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, Math::Color::sRGBToLinear(0xff6694ed), 1.0f, 0);

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;

		device->GetViewport(&oldviewport);

		device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, TRUE);
		device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

		mesh->Update(elapsedtime, &world);
		mesh->Draw();

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
