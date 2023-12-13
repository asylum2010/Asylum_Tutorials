
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"
#include "..\Common\spectatorcamera.h"
#include "..\Common\geometryutils.h"
#include "..\Common\ldgtaorenderer.h"
#include "..\Common\averageluminance.h"

// TODO:
// - transparency
// - pan/zoom in debug mode
// - keybind to uniform/LSPSM

#define METERS_PER_UNIT		0.1f
#define SHADOWMAP_SIZE		2048

// helper macros
#define TITLE				"Shader sample 43: Light Space Perspective Shadow Mapping"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*		app				= nullptr;

OpenGLEffect*		gbuffereffect	= nullptr;
OpenGLEffect*		ambientlight	= nullptr;
OpenGLEffect*		sunlight		= nullptr;
OpenGLEffect*		skyeffect		= nullptr;
OpenGLEffect*		shadow			= nullptr;
OpenGLEffect*		simplecolor		= nullptr;
OpenGLEffect*		tonemap			= nullptr;

OpenGLMesh*			model			= nullptr;
OpenGLMesh*			skymesh			= nullptr;
OpenGLMesh*			unitcube		= nullptr;
OpenGLMesh*			debugbox		= nullptr;
OpenGLMesh*			debugline		= nullptr;

OpenGLFramebuffer*	gbuffer			= nullptr;
OpenGLFramebuffer*	hdrframebuffer	= nullptr;
OpenGLFramebuffer*	shadowmap		= nullptr;

OpenGLScreenQuad*	screenquad		= nullptr;
OpenGLScreenQuad*	scaleshadowmap	= nullptr;

LDGTAORenderer*		gtaorenderer	= nullptr;
AverageLuminance*	avgluminance	= nullptr;

SpectatorCamera		camera;
BasicCamera			light;
BasicCamera			debugcamera;
GLuint				asphalt			= 0;
GLuint				environment		= 0;
GLuint				diffirradiance	= 0;
GLuint				helptext		= 0;
bool				usedebugcamera	= false;
bool				drawgtao		= true;
bool				drawtext		= true;

extern void DebugRender(
	const Math::Matrix& viewproj, const Math::Matrix& lightviewproj, const Math::Vector3& eyepos,
	const Math::AABox& scenebox, const Math::AABox& body, const std::vector<Math::Vector3>& points);

