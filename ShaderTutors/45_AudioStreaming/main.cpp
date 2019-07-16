
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

#include "..\Common\application.h"
#include "..\Common\dx9ext.h"
#include "..\Common\xa2ext.h"
#include "..\Common\basiccamera.h"
#include "..\Common\particlesystem.h"
#include "..\Common\lightning.h"

#define LIGHTNING_THICKNESS	3e-2f	// lightning is around 2-3 cm in diameter

// helper macros
#define TITLE				"Shader sample 45: Storm, Earth and Fire...heed my call!"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }
#define SAFE_RELEASE(x)		{ if ((x)) { (x)->Release(); (x) = NULL; } }

// sample variables
Application*			app					= nullptr;
LPDIRECT3DDEVICE9		device				= nullptr;

LPD3DXMESH				skymesh				= nullptr;
LPD3DXMESH				box					= nullptr;

LPDIRECT3DCUBETEXTURE9	environment			= nullptr;
LPDIRECT3DTEXTURE9		fire				= nullptr;
LPDIRECT3DTEXTURE9		stones				= nullptr;
LPDIRECT3DTEXTURE9		helptext			= nullptr;

LPD3DXEFFECT			skyeffect			= nullptr;
LPD3DXEFFECT			ambient				= nullptr;
LPD3DXEFFECT			gradientcolor		= nullptr;
LPD3DXEFFECT			modulate			= nullptr;
LPD3DXEFFECT			skinning			= nullptr;
LPD3DXEFFECT			screenquad			= nullptr;

D3DXMATRIX				dwarftransforms[6];
DXAnimatedMesh*			dwarfs[6]			= { nullptr };
DXParticleStorage*		firestorage			= nullptr;
DXParticleStorage*		lightningstorage	= nullptr;
ParticleSystem*			firesystem			= nullptr;

XA2AudioStreamer*		audiostreamer		= nullptr;
XA2Sound*				music				= nullptr;
XA2Sound*				burning				= nullptr;
XA2Sound*				thunder				= nullptr;

Lightning*				lightning;
BasicCamera				camera;
bool					drawtext			= false;

