
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gtaorenderer.h"
#include "..\Common\ldgtaorenderer.h"
#include "..\Common\spectatorcamera.h"

#define METERS_PER_UNIT		0.01f	// for Sponza

// helper macros
#define TITLE				"Shader sample 54: Ground Truth-based Ambient Occlusion"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*		app					= nullptr;

OpenGLFramebuffer*	gbuffer				= nullptr;
OpenGLEffect*		gbuffereffect		= nullptr;
OpenGLEffect*		combineeffect		= nullptr;

OpenGLMesh*			model				= nullptr;
OpenGLScreenQuad*	screenquad			= nullptr;

GLuint				supplytex			= 0;			// for missing textures
GLuint				supplynormalmap		= 0;			// for missing normal maps
GLuint				helptext			= 0;

SpectatorCamera		camera;
GTAORenderer*		gtaorenderer		= nullptr;
LDGTAORenderer*		ldgtaorenderer		= nullptr;

int					rendermode			= 3;
bool				drawtext			= true;
bool				useldrenderer		= false;

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// load model
	if (!GLCreateMeshFromQM("../../Media/MeshesQM/sponza/sponza.qm", &model))
		return false;

	std::cout << "Generating tangent frame...\n";

	model->GenerateTangentFrame();
	model->EnableSubset(4, false);
	model->EnableSubset(259, false);

	// create substitute textures
	GLuint normal = 0xffff7f7f;	// RGBA (0.5f, 0.5f, 1.0f, 1.0f)
	OpenGLMaterial* materials = model->GetMaterialTable();

	GLCreateTextureFromFile("../../Media/Textures/vk_logo.jpg", true, &supplytex);
	GLCreateTexture(1, 1, 1, GLFMT_A8B8G8R8, &supplynormalmap, &normal);

	for (GLuint i = 0; i < model->GetNumSubsets(); ++i) {
		if (materials[i].Texture == 0)
			materials[i].Texture = supplytex;

		if (materials[i].NormalMap == 0)
			materials[i].NormalMap = supplynormalmap;
	}

	// create renderer
	gtaorenderer = new GTAORenderer(screenwidth, screenheight);
	ldgtaorenderer = new LDGTAORenderer(screenwidth, screenheight);

	// create render targets
	GLint value = 0;

	gbuffer = new OpenGLFramebuffer(screenwidth, screenheight);

	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A8B8G8R8_sRGB, GL_NEAREST);	// color
	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT1, GLFMT_A16B16G16R16F, GL_NEAREST);	// normals

	gbuffer->Attach(GL_COLOR_ATTACHMENT2, gtaorenderer->GetCurrentDepthBuffer(), 0);
	gbuffer->AttachRenderbuffer(GL_DEPTH_STENCIL_ATTACHMENT, GLFMT_D24S8);

	GL_ASSERT(gbuffer->Validate());

	glBindFramebuffer(GL_FRAMEBUFFER, gbuffer->GetFramebuffer());
	{
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &value);
		GL_ASSERT(value == GL_SRGB);

		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &value);
		GL_ASSERT(value == GL_LINEAR);

		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &value);
		GL_ASSERT(value == GL_LINEAR);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/gbuffer.vert", 0, 0, 0, "../../Media/ShadersGL/gbuffer.frag", &gbuffereffect)) {
		MYERROR("Could not load 'gbuffer' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/gtaocombine.frag", &combineeffect)) {
		MYERROR("Could not load 'combine' shader");
		return false;
	}

	Math::Matrix identity;
	Math::MatrixIdentity(identity);

	combineeffect->SetMatrix("matTexture", identity);

	gbuffereffect->SetInt("sampler0", 0);
	gbuffereffect->SetInt("sampler1", 1);

	combineeffect->SetInt("sampler0", 0);
	combineeffect->SetInt("sampler1", 1);

	screenquad = new OpenGLScreenQuad();

	// setup camera
	camera.SetFov(Math::DegreesToRadians(60));
	camera.SetAspect((float)screenwidth / (float)screenheight);
	camera.SetClipPlanes(0.1f, 50.0f);
	camera.SetEyePosition(-0.12f, 3.0f, 0);
	camera.SetOrientation(-Math::HALF_PI, 0, 0);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"Use WASD and mouse to move around\n\n1 - Scene only\n2 - Scene with GTAO (multi-bounce)\n3 - GTAO only\n4- Toggle low-disprecancy sequence\n\nB - Toggle temporal denoiser\nH - Toggle help text",
		helptext, 512, 512);

	return true;
}

