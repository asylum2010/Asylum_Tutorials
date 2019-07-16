
#pragma comment(lib, "OpenGL32.lib")
#pragma comment(lib, "GLU32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdiplus.lib")

#include <iostream>

#include "..\Common\application.h"
#include "..\Common\gl4ext.h"
#include "..\Common\basiccamera.h"

#define MAX_SAMPLES			16384	// for path tracer

// helper macros
#define TITLE				"Shader sample 54: Compare GTAO to path tracing"
#define MYERROR(x)			{ std::cout << "* Error: " << x << "!\n"; }

// sample structures
struct SceneObject
{
	Math::Vector4 params1;	// sphere: position & radius
	Math::Vector4 params2;
	Math::Color color;
	int type;
	int padding[3];
};

// sample variables
Application*		app				= nullptr;

OpenGLEffect*		pathtracer		= nullptr;
OpenGLEffect*		gtaoeffect		= nullptr;
OpenGLEffect*		gbuffereffect	= nullptr;

OpenGLFramebuffer*	gbuffer			= nullptr;
OpenGLFramebuffer*	gtaotarget		= nullptr;
OpenGLFramebuffer*	accumtargets[2]	= { nullptr };	// for path tracer
OpenGLScreenQuad*	screenquad		= nullptr;

GLuint				sceneobjects	= 0;
GLuint				helptext1		= 0;
GLuint				helptext2		= 0;

BasicCamera			camera;
int					currtarget		= 0;
int					currsample		= 0;
int					numobjects		= 20;

static void GenerateSphereTetrahedron(SceneObject* data, int n, float radius)
{
	// places spheres in a tetrahedral formation
	const float cos30 = 0.86602f;
	const float sin30 = 0.5f;
	const float tan30 = 0.57735f;
	const float sqrt3 = 1.73205f;
	const float tH = 1.63299f;

	float start[3];
	float x, y, z;
	int count = 0;

	start[0] = -n * radius * tan30;
	start[1] = radius;

	for (int ly = 0; ly < n; ++ly) {
		start[2] = -(n - ly - 1) * radius;

		for (int lx = 0; lx < n - ly; ++lx) {
			for (int lz = 0; lz < n - lx - ly; ++lz) {
				x = start[0] + lx * sqrt3 * radius;
				y = 0.01f + start[1] + ly * tH * radius;
				z = start[2] + lz * 2 * radius;

				SceneObject& obj = data[count];

				obj.params1 = Math::Vector4(x, y, z, radius);
				obj.params2 = Math::Vector4(0, 0, 0, 0);
				obj.color = Math::Color(1.0f, 0.3f, 0.1f, 1.0f);
				obj.type = 2;

				// NOTE: for Mitsuba
				//printf("<shape type=\"sphere\">\n\t<point name=\"center\" x=\"%.3f\" y=\"%.3f\" z=\"%.3f\"/>\n\t<float name=\"radius\" value=\"0.5\"/>\n</shape>\n\n", x, y, z);

				++count;
			}

			start[2] += 2 * radius * sin30;
		}

		start[0] += radius * tan30;
	}
}