static void UpdateText()
{
	if (usedebugcamera) {
		GLRenderText(
			"Mouse middle - Orbit camera\n\n"
			, //"Red: scene bounding box\nOrange: shadow frustum\nYellow: intersection points\n",
			helptext, 512, 512);
	} else {
		GLRenderText(
			"Use WASD and mouse to move around\n\nMouse right - Orbit light\n\n"
			"1 - Toggle GTAO\nR - Toggle debug camera\nH - Toggle help text",
			helptext, 512, 512);
	}
}

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	gtaorenderer = new LDGTAORenderer(screenwidth, screenheight);
	avgluminance = new AverageLuminance();

	// load model
	if (!GLCreateMeshFromQM("../../Media/MeshesQM/modern_house/modern_house.qm", &model))
		return false;

	// create unit cube for ground
	OpenGLVertexElement elem[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0, 12, GLDECLTYPE_FLOAT3, GLDECLUSAGE_NORMAL, 0 },
		{ 0, 24, GLDECLTYPE_FLOAT2, GLDECLUSAGE_TEXCOORD, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};
	
	GLCreateMesh(24, 36, GLMESH_32BIT, elem, &unitcube);

	GeometryUtils::CommonVertex* vdata = nullptr;
	uint32_t* idata = nullptr;

	unitcube->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&vdata);
	unitcube->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
	{
		GeometryUtils::CreateBox(vdata, idata, 1, 1, 1);
	}
	unitcube->UnlockIndexBuffer();
	unitcube->UnlockVertexBuffer();

	unitcube->CalculateBoundingBox();

	if (!GLCreateTextureFromFile("../../Media/Textures/asphalt2.jpg", true, &asphalt))
		return false;

	// create debug box
	OpenGLVertexElement boxelem[] = {
		{ 0, 0, GLDECLTYPE_FLOAT3, GLDECLUSAGE_POSITION, 0 },
		{ 0xff, 0, 0, 0, 0 }
	};

	GLCreateMesh(8, 24, GLMESH_32BIT, boxelem, &debugbox);

	Math::Vector3* boxvdata = nullptr;

	debugbox->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&boxvdata);
	debugbox->LockIndexBuffer(0, 0, GLLOCK_DISCARD, (void**)&idata);
	{
		boxvdata[0] = Math::Vector3(-1, -1, -1);
		boxvdata[1] = Math::Vector3(-1, -1, 1);
		boxvdata[2] = Math::Vector3(-1, 1, -1);
		boxvdata[3] = Math::Vector3(-1, 1, 1);

		boxvdata[4] = Math::Vector3(1, -1, -1);
		boxvdata[5] = Math::Vector3(1, -1, 1);
		boxvdata[6] = Math::Vector3(1, 1, -1);
		boxvdata[7] = Math::Vector3(1, 1, 1);

		uint32_t wireindices[] = {
			0, 1, 1, 3, 3, 2,
			2, 0, 0, 4, 4, 6,
			6, 2, 4, 5, 5, 7,
			7, 6, 7, 3, 1, 5
		};

		memcpy(idata, wireindices, 24 * sizeof(uint32_t));
	}
	debugbox->UnlockIndexBuffer();
	debugbox->UnlockVertexBuffer();

	debugbox->GetAttributeTable()[0].PrimitiveType = GL_LINES;

	// create debug line
	GLCreateMesh(2, 0, GLMESH_32BIT, boxelem, &debugline);

	Math::Vector3* linevdata = nullptr;

	debugline->LockVertexBuffer(0, 0, GLLOCK_DISCARD, (void**)&linevdata);
	{
		linevdata[0] = Math::Vector3(0, 0, 0);
		linevdata[1] = Math::Vector3(0, 0, 1);
	}
	debugline->UnlockVertexBuffer();

	debugline->GetAttributeTable()[0].PrimitiveType = GL_LINES;

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/gbufferonly.vert", 0, 0, 0, "../../Media/ShadersGL/gbufferonly.frag", &gbuffereffect)) {
		MYERROR("Could not load 'gbufferonly' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/ambientprobe.vert", 0, 0, 0, "../../Media/ShadersGL/ambientprobe.frag", &ambientlight)) {
		MYERROR("Could not load 'ambientlight' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/sunlight.vert", 0, 0, 0, "../../Media/ShadersGL/sunlight.frag", &sunlight)) {
		MYERROR("Could not load 'sunlight' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/sky.vert", 0, 0, 0, "../../Media/ShadersGL/sky.frag", &skyeffect)) {
		MYERROR("Could not load 'sky' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/lspsm.vert", 0, 0, 0, "../../Media/ShadersGL/lspsm.frag", &shadow)) {
		MYERROR("Could not load 'shadow' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/simplecolor.vert", 0, 0, 0, "../../Media/ShadersGL/simplecolor.frag", &simplecolor)) {
		MYERROR("Could not load 'simplecolor' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/tonemap.frag", &tonemap)) {
		MYERROR("Could not load 'tonemap' shader");
		return false;
	}

	Math::Matrix identity(1, 1, 1, 1);

	ambientlight->SetInt("albedo", 0);
	ambientlight->SetInt("aoTarget", 1);
	ambientlight->SetInt("illumDiffuse", 2);

	sunlight->SetInt("albedo", 0);
	sunlight->SetInt("shadowMap", 1);

	skyeffect->SetInt("sampler0", 0);

	tonemap->SetMatrix("matTexture", identity);
	tonemap->SetInt("renderedScene", 0);
	tonemap->SetInt("averageLuminance", 1);

	// load sky related things
	GLCreateCubeTextureFromDDS("../../Media/Textures/partly_cloudy.dds", false, &environment);
	GLCreateCubeTextureFromDDS("../../Media/Textures/partly_cloudy_diff_irrad.dds", false, &diffirradiance);

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/sky.qm", &skymesh)) {
		MYERROR("Could not load 'sky' mesh");
		return false;
	}

	// create frame buffers
	gbuffer = new OpenGLFramebuffer(screenwidth, screenheight);

	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_NEAREST);		// normals
	gbuffer->Attach(GL_COLOR_ATTACHMENT1, gtaorenderer->GetCurrentDepthBuffer(), 0);	// depth (linearized)
	gbuffer->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D32F);

	GL_ASSERT(gbuffer->Validate());

	hdrframebuffer = new OpenGLFramebuffer(screenwidth, screenheight);

	hdrframebuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_NEAREST);
	hdrframebuffer->Attach(GL_DEPTH_ATTACHMENT, gbuffer->GetDepthAttachment());

	GL_ASSERT(hdrframebuffer->Validate());

	// create shadow map
	shadowmap = new OpenGLFramebuffer(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	shadowmap->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_R32F, GL_NEAREST);

	float border[] = { 1, 1, 1, 1 };

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

	shadowmap->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D24S8);

	if (!shadowmap->Validate())
		return false;

	// other
	screenquad = new OpenGLScreenQuad();
	scaleshadowmap = new OpenGLScreenQuad("../../Media/ShadersGL/scaleto01.frag");

	// setup camera
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetClipPlanes(0.1f, 50.0f);
	camera.SetEyePosition(-9.6f, 3.3f, 13.0f);
	camera.SetOrientation(0.8f, 0.157f, 0);
	camera.SetSpeed(5);

	// setup light camera
	light.SetOrientation(2, 0.66f, 0);
	light.SetPitchLimits(Math::DegreesToRadians(2), Math::DegreesToRadians(88));