static void DwarfSkin(int id, char weapon, char shield, char head, char torso, char legs, char lpads, char rpads)
{
#define DISABLE_IF_SET(var, str) \
	if (var != 1) dwarfs[id]->EnableFrame(str##"1", false); \
	if (var != 2) dwarfs[id]->EnableFrame(str##"2", false); \
	if (var != 3) dwarfs[id]->EnableFrame(str##"3", false);
// END

	DISABLE_IF_SET(weapon, "LOD0_attachment_weapon");
	DISABLE_IF_SET(shield, "LOD0_attachment_shield");
	DISABLE_IF_SET(head, "LOD0_attachment_head");
	DISABLE_IF_SET(torso, "LOD0_attachment_torso");
	DISABLE_IF_SET(legs, "LOD0_attachment_legs");
	DISABLE_IF_SET(lpads, "LOD0_attachment_Lpads");
	DISABLE_IF_SET(rpads, "LOD0_attachment_Rpads");

	dwarfs[id]->EnableFrame("Rshoulder", false);
}

static void LightningStrike()
{
	const int NumSegmentVertices = 5;

	auto RandomXZ = [](float diameter) -> float {
		return -0.5f * diameter + Math::RandomFloat() * diameter;
	};

	// choose a point on the ground first as lightning is mostly vertical
	Math::Vector3 end(RandomXZ(4.75f), 0, RandomXZ(4.75f));
	Math::Vector3 start(end.x + RandomXZ(2.5f), 4, end.z + RandomXZ(2.5f));

	// subdivide start and end to multiple segments
	std::vector<Math::Vector3> subsegments;
	subsegments.reserve(NumSegmentVertices);

	for (int i = 0; i < NumSegmentVertices - 1; ++i) {
		float t1 = (float)i / (NumSegmentVertices - 1);
		float t2 = (float)(i + 1) / (NumSegmentVertices - 1);

		subsegments.push_back((1 - t1) * start + t1 * end);
		subsegments.push_back((1 - t2) * start + t2 * end);
	}

	lightning->Reset();
	lightning->Initialize(lightningstorage, subsegments, 5, 0x08);
}

bool InitScene()
{
	srand((uint32_t)time(NULL));

	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/sky.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &skymesh)))
		return false;

	if (FAILED(D3DXLoadMeshFromX("../../Media/MeshesDX/box.x", D3DXMESH_MANAGED, device, NULL, NULL, NULL, NULL, &box)))
		return false;

	if (FAILED(D3DXCreateCubeTextureFromFile(device, "../../Media/Textures/evening_cloudy.dds", &environment)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/fire.png", &fire)))
		return false;

	if (FAILED(D3DXCreateTextureFromFileA(device, "../../Media/Textures/stones.jpg", &stones)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/sky.fx", &skyeffect)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/ambient.fx", &ambient)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/gradientcolor.fx", &gradientcolor)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/texturemodcolor.fx", &modulate)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/skinning.fx", &skinning)))
		return false;

	if (FAILED(DXCreateEffect(device,  "../../Media/ShadersDX/screenquad.fx", &screenquad)))
		return false;

	if (FAILED(DXRenderTextEx(device,
		"If you don't like metal, then it's time to...\n...REEVALUATE YOUR LIFE, BITCH!!!\n\n(tip: the dwarfs in the fire didn't like metal)",
		1024, 256, L"Arial", 1, Gdiplus::FontStyleBold, 25, &helptext)))
	{
		return false;
	}

	// load animated model
	for (int i = 0; i < 6; ++i) {
		dwarfs[i] = new DXAnimatedMesh();
		dwarfs[i]->SetSkinningMethod(SkinningMethodShader, skinning);
	}

	if (FAILED(dwarfs[0]->Load(device, "../../Media/MeshesDX/dwarf/dwarf.x"))) {
		MYERROR("Could not load 'dwarf.x'");
		return false;
	}

	dwarfs[0]->Clone(*dwarfs[1]);
	dwarfs[0]->Clone(*dwarfs[2]);
	dwarfs[0]->Clone(*dwarfs[3]);
	dwarfs[0]->Clone(*dwarfs[4]);
	dwarfs[0]->Clone(*dwarfs[5]);

	// skin variations
	DwarfSkin(0, 1, 1, 3, 1, 1, 3, 0);
	DwarfSkin(1, 2, 0, 1, 3, 3, 1, 1);
	DwarfSkin(2, 3, 2, 2, 2, 1, 2, 3);
	DwarfSkin(3, 1, 3, 2, 1, 3, 0, 2);
	DwarfSkin(4, 1, 1, 3, 3, 2, 2, 1);
	DwarfSkin(5, 0, 0, 1, 2, 3, 0, 1);

	// 0, 1, 2 - dead
	// 3, 5 - stand
	// 4 - jump
	// 6 - cheer with weapon
	// 7 - cheer with one hand
	// 8 - cheer with both hands
	dwarfs[0]->SetAnimation(6);
	dwarfs[1]->SetAnimation(8);
	dwarfs[2]->SetAnimation(7);
	dwarfs[3]->SetAnimation(4);
	dwarfs[4]->SetAnimation(2);
	dwarfs[5]->SetAnimation(1);

	// setup dwarf trafos
	D3DXQUATERNION rotation;
	D3DXVECTOR3 scale(0.1f, 0.1f, 0.1f);
	D3DXVECTOR3 trans(-1, 0, 1);

	D3DXQuaternionRotationYawPitchRoll(&rotation, -0.785f, 0, 0);
	D3DXMatrixTransformation(&dwarftransforms[0], NULL, NULL, &scale, NULL, &rotation, &trans);

	trans = D3DXVECTOR3(-1, 0, -1);

	D3DXQuaternionRotationYawPitchRoll(&rotation, -2.356f, 0, 0);
	D3DXMatrixTransformation(&dwarftransforms[1], NULL, NULL, &scale, NULL, &rotation, &trans);

	trans = D3DXVECTOR3(1, 0, -1);

	D3DXQuaternionRotationYawPitchRoll(&rotation, 2.356f, 0, 0);
	D3DXMatrixTransformation(&dwarftransforms[2], NULL, NULL, &scale, NULL, &rotation, &trans);

	trans = D3DXVECTOR3(1, 0, 1);

	D3DXQuaternionRotationYawPitchRoll(&rotation, 0.785f, 0, 0);
	D3DXMatrixTransformation(&dwarftransforms[3], NULL, NULL, &scale, NULL, &rotation, &trans);

	trans = D3DXVECTOR3(-0.2f, 0, -0.2f);

	D3DXQuaternionRotationYawPitchRoll(&rotation, -1.57f, 0, 0);
	D3DXMatrixTransformation(&dwarftransforms[4], NULL, NULL, &scale, NULL, &rotation, &trans);

	trans = D3DXVECTOR3(0.2f, 0, 0);

	D3DXQuaternionRotationYawPitchRoll(&rotation, -1.57f, 0, 0);
	D3DXMatrixTransformation(&dwarftransforms[5], NULL, NULL, &scale, NULL, &rotation, &trans);

	// setup camera
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetFov(Math::DegreesToRadians(45));
	camera.SetClipPlanes(0.1f, 20);
	camera.SetZoomLimits(3, 8);
	camera.SetDistance(5);
	camera.SetPosition(0, 0, 0);
	camera.SetOrientation(Math::DegreesToRadians(-135), Math::DegreesToRadians(45), 0);

	// setup particle system
	firesystem = new ParticleSystem();
	firestorage = new DXParticleStorage(device);

	firesystem->Initialize(firestorage, 500);
	firesystem->Force = Math::Vector3(0, 3e-2f, 0);

	// setup lightning
	lightning = new Lightning(CoilLightning);
	lightningstorage = new DXParticleStorage(device);

	// setup sound
	audiostreamer = new XA2AudioStreamer();

	burning = audiostreamer->LoadSound(L"../../Media/Sound/fire1.mp3");
	thunder = audiostreamer->LoadSound(L"../../Media/Sound/thunder.mp3");
	music = audiostreamer->LoadSound(L"../../Media/Sound/02_Lightning_Strike.mp3");

	music->SetVolume(1.0f);
	burning->SetVolume(1.5f);
	thunder->SetVolume(2.0f);

	burning->Loop();

	return true;
}

void UninitScene()
{
	delete music;
	delete burning;
	delete thunder;
	delete audiostreamer;
	delete firesystem;
	delete lightning;

	for (int i = 0; i < 6; ++i)
		delete dwarfs[i];

	SAFE_RELEASE(environment);
	SAFE_RELEASE(fire);
	SAFE_RELEASE(stones);
	SAFE_RELEASE(helptext);

	SAFE_RELEASE(skymesh);
	SAFE_RELEASE(box);

	SAFE_RELEASE(skyeffect);
	SAFE_RELEASE(ambient);
	SAFE_RELEASE(gradientcolor);
	SAFE_RELEASE(modulate);
	SAFE_RELEASE(skinning);
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
	firesystem->Update(dt);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;
	static float lightningstarttime = 0;
	static float lightningduration = 0;
	static bool started = false;

	// wait a few seconds
	if (!started && time > 0.5f) {
		music->Play();
		started = true;
	}

	D3DXMATRIX		world;
	Math::Matrix	identity(1, 1, 1, 1);
	Math::Matrix	view, proj;
	Math::Matrix	skyview, viewproj;
	bool			renderlightning = false;

	// NOTE: camera is right-handed
	camera.Animate(alpha);
	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);

	// do a lightning strike sometimes
	if (started) {
		renderlightning = (time >= lightningstarttime && time <= lightningstarttime + lightningduration);

		if (time > lightningstarttime + lightningduration) {
			// choose a new start time
			//lightningstarttime = time + (1 + Math::RandomFloat() * 3);
			//lightningduration = 0.3f + Math::RandomFloat() * 0.7f;

			lightningstarttime = time + (3 + Math::RandomFloat() * 2);
			lightningduration = 0.8f + Math::RandomFloat() * 0.8f;

			LightningStrike();
		}

		if (renderlightning && !thunder->IsPlaying())
			thunder->Play();
	}

	if (SUCCEEDED(device->BeginScene())) {
		D3DVIEWPORT9 oldviewport;
		D3DVIEWPORT9 viewport;

		device->GetViewport(&oldviewport);

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

		// render ground
		Math::MatrixMultiply(viewproj, view, proj);

		D3DXMatrixScaling(&world, 5, 0.1f, 5);
		world._42 = -0.05f;

		ambient->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		ambient->SetMatrix("matWorld", &world);

		ambient->Begin(NULL, 0);
		ambient->BeginPass(0);
		{
			device->SetTexture(0, stones);
			box->DrawSubset(0);
		}
		ambient->EndPass();
		ambient->End();

		// render dwarfs
		skinning->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);

		for (int i = 0; i < 6; ++i) {
			dwarfs[i]->Update(elapsedtime, &dwarftransforms[i]);
			dwarfs[i]->Draw();
		}

		// render particle system
		D3DXMatrixTranslation(&world, 0, 0.55f, 0);

		modulate->SetMatrix("matViewProj", (D3DXMATRIX*)&viewproj);
		modulate->SetMatrix("matWorld", (D3DXMATRIX*)&identity);

		firesystem->Draw((Math::Matrix&)world, view, [&](size_t numparticles) {
			modulate->Begin(NULL, 0);
			modulate->BeginPass(0);
			{
				device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
				device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

				device->SetStreamSource(0, firestorage->GetVertexBuffer(), 0, (UINT)firestorage->GetVertexStride());
				device->SetIndices(nullptr);
				device->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);

				device->SetTexture(0, fire);
				device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, (UINT)numparticles * 2);

				device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
			}
			modulate->EndPass();
			modulate->End();
		});

		if (renderlightning) {
			// render lightning
			D3DXVECTOR4 color(1, 0.5f, 1, 1);

			lightning->Subdivide(time);
			lightning->Generate(LIGHTNING_THICKNESS, view);

			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

			device->SetStreamSource(0, lightningstorage->GetVertexBuffer(), 0, (UINT)lightningstorage->GetVertexStride());
			device->SetIndices(nullptr);
			device->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1);

			gradientcolor->SetMatrix("matViewProj", (D3DXMATRIX*)&proj);
			gradientcolor->SetMatrix("matWorld", (D3DXMATRIX*)&identity);
			gradientcolor->SetVector("matColor", &color);

			gradientcolor->Begin(NULL, 0);
			gradientcolor->BeginPass(0);
			{
				device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, (UINT)lightningstorage->GetNumVertices() / 3);
			}
			gradientcolor->EndPass();
			gradientcolor->End();

			device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		}

		if (drawtext) {
			// render text
			viewport = oldviewport;

			viewport.Width = 1024;
			viewport.Height = 256;
			viewport.X = viewport.Y = 10;

			device->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
			device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
			device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

			device->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);

			device->SetFVF(D3DFVF_XYZW|D3DFVF_TEX1);
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
