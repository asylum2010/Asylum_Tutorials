
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define SHADOWMAP_SIZE		512

// helper macros
#define TITLE				"Shader sample 43: Variance shadow mapping"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample variables
Application*		app				= nullptr;

OpenGLMesh*			angel			= nullptr;
OpenGLMesh*			table			= nullptr;
OpenGLMesh*			skymesh			= nullptr;

OpenGLEffect*		skyeffect		= nullptr;
OpenGLEffect*		bloom			= nullptr;
OpenGLEffect*		blinnphong		= nullptr;
OpenGLEffect*		varianceshadow	= nullptr;
OpenGLEffect*		boxblur3x3		= nullptr;

OpenGLFramebuffer*	scenetarget		= nullptr;
OpenGLFramebuffer*	bloomtarget		= nullptr;
OpenGLFramebuffer*	shadowmap		= nullptr;
OpenGLFramebuffer*	blurredshadow	= nullptr;

OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				wood			= 0;
GLuint				environment		= 0;
GLuint				helptext		= 0;

BasicCamera			camera;
BasicCamera			light;
Math::AABox			scenebox;

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

	// load meshes
	if (!GLCreateMeshFromQM("../../Media/MeshesQM/sky.qm", &skymesh)) {
		MYERROR("Could not load 'sky'");
		return false;
	}

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/box.qm", &table)) {
		MYERROR("Could not load 'box'");
		return false;
	}

	if (!GLCreateMeshFromQM("../../Media/MeshesQM/angel.qm", &angel)) {
		MYERROR("Could not load 'angel'");
		return false;
	}

	// load textures
	const char* files[6] = {
		"../../Media/Textures/cubemap1_posx.jpg",
		"../../Media/Textures/cubemap1_negx.jpg",
		"../../Media/Textures/cubemap1_posy.jpg",
		"../../Media/Textures/cubemap1_negy.jpg",
		"../../Media/Textures/cubemap1_posz.jpg",
		"../../Media/Textures/cubemap1_negz.jpg"
	};

	GLCreateCubeTextureFromFiles(files, true, &environment);
	GLCreateTextureFromFile("../../Media/Textures/burloak.jpg", true, &wood);

	// create render targets
	scenetarget = new OpenGLFramebuffer(screenwidth, screenheight);
	scenetarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_LINEAR);
	scenetarget->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D24S8);

	if (!scenetarget->Validate())
		return false;

	bloomtarget = new OpenGLFramebuffer(screenwidth, screenheight);
	bloomtarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_LINEAR);

	if (!bloomtarget->Validate())
		return false;

	shadowmap = new OpenGLFramebuffer(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	shadowmap->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);

	float border[] = { 1, 1, 1, 1 };

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

	shadowmap->AttachRenderbuffer(GL_DEPTH_ATTACHMENT, GLFMT_D24S8);

	if (!shadowmap->Validate())
		return false;

	blurredshadow = new OpenGLFramebuffer(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	blurredshadow->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_G32R32F, GL_LINEAR);

	if (!blurredshadow->Validate())
		return false;

	// load shaders
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/sky.vert", 0, 0, 0, "../../Media/ShadersGL/sky.frag", &skyeffect)) {
		MYERROR("Could not load 'sky' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/bloomcombine.frag", &bloom)) {
		MYERROR("Could not load 'bloom' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/shadowmap_variance.vert", 0, 0, 0, "../../Media/ShadersGL/shadowmap_variance.frag", &varianceshadow)) {
		MYERROR("Could not load 'shadowmap' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/blinnphong_variance.vert", 0, 0, 0, "../../Media/ShadersGL/blinnphong_variance.frag", &blinnphong)) {
		MYERROR("Could not load 'blinnphong' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/boxblur3x3.frag", &boxblur3x3)) {
		MYERROR("Could not load 'boxblur3x3' shader");
		return false;
	}

	Math::Matrix identity(1, 1, 1, 1);

	skyeffect->SetInt("sampler0", 0);
	varianceshadow->SetInt("isPerspective", 1);

	boxblur3x3->SetMatrix("matTexture", identity);
	boxblur3x3->SetInt("sampler0", 0);

	blinnphong->SetInt("sampler0", 0);
	blinnphong->SetInt("sampler1", 1);
	blinnphong->SetInt("isPerspective", 1);

	bloom->SetMatrix("matTexture", identity);
	bloom->SetInt("sampler0", 0);
	bloom->SetInt("sampler1", 1);

	screenquad = new OpenGLScreenQuad();

	// setup camera
	camera.SetAspect((float)screenwidth / screenheight);
	camera.SetFov(Math::HALF_PI);
	camera.SetClipPlanes(0.1f, 30.0f);
	camera.SetDistance(1.7f);
	camera.SetPosition(0, 0.5f, 0);
	camera.SetOrientation(Math::DegreesToRadians(135), 0.45f, 0);
	camera.SetPitchLimits(0.3f, Math::HALF_PI);

	light.SetAspect(1);
	light.SetFov(Math::HALF_PI);
	light.SetClipPlanes(0.1f, 30.0f);
	light.SetDistance(3);
	light.SetPosition(0, 0.5f, 0);
	light.SetOrientation(Math::DegreesToRadians(225), 0.78f, 0);
	light.SetPitchLimits(-0.4f, Math::HALF_PI);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext);

	GLRenderText(
		"Use the mouse to rotate camera and light",
		helptext, 512, 512);

	scenebox.Min = { -5, -1, -5 };
	scenebox.Max = { 5, 2.4f, 5 };

	return true;
}