#if 0	// to see if calculations match article figures
	camera.SetEyePosition(0.2f, 3.35f, 15.4f);
	camera.SetOrientation(0, 0, 0);

	light.SetOrientation(1.58112097f, 1.53588903f, 0);
#elif 0	// test setup for incorrect shadow frustum (solved by disabling near/far clipping)
	camera.SetEyePosition(10.3869305f, 5.58259392f, 14.1368666f);
	camera.SetOrientation(-0.482815713f, 0.588968873f, 0);

	light.SetOrientation(-2.89564919f, 1.33953965f, 0);
#elif 0	// test setup for unexpected shadow (must always check for containment)
	camera.SetEyePosition(10.7709608f, 10.4315701f, 15.2841330f);
	camera.SetOrientation(-1.42529273f, 0.732958078f, 0);

	light.SetOrientation(-1.73064148f, 0.946840048f, 0);
#endif

	// setup debug camera
	debugcamera.SetFov(Math::DegreesToRadians(60));
	debugcamera.SetAspect((float)screenwidth / (float)screenheight);
	debugcamera.SetClipPlanes(0.1f, 200.0f);
	debugcamera.SetPosition(0, 0, 0);
	debugcamera.SetDistance(45);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	UpdateText();

	return true;
}

void UninitScene()
{
	delete avgluminance;
	delete gtaorenderer;

	delete shadowmap;
	delete hdrframebuffer;
	delete gbuffer;

	delete tonemap;
	delete simplecolor;
	delete shadow;
	delete skyeffect;
	delete sunlight;
	delete ambientlight;
	delete gbuffereffect;
	delete scaleshadowmap;

	delete screenquad;
	delete debugline;
	delete debugbox;
	delete skymesh;
	delete model;
	delete unitcube;

	GL_SAFE_DELETE_TEXTURE(diffirradiance);
	GL_SAFE_DELETE_TEXTURE(environment);
	GL_SAFE_DELETE_TEXTURE(asphalt);
	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

void KeyDown(KeyCode key)
{
	camera.OnKeyDown(key);
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
		drawgtao = !drawgtao;
		break;

	case KeyCodeH:
		drawtext = !drawtext;
		break;

	case KeyCodeR:
		usedebugcamera = !usedebugcamera;
		UpdateText();
		break;

	default:
		camera.OnKeyUp(key);
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonMiddle) {
		if (usedebugcamera) {
			debugcamera.OrbitRight(Math::DegreesToRadians(dx));
			debugcamera.OrbitUp(Math::DegreesToRadians(dy));
		}
	}

	if (state & MouseButtonRight) {
		light.OrbitRight(Math::DegreesToRadians(dx));
		light.OrbitUp(Math::DegreesToRadians(dy));
	}

	// uses left button
	camera.OnMouseMove(dx, dy);
}

void MouseDown(MouseButton button, int32_t x, int32_t y)
{
	camera.OnMouseDown(button);
}

void MouseUp(MouseButton button, int32_t x, int32_t y)
{
	camera.OnMouseUp(button);
}

void Update(float delta)
{
	camera.Update(delta);
	light.Update(delta);

	if (usedebugcamera)
		debugcamera.Update(delta);
}