void UninitScene()
{
	gbuffer->Detach(GL_COLOR_ATTACHMENT2);

	delete ldgtaorenderer;
	delete gtaorenderer;
	delete gbuffereffect;
	delete combineeffect;
	delete gbuffer;
	delete model;
	delete screenquad;

	GL_SAFE_DELETE_TEXTURE(supplytex);
	GL_SAFE_DELETE_TEXTURE(supplynormalmap);
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
	case KeyCodeB:
		gtaorenderer->ToggleTemporalDenoiser();
		ldgtaorenderer->ToggleTemporalDenoiser();
		break;

	case KeyCodeH:
		drawtext = !drawtext;
		break;

	case KeyCode1:
		rendermode = 1;
		break;

	case KeyCode2:
		rendermode = 2;
		break;

	case KeyCode3:
		rendermode = 3;
		break;

	case KeyCode4:
		useldrenderer = !useldrenderer;
		break;

	default:
		camera.OnKeyUp(key);
		break;
	}
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
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
}

void RenderModel(const Math::Matrix& view, const Math::Matrix& proj, const Math::Vector3& eye, const Math::Vector4& clipinfo)
{
	Math::Color clearcolor	= { 0, 0.0103f, 0.0707f, 1 };
	Math::Color black		= { 0, 0, 0, 1 };
	Math::Color white		= { 1, 1, 1, 1 };

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_FRAMEBUFFER_SRGB);

	// render g-buffer
	gbuffer->Detach(GL_COLOR_ATTACHMENT2);

	if (useldrenderer)
		gbuffer->Attach(GL_COLOR_ATTACHMENT2, ldgtaorenderer->GetCurrentDepthBuffer(), 0);
	else
		gbuffer->Attach(GL_COLOR_ATTACHMENT2, gtaorenderer->GetCurrentDepthBuffer(), 0);

	gbuffer->Set();
	{
		glClearBufferfv(GL_COLOR, 0, clearcolor);

		// rgba16f reads seem to be incoherent on AMD; clearing the buffer apparently flushes the cache
		glClearBufferfv(GL_COLOR, 1, black);

		glClearBufferfv(GL_COLOR, 2, white);
		glClearBufferfv(GL_DEPTH, 0, white);

		gbuffereffect->Begin();
		{
			for (GLuint i = 0; i < model->GetNumSubsets(); ++i)
				model->DrawSubset(i, true);
		}
		gbuffereffect->End();
	}
	gbuffer->Unset();

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_FRAMEBUFFER_SRGB);

	if (rendermode > 1) {
		if (useldrenderer)
			ldgtaorenderer->Render(gbuffer->GetColorAttachment(1), view, proj, clipinfo);
		else
			gtaorenderer->Render(gbuffer->GetColorAttachment(1), view, proj, clipinfo);
	}

	// present to screen
	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glEnable(GL_FRAMEBUFFER_SRGB);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffer->GetColorAttachment(0));

	glActiveTexture(GL_TEXTURE1);

	if (useldrenderer)
		glBindTexture(GL_TEXTURE_2D, ldgtaorenderer->GetGTAO());
	else
		glBindTexture(GL_TEXTURE_2D, gtaorenderer->GetGTAO());
	
	combineeffect->SetInt("renderMode", rendermode);
	combineeffect->Begin();
	{
		screenquad->Draw(false);
	}
	combineeffect->End();
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix world, view, proj;
	Math::Matrix viewproj, viewprojinv;
	Math::Matrix worldinv, viewinv;
	Math::Matrix currWVP;

	Math::Vector4 clipinfo;
	Math::Vector4 params(0, 0, 1, 0);
	Math::Vector3 eye;

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	clipinfo.x = camera.GetNearPlane();
	clipinfo.y = camera.GetFarPlane();
	clipinfo.z = 0.5f * (app->GetClientHeight() / (2.0f * tanf(camera.GetFov() * 0.5f)));

	// setup model
	Math::MatrixScaling(world, METERS_PER_UNIT, METERS_PER_UNIT, METERS_PER_UNIT);

	world._41 = 60.518921f * METERS_PER_UNIT;
	world._42 = (778.0f - 651.495361f) * METERS_PER_UNIT;
	world._43 = -38.690552f * METERS_PER_UNIT;

	Math::MatrixInverse(worldinv, world);
	Math::MatrixInverse(viewinv, view);

	Math::MatrixMultiply(currWVP, world, view);
	Math::MatrixMultiply(currWVP, currWVP, proj);

	gbuffereffect->SetMatrix("matWorld", world);
	gbuffereffect->SetMatrix("matWorldInv", worldinv);
	gbuffereffect->SetMatrix("matView", view);
	gbuffereffect->SetMatrix("matViewInv", viewinv);
	gbuffereffect->SetMatrix("matProj", proj);
	gbuffereffect->SetVector("clipPlanes", clipinfo);
	gbuffereffect->SetVector("matParams", params);

	RenderModel(view, proj, eye, clipinfo);

	if (drawtext) {
		uint32_t screenwidth = app->GetClientWidth();
		uint32_t screenheight = app->GetClientHeight();

		// render text
		glViewport(10, screenheight - 522, 512, 512);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
		Math::MatrixReflect(world, xzplane);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, helptext);

		screenquad->SetTextureMatrix(world);
		screenquad->Draw();

		// reset states
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glViewport(0, 0, screenwidth, screenheight);
	}

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