void UninitScene()
{
	delete skymesh;
	delete angel;
	delete table;

	delete skyeffect;
	delete bloom;
	delete varianceshadow;
	delete blinnphong;
	delete boxblur3x3;

	delete scenetarget;
	delete bloomtarget;
	delete shadowmap;
	delete blurredshadow;

	delete screenquad;

	GL_SAFE_DELETE_TEXTURE(environment);
	GL_SAFE_DELETE_TEXTURE(wood);
	GL_SAFE_DELETE_TEXTURE(helptext);

	OpenGLContentManager().Release();
}

void KeyUp(KeyCode key)
{
}

void MouseMove(int32_t x, int32_t y, int16_t dx, int16_t dy)
{
	uint8_t state = app->GetMouseButtonState();

	if (state & MouseButtonLeft) {
		camera.OrbitRight(Math::DegreesToRadians(dx));
		camera.OrbitUp(Math::DegreesToRadians(dy));
	} else if (state & MouseButtonRight) {
		light.OrbitRight(Math::DegreesToRadians(dx));
		light.OrbitUp(Math::DegreesToRadians(dy));
	}
}

void Update(float delta)
{
	camera.Update(delta);
	light.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	Math::Matrix world, view, proj;
	Math::Matrix lightview, lightproj;
	Math::Matrix viewproj, lightviewproj;
	Math::Vector3 eye, lightpos, lightlook;
	Math::Vector2 clipplanes;

	camera.Animate(alpha);
	light.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);

	light.GetViewMatrix(lightview);
	light.GetEyePosition(lightpos);
	light.GetPosition(lightlook);

	Math::FitToBoxPerspective(clipplanes, lightpos, lightlook - lightpos, scenebox);

	light.SetClipPlanes(clipplanes.x, clipplanes.y);
	light.GetProjectionMatrix(lightproj);

	Math::MatrixMultiply(lightviewproj, lightview, lightproj);

	// render shadow map
	shadowmap->Set();
	{
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		Math::MatrixScaling(world, 10, 0.2f, 10);
		world._42 = -1;

		varianceshadow->SetMatrix("matViewProj", lightviewproj);
		varianceshadow->SetVector("clipPlanes", clipplanes);
		varianceshadow->SetMatrix("matWorld", world);

		varianceshadow->Begin();
		{
			table->Draw();

			Math::MatrixScaling(world, 0.006667f, 0.006667f, 0.006667f);
			world._42 = -0.9f;

			varianceshadow->SetMatrix("matWorld", world);
			varianceshadow->CommitChanges();

			angel->Draw();
		}
		varianceshadow->End();
	}
	shadowmap->Unset();

	// blur shadowmap
	float texelsize[] = { 1.0f / SHADOWMAP_SIZE, 1.0f / SHADOWMAP_SIZE };

	glDisable(GL_DEPTH_TEST);
	GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, shadowmap->GetColorAttachment(0));

	boxblur3x3->SetVector("texelSize", texelsize);

	blurredshadow->Set();
	{
		boxblur3x3->Begin();
		{
			screenquad->Draw(false);
		}
		boxblur3x3->End();
	}
	blurredshadow->Unset();

	glEnable(GL_DEPTH_TEST);

	// render scene
	float ambient[]	= { 0.1f, 0.1f, 0.1f, 1 };
	float black[]	= { 0, 0, 0, 1 };
	float white[]	= { 1, 1, 1, 1 };
	float uv[4]		= { 3, 3, 0, 1 };

	scenetarget->Set();
	{
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		// sky
		Math::MatrixScaling(world, 20, 20, 20);

		world._41 = eye.x;
		world._42 = eye.y;
		world._43 = eye.z;

		skyeffect->SetVector("eyePos", eye);
		skyeffect->SetMatrix("matWorld", world);
		skyeffect->SetMatrix("matViewProj", viewproj);

		skyeffect->Begin();
		{
			GLSetTexture(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP, environment);
			skymesh->Draw();
		}
		skyeffect->End();

		// objects
		GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, blurredshadow->GetColorAttachment(0));

		blinnphong->SetMatrix("matViewProj", viewproj);
		blinnphong->SetMatrix("lightViewProj", lightviewproj);
		blinnphong->SetVector("eyePos", eye);
		blinnphong->SetVector("lightPos", Math::Vector4(lightpos, 1));
		blinnphong->SetVector("lightColor", white);
		blinnphong->SetVector("clipPlanes", clipplanes);
		blinnphong->SetVector("uv", uv);
		blinnphong->SetFloat("ambient", 0.1f);

		blinnphong->Begin();
		{
			// table
			Math::MatrixScaling(world, 10, 0.2f, 10);
			world._42 = -1;

			blinnphong->SetMatrix("matWorld", world);
			blinnphong->SetMatrix("matWorldInv", world);
			blinnphong->SetVector("matSpecular", black);
			blinnphong->CommitChanges();

			GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, wood);
			table->Draw();

			// angel
			Math::MatrixScaling(world, 0.006667f, 0.006667f, 0.006667f);
			world._42 = -0.9f;

			blinnphong->SetMatrix("matWorld", world);
			blinnphong->SetMatrix("matWorldInv", world);
			blinnphong->SetVector("matSpecular", white);
			blinnphong->CommitChanges();

			angel->DrawSubset(0, true);
		}
		blinnphong->End();
	}
	scenetarget->Unset();

	// blur scene
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	texelsize[0] = 1.0f / screenwidth;
	texelsize[1] = 1.0f / screenheight;

	glDisable(GL_DEPTH_TEST);
	GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, scenetarget->GetColorAttachment(0));

	boxblur3x3->SetVector("texelSize", texelsize);

	bloomtarget->Set();
	{
		boxblur3x3->Begin();
		{
			screenquad->Draw(false);
		}
		boxblur3x3->End();
	}
	bloomtarget->Unset();

	// combine
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glEnable(GL_FRAMEBUFFER_SRGB);
	
	GLSetTexture(GL_TEXTURE0, GL_TEXTURE_2D, scenetarget->GetColorAttachment(0));
	GLSetTexture(GL_TEXTURE1, GL_TEXTURE_2D, bloomtarget->GetColorAttachment(0));

	bloom->Begin();
	{
		screenquad->Draw(false);
	}
	bloom->End();

	// render text
	glViewport(10, screenheight - 522, 512, 512);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FRAMEBUFFER_SRGB);

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
	app->KeyUpCallback = KeyUp;
	app->MouseMoveCallback = MouseMove;

	app->Run();
	delete app;

	return 0;
}