void RenderSky(const Math::Matrix& view, const Math::Matrix& proj, const Math::Vector3& eye)
{
	Math::Matrix world, tmp(proj);

	// project to depth ~ 1.0
	tmp._33 = -1.0f + 1e-4f;
	tmp._43 = 0;

	Math::MatrixTranslation(world, eye.x, eye.y, eye.z);
	Math::MatrixMultiply(tmp, view, tmp);

	skyeffect->SetVector("eyePos", eye);
	skyeffect->SetMatrix("matWorld", world);
	skyeffect->SetMatrix("matViewProj", tmp);

	skyeffect->Begin();
	{
		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP, environment);
		skymesh->DrawSubset(0);
	}
	skyeffect->End();
}

void RenderScene(OpenGLEffect* effect, const Math::Matrix& world, const Math::Matrix& groundworld)
{
	Math::Matrix worldinv;
	Math::Vector4 pbrparams;
	Math::Vector4 uvscale(1, 1, 1, 1);

	Math::MatrixInverse(worldinv, world);

	effect->SetVector("uvScale", &uvscale.x);
	effect->SetMatrix("matWorld", world);
	effect->SetMatrix("matWorldInv", worldinv);

	effect->Begin();
	{
		for (GLuint i = 0; i < model->GetNumSubsets(); ++i) {
			model->DrawSubset(i, [&](const OpenGLMaterial& mat) -> void {
				pbrparams.x = 1.0f - (mat.Power / 1000.0f);
				pbrparams.y = 0;
				pbrparams.z = ((mat.Texture == 0) ? 0.0f : 1.0f);

				effect->SetVector("baseColor", &mat.Diffuse.r);
				effect->SetVector("matParams", &pbrparams.x);
				effect->CommitChanges();
				});
		}

		uvscale.x = 5;
		uvscale.y = 10;

		Math::MatrixInverse(worldinv, groundworld);

		effect->SetMatrix("matWorld", groundworld);
		effect->SetMatrix("matWorldInv", worldinv);
		effect->SetVector("uvScale", &uvscale.x);
		effect->CommitChanges();

		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, asphalt);

		unitcube->Draw();
	}
	effect->End();
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix	world, view, viewinv, proj;
	Math::Matrix	groundworld(1, 1, 1, 1);
	Math::Matrix	viewproj;
	Math::Matrix	lightview, lightproj, lightviewproj;
	Math::AABox		modelbox = model->GetBoundingBox();
	Math::AABox		groundbox(Math::Vector3(1, 1, 1));
	Math::AABox		scenebox;
	Math::Vector4	eyepos(0, 0, 0, 1);
	Math::Vector4	clipinfo;
	Math::Vector4	texelsize(1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE, 0, 0);
	Math::Vector3	groundsize(20, 0.01f, 40);
	Math::Vector3	viewdir, lightdir;

	uint32_t		screenwidth = app->GetClientWidth();
	uint32_t		screenheight = app->GetClientHeight();

	// calculate scene envelope
	Math::MatrixScaling(world, METERS_PER_UNIT, METERS_PER_UNIT, METERS_PER_UNIT);

	modelbox.TransformAxisAligned(world);

	groundworld._11 = groundsize.x;
	groundworld._22 = groundsize.y;
	groundworld._33 = groundsize.z;
	groundworld._42 = modelbox.Min.y;

	groundbox.TransformAxisAligned(groundworld);
	scenebox = modelbox + groundbox;

	// animate
	camera.Animate(alpha);
	camera.FitToBox(scenebox);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition((Math::Vector3&)eyepos);

	light.Animate(alpha);
	light.GetViewMatrix(lightview);

	Math::MatrixInverse(viewinv, view);

	clipinfo.x = camera.GetNearPlane();
	clipinfo.y = camera.GetFarPlane();
	clipinfo.z = 0.5f * (screenheight / (2.0f * tanf(camera.GetFov() * 0.5f)));

	// handedness is the same but we want the light direction reversed
	viewdir = Math::Vector3(-view._13, -view._23, -view._33);
	lightdir = Math::Vector3(-lightview._13, -lightview._23, -lightview._33);
	
	Math::MatrixMultiply(viewproj, view, proj);

	// focus the shadow map
	std::vector<Math::Vector3> isectpoints;
	
	GeometryUtils::FrustumIntersectAABox(isectpoints, viewproj, scenebox);
	GeometryUtils::LightVolumeIntersectAABox(isectpoints, -lightdir, scenebox);

	// calculate light space perspective frustum
	Math::AABox lsbody;
	Math::Matrix lslightview(1, 1, 1, 1);
	Math::Matrix lispsmproj(1, 1, 1, 1);
	Math::Matrix lslightviewproj;
	Math::Vector3 lsleft, lsup;

	Math::Vec3Cross(lsleft, lightdir, viewdir);
	Math::Vec3Normalize(lsleft, lsleft);

	Math::Vec3Cross(lsup, lsleft, lightdir);
	Math::Vec3Normalize(lsup, lsup);

	lslightview._11 = lsleft.x;	lslightview._12 = lsup.x;	lslightview._13 = lightdir.x;
	lslightview._21 = lsleft.y;	lslightview._22 = lsup.y;	lslightview._23 = lightdir.y;
	lslightview._31 = lsleft.z;	lslightview._32 = lsup.z;	lslightview._33 = lightdir.z;

	lslightview._41 = -Math::Vec3Dot(lsleft, (Math::Vector3&)eyepos);
	lslightview._42 = -Math::Vec3Dot(lsup, (Math::Vector3&)eyepos);
	lslightview._43 = -Math::Vec3Dot(lightdir, (Math::Vector3&)eyepos);

	//GeometryUtils::CalculateAABoxFromPoints(lsbody, isectpoints, lslightview);
	float viewnear = 1.0f; //-lsbody.Max.z;

	// transform body B to light space, calculate bounding box
	GeometryUtils::CalculateAABoxFromPoints(lsbody, isectpoints, lslightview);

	// calculate LSPSM matrix
	float cosgamma	= Math::Vec3Dot(viewdir, lightdir);
	float singamma	= sqrtf(1.0f - cosgamma * cosgamma);
	float znear		= viewnear / singamma;
	float d			= fabs(lsbody.Max.y - lsbody.Min.y);
	float zfar		= znear + d * singamma;
	float n			= (znear + sqrtf(zfar * znear)) / singamma;
	float f			= n + d;

	lispsmproj._22 = (f + n) / (f - n);
	lispsmproj._42 = -2 * f * n / (f - n);
	lispsmproj._24 = 1;
	lispsmproj._44 = 0;
	
	// project everything into unit cube
	Math::Matrix fittounitcube;
	Math::Vector3 corrected;

	corrected = (Math::Vector3&)eyepos - lsup * (n - viewnear);

	lslightview._41 = -Math::Vec3Dot(lsleft, corrected);
	lslightview._42 = -Math::Vec3Dot(lsup, corrected);
	lslightview._43 = -Math::Vec3Dot(lightdir, corrected);

	Math::MatrixMultiply(lslightviewproj, lslightview, lispsmproj);
	GeometryUtils::CalculateAABoxFromPoints(lsbody, isectpoints, lslightviewproj);

	MatrixOrthoOffCenterRH(
		fittounitcube,
		lsbody.Min.x, lsbody.Max.x,
		lsbody.Min.y, lsbody.Max.y,
		-lsbody.Min.z, -lsbody.Max.z);

	Math::MatrixMultiply(lslightviewproj, lslightviewproj, fittounitcube);