bool InitScene()
{
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	glClearColor(0.0f, 0.125f, 0.3f, 1.0f);
	glClearDepth(1.0);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	glDisable(GL_DEPTH_TEST);

	// create scene
	glGenBuffers(1, &sceneobjects);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, sceneobjects);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 21 * sizeof(SceneObject), nullptr, GL_STATIC_DRAW);

	SceneObject* data = (SceneObject*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
	{
		GenerateSphereTetrahedron(data, 4, 0.5f);

		SceneObject& ground = data[20];

		ground.params1 = Math::Vector4(0.0f, 1.0f, 0.0f, 0.0f);
		ground.params2 = Math::Vector4(0, 0, 0, 0);
		ground.color = Math::Color(0.664f, 0.824f, 0.85f, 1.0f);
		ground.type = 1;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// create render targets
	gtaotarget = new OpenGLFramebuffer(screenwidth / 2, screenheight);

	gtaotarget->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A16B16G16R16F, GL_NEAREST);

	GL_ASSERT(gtaotarget->Validate());

	accumtargets[0] = new OpenGLFramebuffer(screenwidth / 2, screenheight);
	accumtargets[1] = new OpenGLFramebuffer(screenwidth / 2, screenheight);

	accumtargets[0]->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A32B32G32R32F, GL_NEAREST);
	accumtargets[1]->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A32B32G32R32F, GL_NEAREST);

	GL_ASSERT(accumtargets[0]->Validate());
	GL_ASSERT(accumtargets[1]->Validate());

	GLint value = 0;
	float white[] = { 1, 1, 1, 1 };

	gbuffer = new OpenGLFramebuffer(screenwidth / 2, screenheight);

	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT0, GLFMT_A8B8G8R8_sRGB, GL_NEAREST);	// color
	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT1, GLFMT_A16B16G16R16F, GL_NEAREST);	// normals
	gbuffer->AttachTexture(GL_COLOR_ATTACHMENT2, GLFMT_R32F, GL_NEAREST);			// depth

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white);

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
	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/aopathtracer.frag", &pathtracer)) {
		MYERROR("Could not load 'aopathtracer' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/aopathtracer.frag", &gbuffereffect, "#define RENDER_GBUFFER\n")) {
		MYERROR("Could not load 'gbuffer' shader");
		return false;
	}

	if (!GLCreateEffectFromFile("../../Media/ShadersGL/screenquad.vert", 0, 0, 0, "../../Media/ShadersGL/gtaoreference.frag", &gtaoeffect)) {
		MYERROR("Could not load 'gtao' shader");
		return false;
	}

	pathtracer->SetInt("prevIteration", 0);

	gtaoeffect->SetInt("gbufferDepth", 0);
	gtaoeffect->SetInt("gbufferNormals", 1);

	screenquad = new OpenGLScreenQuad();

	// setup camera
	camera.SetAspect((screenwidth * 0.5f) / (float)screenheight);
	camera.SetFov(Math::PI / 4.0f);
	camera.SetPosition(0, 1.633f, 0);
	camera.SetOrientation(Math::DegreesToRadians(135), Math::DegreesToRadians(30), 0);
	camera.SetDistance(7);
	camera.SetClipPlanes(0.1f, 20);

	// render text
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext1);
	GLCreateTexture(512, 512, 1, GLFMT_A8B8G8R8, &helptext2);

	GLRenderText("GTAO", helptext1, 512, 512);
	GLRenderText("Path traced reference", helptext2, 512, 512);

	return true;
}

void UninitScene()
{
	delete pathtracer;
	delete gtaoeffect;
	delete gbuffereffect;
	delete gbuffer;
	delete gtaotarget;
	delete accumtargets[0];
	delete accumtargets[1];
	delete screenquad;

	GL_SAFE_DELETE_BUFFER(sceneobjects);
	GL_SAFE_DELETE_TEXTURE(helptext1);
	GL_SAFE_DELETE_TEXTURE(helptext2);

	OpenGLContentManager().Release();
}

void KeyUp(KeyCode key)
{
	switch (key) {
	case KeyCode1:
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
	}
}

void Update(float delta)
{
	camera.Update(delta);
}