#if 0
	// uniform shadow mapping
	Math::Vector2 clipplanes;

	//Math::AABox vsbody;
	//GeometryUtils::CalculateAABoxFromPoints(vsbody, isectpoints, Math::Matrix(1, 1, 1, 1));
	//Math::FitToBoxOrtho(lightproj, clipplanes, lightview, vsbody);

	Math::FitToBoxOrtho(lightproj, clipplanes, lightview, scenebox);
	Math::MatrixMultiply(lightviewproj, lightview, lightproj);

	/*
	Math::AABox vsbody;

	GeometryUtils::CalculateAABoxFromPoints(vsbody, isectpoints, lightview);

	MatrixOrthoOffCenterRH(
		lightproj,
		vsbody.Min.x, vsbody.Max.x,
		vsbody.Min.y, vsbody.Max.y,
		vsbody.Min.z, vsbody.Max.z);

	Math::MatrixMultiply(lightviewproj, lightview, lightproj);
	*/
#else
	lightviewproj = lslightviewproj;
#endif

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_CLAMP);	// cheat

	// render shadow map
	shadow->SetMatrix("matWorld", world);
	shadow->SetMatrix("matViewProj", lightviewproj);

	shadowmap->Set();
	{
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		shadow->Begin();
		{
			for (GLuint i = 0; i < model->GetNumSubsets(); ++i)
				model->DrawSubset(i, false);

			shadow->SetMatrix("matWorld", groundworld);
			shadow->CommitChanges();

			unitcube->Draw();
		}
		shadow->End();
	}
	shadowmap->Unset();

	glDisable(GL_DEPTH_CLAMP);

	// render scene or debug geometry
	if (usedebugcamera) {
		debugcamera.Animate(alpha);

		glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
		glViewport(0, 0, screenwidth, screenheight);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		DebugRender(viewproj, lightviewproj, corrected, scenebox, lsbody, isectpoints);
	} else {
		Math::Color black = { 0, 0, 0, 1 };
		Math::Color white = { 1, 1, 1, 1 };

		// render g-buffer
		gbuffer->Set();
		{
			glClearBufferfv(GL_COLOR, 0, black);
			glClearBufferfv(GL_COLOR, 1, white);
			glClearBufferfv(GL_DEPTH, 0, white);

			glDepthFunc(GL_LESS);

			gbuffereffect->SetMatrix("matView", view);
			gbuffereffect->SetMatrix("matViewInv", viewinv);
			gbuffereffect->SetMatrix("matViewProj", viewproj);
			gbuffereffect->SetVector("clipPlanes", clipinfo);

			RenderScene(gbuffereffect, world, groundworld);
		}
		gbuffer->Unset();

		// render GTAO
		if (drawgtao)
			gtaorenderer->Render(gbuffer->GetColorAttachment(0), view, proj, clipinfo);
		else
			gtaorenderer->ClearOnly();

		// render lighting
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_EQUAL);
		glDepthMask(GL_FALSE);

		hdrframebuffer->Set();
		{
			glClear(GL_COLOR_BUFFER_BIT);

			// render ambient light
			ambientlight->SetMatrix("matViewProj", viewproj);

			GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, gtaorenderer->GetGTAO());
			GLSetTexture(GL_TEXTURE2, GL_TEXTURE_CUBE_MAP, diffirradiance);

			RenderScene(ambientlight, world, groundworld);

			// render sunlight
			glEnable(GL_BLEND);
			glBlendEquation(GL_FUNC_ADD);
			glBlendFunc(GL_ONE, GL_ONE);

			lightdir = -lightdir;

			sunlight->SetMatrix("matViewProj", viewproj);
			sunlight->SetMatrix("lightViewProj", lightviewproj);

			sunlight->SetVector("eyePos", &eyepos.x);
			sunlight->SetVector("sunDir", &lightdir.x);
			sunlight->SetVector("texelSize", &texelsize.x);

			GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, shadowmap->GetColorAttachment(0));

			RenderScene(sunlight, world, groundworld);

			glDisable(GL_BLEND);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);

			// render sky
			RenderSky(view, proj, (const Math::Vector3&)eyepos);
		}
		hdrframebuffer->Unset();

		// measure & adapt luminance
		avgluminance->Measure(hdrframebuffer->GetColorAttachment(0), Math::Vector2(1.0f / screenwidth, 1.0f / screenheight), elapsedtime);
		glViewport(0, 0, screenwidth, screenheight);

		// tone mapping
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glEnable(GL_FRAMEBUFFER_SRGB);

		tonemap->Begin();
		{
			GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, hdrframebuffer->GetColorAttachment(0));
			GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, avgluminance->GetAverageLuminance());

			screenquad->Draw(false);
		}
		tonemap->End();

		glDisable(GL_FRAMEBUFFER_SRGB);
	}

	if (drawtext) {
		// render text
		glViewport(10, screenheight - 522, 512, 512);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
		Math::MatrixReflect(world, xzplane);

		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, helptext);

		screenquad->SetTextureMatrix(world);
		screenquad->Draw();

		// debug render shadow map
		glViewport(0, 0, 256, 256);
		glDisable(GL_BLEND);

		Math::MatrixIdentity(world);

		GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, shadowmap->GetColorAttachment(0));

		scaleshadowmap->SetTextureMatrix(world);
		scaleshadowmap->Draw();
	}

	// reset states
	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, screenwidth, screenheight);

	// check errors
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
		std::cout << "Error\n";

	app->Present();
}

int main(int argc, char* argv[])
{
	app = Application::Create(1360, 768);
	app->SetTitle(TITLE);

	if (!app->InitializeDriverInterface(GraphicsAPIOpenGL)) {
		delete app;
		return 1;
	}

	app->InitSceneCallback = InitScene;
	app->UninitSceneCallback = UninitScene;
	app->UpdateCallback = Update;
	app->RenderCallback = Render;
	app->KeyDownCallback = KeyDown;
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;
	app->MouseDownCallback = MouseDown;
	app->MouseUpCallback = MouseUp;

	app->Run();
	delete app;

	return 0;
}