void Render(float alpha, float elapsedtime)
{
	static float time = 0;

	Math::Matrix world, view, proj;
	Math::Matrix viewinv;
	Math::Matrix viewproj, viewprojinv;
	Math::Vector4 clipinfo;
	Math::Vector3 eye;

	camera.Animate(alpha);

	camera.GetViewMatrix(view);
	camera.GetProjectionMatrix(proj);
	camera.GetEyePosition(eye);

	Math::MatrixMultiply(viewproj, view, proj);
	Math::MatrixInverse(viewprojinv, viewproj);
	Math::MatrixInverse(viewinv, view);

	clipinfo.x = camera.GetNearPlane();
	clipinfo.y = camera.GetFarPlane();
	clipinfo.z = 0.5f * (gbuffer->GetHeight() / (2.0f * tanf(camera.GetFov() * 0.5f)));

	// GTAO
	gbuffer->Set();
	{
		glClear(GL_COLOR_BUFFER_BIT);

		gbuffereffect->SetMatrix("matView", view);
		gbuffereffect->SetMatrix("matViewInv", viewinv);
		gbuffereffect->SetMatrix("matViewProjInv", viewprojinv);
		gbuffereffect->SetVector("eyePos", eye);
		gbuffereffect->SetVector("clipPlanes", clipinfo);
		gbuffereffect->SetInt("numObjects", numobjects);

		gbuffereffect->Begin();
		{
			screenquad->Draw(false);
		}
		gbuffereffect->End();
	}
	gbuffer->Unset();

	Math::Vector4 projinfo = { 2.0f / (gbuffer->GetWidth() * proj._11), 2.0f / (gbuffer->GetHeight() * proj._22), -1.0f / proj._11, -1.0f / proj._22 };

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gbuffer->GetColorAttachment(2));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gbuffer->GetColorAttachment(1));

	gtaoeffect->SetVector("eyePos", eye);
	gtaoeffect->SetVector("projInfo", projinfo);
	gtaoeffect->SetVector("clipInfo", clipinfo);

	gtaotarget->Set();
	{
		gtaoeffect->Begin();
		{
			screenquad->Draw(false);
		}
		gtaoeffect->End();
	}
	gtaotarget->Unset();

	glActiveTexture(GL_TEXTURE0);

	// path traced reference
	if (!camera.IsAnimationFinished() || (app->GetMouseButtonState() & MouseButtonLeft))
		currsample = 0;

	if (currsample == 0) {
		accumtargets[currtarget]->Set();
		glClear(GL_COLOR_BUFFER_BIT);
		accumtargets[currtarget]->Unset();
	}

	if (currsample < MAX_SAMPLES) {
		currtarget = 1 - currtarget;
		++currsample;

		accumtargets[currtarget]->Set();
		{
			glClear(GL_COLOR_BUFFER_BIT);
			glBindTexture(GL_TEXTURE_2D, accumtargets[1 - currtarget]->GetColorAttachment(0));
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sceneobjects);

			pathtracer->SetMatrix("matViewProjInv", viewprojinv);
			pathtracer->SetVector("eyePos", eye);
			pathtracer->SetFloat("time", time);
			pathtracer->SetFloat("currSample", (float)currsample);
			pathtracer->SetInt("numObjects", numobjects);

			pathtracer->Begin();
			{
				screenquad->Draw(false);
			}
			pathtracer->End();
		}
		accumtargets[currtarget]->Unset();
	}

	// present
	uint32_t screenwidth = app->GetClientWidth();
	uint32_t screenheight = app->GetClientHeight();

	Math::MatrixIdentity(world);
	screenquad->SetTextureMatrix(world);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glClear(GL_COLOR_BUFFER_BIT);

	glViewport(0, 0, screenwidth / 2, screenheight);
	glBindTexture(GL_TEXTURE_2D, gtaotarget->GetColorAttachment(0));

	screenquad->Draw();

	glViewport(screenwidth / 2, 0, screenwidth / 2, screenheight);
	glBindTexture(GL_TEXTURE_2D, accumtargets[currtarget]->GetColorAttachment(0));
	
	screenquad->Draw();

	time += elapsedtime;

	// render text
	glDisable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	Math::Vector4 xzplane = { 0, 1, 0, -0.5f };
	Math::MatrixReflect(world, xzplane);

	screenquad->SetTextureMatrix(world);

	glViewport(10, screenheight - 522, 512, 512);
	glBindTexture(GL_TEXTURE_2D, helptext1);
	
	screenquad->Draw();

	glViewport(screenwidth / 2 + 10, screenheight - 522, 512, 512);
	glBindTexture(GL_TEXTURE_2D, helptext2);
	
	screenquad->Draw();

	// reset states
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
